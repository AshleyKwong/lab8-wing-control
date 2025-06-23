#include <elapsedMillis.h>
#include <NewPing.h>
#include <ctype.h>
elapsedMillis timeElapsed;

#define numberOfActuators 6
#define numberOfSensors 4
int LPWM[numberOfActuators] = { 8, 9, 10, 11, 12, 13 };
int RPWM[numberOfActuators] = { 2, 3, 4, 5, 6, 7 };
float dutyCycle[numberOfActuators] = { 0.5, 0.5, 0.5, 0.5, 0.26, 0.26 };  // last two for the flaps
float Kp[numberOfActuators] = { 1, 1, 1, 1, 1, 1 };
float sensorReading[numberOfSensors] = { 0, 0, 0, 0 };  // init as zeros

int triggerPins[numberOfSensors] = { 23, 25, 27, 29 };
int echoPins[numberOfSensors] = { 22, 24, 26, 28 };
NewPing sonar[numberOfSensors] = {
  NewPing(triggerPins[0], echoPins[0], 60),  // max 60 cm for the ping --> from the ceiling
  NewPing(triggerPins[1], echoPins[1], 60),
  NewPing(triggerPins[2], echoPins[2], 60),
  NewPing(triggerPins[3], echoPins[3], 60)
};

float motorSpeed = 0.36;  // cm/s
struct nanoToMega {
  char cmd[16];
  char frontOrAft[2];
  int direction;
  float distanceReq;
  int indivActuator;
};
nanoToMega recievedStruct;


enum CmdState {
  SET_Y0,
  SET_AOA,
  FLAP_AOA,
  FRONT_AFT,
  SENSOR_CHECK,
  INDIV_ACTUATOR,
  UNKNOWN_CMD
};
bool handshakeDone = false;

void setup() {
  Serial.begin(9600);   // Debugging
  Serial1.begin(9600);  // Serial1 on Mega: RX=19, TX=18
  Serial.println("Mega ready");
  for (int i = 0; i < numberOfActuators; i++) {
    pinMode(RPWM[i], OUTPUT);
    pinMode(LPWM[i], OUTPUT);
    pinMode(triggerPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }
}

void loop() {
  // read in the ping from nano and output pong to confirm cxn
  while (!handshakeDone) {
    if (Serial1.available()) {
      String incoming = Serial1.readStringUntil('\n');
      if (incoming.indexOf("Ping!") >= 0) {
        Serial1.println("Pong!");
        Serial.println("Handshake complete.");
        handshakeDone = true;
        while (Serial1.available() > 0) {
          Serial1.read();  // clear to make way for the cmd line.
        }
        delay(1000);
      }
    }
  }
  // if(!handshakeDone){
  //   Serial.println("Handshake wasn't completed!");
  // }


  if (readNanoStructFromSerial(recievedStruct)) {
    CmdState commandToDo = parseCmd(recievedStruct.cmd);
    switch (commandToDo) {
      case SET_Y0:
        // Serial.println("Recieved set Y0 from Nano 2.");
        // Serial.println(recievedStruct.direction);
        // Serial.println(recievedStruct.distanceReq);
        driveActuator(numberOfActuators, recievedStruct.direction, recievedStruct.distanceReq, 0, 0, 0);
        // set the other arg to 0 jic for safety.
        break;
      case SET_AOA:
        // Handle SET_AOA behavior
        Serial.println("Inside set aoa");
        driveActuator(2, recievedStruct.direction, recievedStruct.distanceReq, 0, 0, 0);
        Serial.println("Done moving.");
        break;
      case FLAP_AOA:
        // Handle FLAP_AOA behavior
        driveActuator(numberOfActuators, recievedStruct.direction, recievedStruct.distanceReq, 0, 1, 0);
        break;
      case FRONT_AFT:
        // Handle FRONT_AFT behavior
        if (toupper(recievedStruct.frontOrAft[0]) == 'F') {
          driveActuator(2, recievedStruct.direction, recievedStruct.distanceReq, 0, 0, 0);
        } else if (toupper(recievedStruct.frontOrAft[0]) == 'R') {
          driveActuator(numberOfActuators, recievedStruct.direction, recievedStruct.distanceReq, 1, 0, 0);
        }
        break;
      case SENSOR_CHECK:
        int i;
        for (i = 0; i < numberOfSensors; i++) {
          sensorReading[i] = sampleDistance(sonar[i]);
        }
        Serial1.print("FL sensor reading:  ");  // print to the nano.
        Serial1.println(sensorReading[0]);
        Serial1.print("FR sensor reading:  ");
        Serial1.println(sensorReading[1]);
        Serial1.print("AL sensor reading:  ");
        Serial1.println(sensorReading[2]);
        Serial1.print("AR sensor reading:  ");
        Serial1.println(sensorReading[3]);
        break;
      case INDIV_ACTUATOR:
        // Handle INDIV_ACTUATOR behavior
        driveActuator(numberOfActuators, recievedStruct.direction, recievedStruct.distanceReq, 0, 0, recievedStruct.indivActuator);
        break;
      default:
        // Handle unknown command
        Serial1.println("Unable to process command. Please send again.");  // sends to nano.
        break;
    }
  }
}


bool readNanoStructFromSerial(nanoToMega& data) {
  if (Serial1.available()) {
    String incoming = Serial1.readStringUntil('\n');
    // Find comma indices
    int idx1 = incoming.indexOf(',');
    int idx2 = incoming.indexOf(',', idx1 + 1);
    int idx3 = incoming.indexOf(',', idx2 + 1);
    int idx4 = incoming.indexOf(',', idx3 + 1);

    // Basic validity check
    if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1) {
      return false;  // Malformed input
    }

    // Parse and assign safely
    incoming.substring(0, idx1).toCharArray(data.cmd, sizeof(data.cmd));
    data.direction = incoming.substring(idx1 + 1, idx2).toInt();
    data.distanceReq = incoming.substring(idx2 + 1, idx3).toFloat();
    incoming.substring(idx3 + 1, idx4).toCharArray(data.frontOrAft, sizeof(data.frontOrAft));
    data.indivActuator = incoming.substring(idx4 + 1).toInt();

    return true;  // Successfully parsed
  }
  return false;  // No data available
}


CmdState parseCmd(const char* cmd) {
  if (strcmp(cmd, "SET_Y0") == 0) return SET_Y0;
  if (strcmp(cmd, "SET_AOA") == 0) return SET_AOA;
  if (strcmp(cmd, "FLAP_AOA") == 0) return FLAP_AOA;
  if (strcmp(cmd, "FRONT_AFT") == 0) return FRONT_AFT;
  if (strcmp(cmd, "SENSOR_CHECK") == 0) return SENSOR_CHECK;
  if (strcmp(cmd, "INDIV_ACTUATOR") == 0) return INDIV_ACTUATOR;
  return UNKNOWN_CMD;
}



void driveActuator(int Actuators, int Direction, float distanceReq, int rearActuatorsBool, int flapActuatorsBool, int indivActuatorMove) {
  unsigned long* startTimeMillis = new unsigned long[Actuators];

  unsigned long timeToMove = (abs(distanceReq) / motorSpeed) * 1000;  // ms

  const int maxActuators = 6;
  if (Actuators > maxActuators) Actuators = maxActuators;  // safety jic
  for (int i = 0; i < Actuators; i++) {
    startTimeMillis[i] = 0;  // init as zeros.
  }
  int startI = 0;
  if (rearActuatorsBool == 0) {
    startI = 0;

  } else if (rearActuatorsBool == 1) {
    startI = 2;  // will only move the back actuators. should also move the flap actuators as well.
  }
  if (flapActuatorsBool == 1) {
    startI = 4;
  }

  if (indivActuatorMove > 0 && indivActuatorMove <= Actuators) {
    startI = indivActuatorMove - 1;
    Actuators = startI + 1;
  }

  for (int i = startI; i < Actuators; i++) {
    if (i >= maxActuators) {
      continue;  // saftety check
    }
    int movePin, stopPin;
    int flapMovePin, flapStopPin;
    if (Direction == 1) {
      movePin = RPWM[i];
      stopPin = LPWM[i];
    } else if (Direction == -1) {
      movePin = LPWM[i];
      stopPin = RPWM[i];
    } else {
      continue;
    }
    // Serial.print("i: ");
    // Serial.print(i);
    // Serial.print(", movePin: ");
    // Serial.print(movePin);
    // Serial.print(", PWM value: ");
    // Serial.println(dutyCycle[i] * 255 * Kp[i]);
    // Serial.print("Time to move: "); 
    // Serial.println(timeToMove); 
    analogWrite(movePin, dutyCycle[i] * 255 * Kp[i]);
    analogWrite(stopPin, 0);
    startTimeMillis[i] = millis();
  }
  bool allFinished = false;  // checker to make sure they all stop at same time!
  while (!allFinished) {
    allFinished = true;
    for (int i = startI; i < Actuators; i++) {
      if (i >= maxActuators) {
        continue;
      }
      int movePin, stopPin;
      if (Direction == 1) {
        movePin = RPWM[i];
        stopPin = LPWM[i];
      } else {
        movePin = LPWM[i];
        stopPin = RPWM[i];
      }
      if (millis() - startTimeMillis[i] < timeToMove) {
        allFinished = false;  // keep moving!
      } else {
        analogWrite(movePin, 0);
        analogWrite(stopPin, 0);  // stop evt !
      }
    }
  }
  delete[] startTimeMillis;  // to clear space
}
float sampleDistance(NewPing sonarObj) {
  unsigned int medianPing = sonarObj.ping_median(20);  // return the ping object after sampling for 20 microseconds
  int distance = sonarObj.convert_cm(medianPing);      // convert to cm
  int floor_distance = 100 - distance;                 // height away from the floor.
  return floor_distance;                               // return the distance it is at
}