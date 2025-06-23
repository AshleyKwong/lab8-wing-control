#include <WiFi.h>
#include <esp_now.h>

uint8_t receiverAddress[] = { 0x74, 0x4D, 0xBD, 0xA0, 0xC9, 0x50 };

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}
struct nanoToNano {
  char cmd[16];
  char frontOrAft[2];  // either F or R
  int direction;
  float distanceReq;
  int indivActuator;
};
// String cmdList[6] = { "SET_Y0", "SET_AOA", "FLAP_AOA", "FRONT_AFT", "SENSOR_CHECK", "INDIV_ACTUATOR" };
const char cmdList[6][16] = {
  "SET_Y0",
  "SET_AOA",
  "FLAP_AOA",
  "FRONT_AFT",
  "SENSOR_CHECK",
  "INDIV_ACTUATOR"
};

int userMenuChoice;
int attempt = 0;
// bool handshakeSuccess = false;
// int maxRetries = 5;
// int attempt = 0;
// const unsigned long timeout = 5000;  // 5 seconds timeout
void setup() {
  WiFi.mode(WIFI_STA);  // station mode
  Serial.begin(9600);   // Debugging
  delay(2000);
  Serial.println("Nano ESP32 Sender Ready.");
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1)
      ;
  }
  esp_now_register_send_cb(onSent);  // call back function to see if message was successful

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    // Serial.println("Failed to add peer...trying again.");
    // while(int i = 0; i < 5; i++){
    //   esp_now_add_peer(peerInfo);
    //   Serial.print("Attempt ");
    //   Serial.println(i);
    while (1)
      ;
  }
}
// init in the beginning - all function def at at the bottom.
void setY0();
void setAOA();
void setFlap();
void moveFrontsOrAft();
void checkSensors();
void moveIndivActuators();
int printMenu();
void clearQuery();
void resetStruct(nanoToNano &val);

void loop() {
  // while (!handshakeSuccess) {
  //   Serial1.println("Ping!");
  //   // Print anything received from Mega
  //   if (Serial1.available()) {
  //     String incoming = Serial1.readStringUntil('\n');
  //     Serial.print("Received from Mega: ");
  //     Serial.println(incoming);
  //     if (incoming.indexOf("Pong!") >= 0) {
  //       Serial.println("Handshake complete.");
  //       handshakeSuccess = true;
  //       while (Serial1.available() > 0) {  // clear the buffer.
  //         Serial1.read();
  //       }
  //       break;
  //     }
  //   }
  //   delay(1000);
  // }
  // if (handshakeSuccess) {
  while (true) {  // begin infinite loop for main menu
    userMenuChoice = printMenu();
    switch (userMenuChoice) {
      case 1:
        setY0();
        break;
      case 2:
        setAOA();
        break;
      case 3:
        setFlap();
        break;
      case 4:
        moveFrontsOrAft();
        break;
      case 5:
        checkSensors();
        break;
      case 6:
        moveIndivActuators();
        break;
    }
  }
  // }
}

int printMenu() {
  int menuChoice;
  while (true) {
    Serial.println("Menu Options:\n1. y0 move.\n2. AOA change.\n3. Flap AOA.\n4. Move Front/Aft Actuator Pairs.\n5. Position Check. \n6. Move Individual Actuator.");
    Serial.print("Enter your choice (1-6): ");
    while (Serial.available() == 0) {
      // Wait for user input
    }
    menuChoice = Serial.parseInt();
    clearQuery();
    Serial.print("Selected: ");
    Serial.println(menuChoice);
    if (menuChoice >= 1 && menuChoice <= 6) {
      break;  // Valid input, exit loop
    } else {
      Serial.println("Error. Enter 1-6 for the listed options.");
      delay(200);
    }
  }
  return menuChoice;
}


void setY0() {
  nanoToNano userStruct;
  String stringDir;
  String sendCmdToMega;
  // define all necessary things before
  while (true) {
    strncpy(userStruct.cmd, cmdList[0], sizeof(userStruct.cmd));
    userStruct.cmd[sizeof(userStruct.cmd) - 1] = '\0';  // Ensure null-termination
    userStruct.indivActuator = 0;
    Serial.println("Where to move? y0 [cm]: ");
    while (Serial.available() == 0) {}
    float userInput = Serial.parseFloat();
    userStruct.distanceReq = abs(userInput);
    clearQuery();  // loops until clear. all vals stored in a variable
    if (userInput != 0) {
      int requestedDirection = (userInput > 0) ? -1 : 1;
      userStruct.direction = requestedDirection;  // based on the user input sign assigns a direction
      if (requestedDirection == 1) {
        stringDir = "DOWN towards floor";
      } else {
        stringDir = "UP towards ceiling";
      }
      Serial.print("Moving ");
      Serial.print(userInput);
      Serial.print(" ");
      Serial.println(stringDir);
      Serial.println("Sending to Nano 2...");
      esp_now_send(receiverAddress, (uint8_t *)&userStruct, sizeof(userStruct));
      delay(2000);  // gives some time for the nano to catch up.
    } else {
      clearQuery();
      Serial.println("Invalid input. Enter nonzero value...ex: - 10 cm for 10 cm to floor.");
      delay(200);
      continue;  // continues the loop !
    }
    Serial.println("Continue? (Y/N)");
    while (Serial.available() == 0) {}
    String userCont = Serial.readStringUntil('\n');
    userCont.trim();
    if (userCont.equalsIgnoreCase("Y")) {
      resetStruct(userStruct);  // reset to default values.
      clearQuery();
    } else if (userCont.equalsIgnoreCase("N")) {
      clearQuery();
      Serial.println("Exiting...");
      break;
    } else {
      clearQuery();  // prep for the repeat.
      Serial.println("Invalid input. Please enter 'Y' or 'N'.");
      delay(200);
    }
  }
}

void setAOA() {
  nanoToNano userStruct;
  String stringDir;
  String sendCmdToMega;
  while (true) {
    strncpy(userStruct.cmd, cmdList[1], sizeof(userStruct.cmd));
    userStruct.cmd[sizeof(userStruct.cmd) - 1] = '\0';  // Ensure null-termination
    userStruct.indivActuator = 0;
    Serial.print("Where to move front actuators?: ");
    while (Serial.available() == 0) {}
    float userInput = Serial.parseFloat();
    clearQuery();
    if (userInput != 0) {
      userStruct.distanceReq = userInput;  // only input if valid.
      int requestedDirection = (userInput > 0) ? -1 : 1;
      userStruct.direction = requestedDirection;
      if (requestedDirection == 1) {
        stringDir = "DOWN towards floor";
      } else {
        stringDir = "UP towards ceiling";
      }
      Serial.print("Moving ");
      Serial.print(userInput);
      Serial.print(" ");
      Serial.println(stringDir);
      Serial.println("Sending to Nano 2...");
      esp_now_send(receiverAddress, (uint8_t *)&userStruct, sizeof(userStruct));
      delay(2000);  // gives some time for the nano to catch up.
    } else {
      clearQuery();
      Serial.println("Invalid input. Enter nonzero value...ex: -10 cm for 10 cm to floor.");
      delay(200);
      continue;
    }
    Serial.println("Continue? (Y/N)");
    while (Serial.available() == 0) {}
    String userCont = Serial.readStringUntil('\n');
    userCont.trim();
    clearQuery();
    if (userCont.equalsIgnoreCase("Y")) {
      resetStruct(userStruct);
      clearQuery();
    } else if (userCont.equalsIgnoreCase("N")) {
      clearQuery();
      Serial.println("Exiting...");
      break;
    } else {
      clearQuery();
      Serial.println("Invalid input. Please enter 'Y' or 'N'.");
      delay(200);
    }
  }
}

void setFlap() {  // need to set the int flap arg in the mega code for driveActuator
  nanoToNano userStruct;
  String stringDir;
  while (true) {
    strncpy(userStruct.cmd, cmdList[2], sizeof(userStruct.cmd));
    userStruct.cmd[sizeof(userStruct.cmd) - 1] = '\0';  // Ensure null-termination
    userStruct.indivActuator = 0;
    Serial.print("Enter displacement for flap actuators (ex: -10 cm is 10 cm towards the floor): ");
    while (Serial.available() == 0) {}
    float userInput = Serial.parseFloat();
    userStruct.distanceReq = userInput;
    clearQuery();
    if (userInput != 0) {
      int requestedDirection = (userInput > 0) ? -1 : 1;
      userStruct.direction = requestedDirection;
      if (requestedDirection == 1) {
        stringDir = "DOWN towards floor";
      } else {
        stringDir = "UP towards ceiling";
      }
      Serial.print("Moving ");
      Serial.print(userInput);
      Serial.print(" ");
      Serial.println(stringDir);
      Serial.println("Sending to Nano 2...");
      esp_now_send(receiverAddress, (uint8_t *)&userStruct, sizeof(userStruct));
      delay(2000);  // gives some time for the nano to catch up.
    } else {
      clearQuery();  // clear before looping back.
      Serial.println("Invalid input. Enter nonzero value...ex: -10 cm for 10 cm to floor.");
      delay(200);
      continue;  // keep looping.
    }
    Serial.println("Continue? (Y/N)");
    while (Serial.available() == 0) {}
    String userCont = Serial.readStringUntil('\n');
    userCont.trim();
    clearQuery();

    if (userCont.equalsIgnoreCase("Y")) {
      resetStruct(userStruct);
      clearQuery();
    } else if (userCont.equalsIgnoreCase("N")) {
      clearQuery();
      Serial.println("Exiting...");
      break;
    } else {
      clearQuery();
      Serial.println("Invalid input. Please enter 'Y' or 'N'.");
      delay(200);
    }
  }
}

void moveFrontsOrAft() {  // here i will need to set the rear actuators in the mega code ! make a note!
  nanoToNano userStruct;
  String stringDir;
  String frontAftReq;

  while (true) {
    strncpy(userStruct.cmd, cmdList[3], sizeof(userStruct.cmd));
    userStruct.indivActuator = 0;
    userStruct.cmd[sizeof(userStruct.cmd) - 1] = '\0';  // Ensure null-termination
    Serial.println("Front or Rear Actuator Move? (F/R)");
    while (Serial.available() == 0) {}
    // Read user input
    frontAftReq = Serial.readStringUntil('\n');
    frontAftReq.trim();  // Remove any trailing whitespace or newline characters
    clearQuery();
    if (frontAftReq.equalsIgnoreCase("F")) {
      // userStruct.frontOrAft = "F";
      strncpy(userStruct.frontOrAft, "F", sizeof(userStruct.frontOrAft));
      userStruct.frontOrAft[sizeof(userStruct.frontOrAft) - 1] = '\0';

      Serial.print("Enter front displacement ex: -10 for 10 cm retraction: ");
      while (Serial.available() == 0) {}
      float userInput = Serial.parseFloat();
      clearQuery();
      if (userInput != 0 && abs(userInput) <= 23) {
        userStruct.distanceReq = userInput;
        int requestedDirection = (userInput > 0) ? -1 : 1;
        userStruct.direction = requestedDirection;
        if (requestedDirection == 1) {
          stringDir = "DOWN towards floor";
        } else {
          stringDir = "UP towards ceiling";
        }
        Serial.print("Moving ");
        Serial.print(userInput);
        Serial.print(" ");
        Serial.println(stringDir);
        Serial.println("Sending to Nano 2...");
        esp_now_send(receiverAddress, (uint8_t *)&userStruct, sizeof(userStruct));
        delay(2000);  // gives some time for the nano to catch up.        delay(2000);  // gives some time for the nano to catch up.
                      //   delay(200);
      } else {
        clearQuery();
        Serial.println("Invalid input. Enter nonzero value or smaller than 23 cm...ex: -10 cm for 10 cm to floor.");
        delay(200);
      }
    } else if (frontAftReq.equalsIgnoreCase("R")) {
      // userStruct.frontOrAft = "R";
      strncpy(userStruct.frontOrAft, "R", sizeof(userStruct.frontOrAft));
      userStruct.frontOrAft[sizeof(userStruct.frontOrAft) - 1] = '\0';
      Serial.print("Enter rear displacement ex: -10 for 10 cm retraction: ");
      while (Serial.available() == 0) {}
      float userInput = Serial.parseFloat();  // Parse the duration
      clearQuery();
      if (userInput != 0 && abs(userInput) <= 23) {
        userStruct.distanceReq = userInput;
        int requestedDirection = (userInput > 0) ? -1 : 1;
        userStruct.direction = requestedDirection;
        if (requestedDirection == 1) {
          stringDir = "DOWN towards floor";
        } else {
          stringDir = "UP towards ceiling";
        }
        Serial.print("Moving ");
        Serial.print(userInput);
        Serial.print(" ");
        Serial.println(stringDir);
        Serial.println("Sending to Nano 2...");
        esp_now_send(receiverAddress, (uint8_t *)&userStruct, sizeof(userStruct));
        delay(2000);  // gives some time for the nano to catch up. gives some time for the nano to catch up.
      } else {
        clearQuery();
        Serial.println("Invalid input. Enter nonzero value or smaller than 23 cm...ex: -10 cm for 10 cm to floor.");
        delay(200);
        continue;
      }
    } else {
      Serial.println("Invalid choice. Please enter 'F' or 'R'.");
      delay(200);
      continue;  // keeps looping.
    }
    Serial.println("Continue? (Y/N)");
    while (Serial.available() == 0) {}
    String userCont = Serial.readStringUntil('\n');
    userCont.trim();
    clearQuery();

    if (userCont.equalsIgnoreCase("Y")) {
      resetStruct(userStruct);
      clearQuery();
    } else if (userCont.equalsIgnoreCase("N")) {
      clearQuery();
      Serial.println("Exiting...");
      // don't want to reset the struct because we want the default values of the cmd in there.
      break;
    } else {
      clearQuery();
      Serial.println("Invalid input. Please enter 'Y' or 'N'.");
      delay(200);
    }
  }
}

void checkSensors() {
  nanoToNano userStruct;
  String sendCmdToMega;
  while (true) {
    strncpy(userStruct.cmd, cmdList[4], sizeof(userStruct.cmd));
    userStruct.cmd[sizeof(userStruct.cmd) - 1] = '\0';  // Ensure null-termination
    userStruct.indivActuator = 0;
    Serial.println("Sending to Nano 2...");
    esp_now_send(receiverAddress, (uint8_t *)&userStruct, sizeof(userStruct));
    esp_now_register_recv_cb(onReceive);  // should read in the sensor data and display
    // void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {

    delay(2000);  // gives some time for the nano to catch up.    delay(2000);  // gives some time for the nano to catch up.
    Serial.println("Continue? (Y/N)");
    while (Serial.available() == 0) {}
    String userCont = Serial.readStringUntil('\n');
    userCont.trim();
    clearQuery();

    if (userCont.equalsIgnoreCase("Y")) {
      resetStruct(userStruct);  // not really needed but jic for consistency.
      clearQuery();
    } else if (userCont.equalsIgnoreCase("N")) {
      clearQuery();
      Serial.println("Exiting...");
      // don't want to reset the struct because we want the default values of the cmd in there.
      break;
    } else {
      clearQuery();
      Serial.println("Invalid input. Please enter 'Y' or 'N'.");
      delay(200);
    }
  }
}

void moveIndivActuators() {
  nanoToNano userStruct;
  String stringDir;
  int userActuatorChoice;
  while (true) {
    Serial.println("Select actuator (1-6):");
    Serial.println("1. FL \n2. FR \n3. AL \n4. AR \n5. FlapL \n6. FlapR");
    while (Serial.available() == 0) {};
    userActuatorChoice = Serial.parseInt();
    Serial.print("Selected: ");
    Serial.println(userActuatorChoice);
    if (userActuatorChoice >= 1 && userActuatorChoice <= 6) {
      //break;  // Valid input, exit loop
    } else {
      clearQuery();
      Serial.println("Error. Enter 1-6 for the listed actuators.");
      delay(200);
      // Loop continues, prompting again
    }
    clearQuery();
    switch (userActuatorChoice) {
      case 1:
        moveActuator("FL", 1);
        break;
      case 2:
        // FR actuator
        moveActuator("FR", 2);
        break;
      case 3:
        // AL actuator
        moveActuator("AL", 3);
        break;
      case 4:
        // AR actuator
        moveActuator("AR", 4);
        break;
      case 5:
        // FA actuator
        moveActuator("FLAPL", 5);
        break;
      case 6:
        // RA actuator
        moveActuator("FLAPR", 6);
        break;
      default:
        // This should not happen if input validation is correct
        break;
    }
    Serial.println("Continue? (Y/N)");
    while (Serial.available() == 0) {}
    String userCont = Serial.readStringUntil('\n');
    userCont.trim();
    clearQuery();
    if (userCont.equalsIgnoreCase("Y")) {
      resetStruct(userStruct);
      clearQuery();
    } else if (userCont.equalsIgnoreCase("N")) {
      clearQuery();
      Serial.println("Exiting...");
      break;
    } else {
      clearQuery();
      Serial.println("Invalid input. Please enter 'Y' or 'N'.");
      delay(200);
    }
  }
}
void moveActuator(const String &actuatorName, int actuatorIndex) {
  nanoToNano userStruct;
  String stringDir;
  String sendCmdToMega;



  while (true) {
    strncpy(userStruct.cmd, cmdList[5], sizeof(userStruct.cmd));
    userStruct.cmd[sizeof(userStruct.cmd) - 1] = '\0';  // Ensure null-termination
    userStruct.indivActuator = actuatorIndex;  // or use actuatorName if needed
    Serial.print("Enter displacement for ");
    Serial.print(actuatorName);
    Serial.println(" (ex: -10 for 10 cm towards floor): ");
    while (Serial.available() == 0) {}
    float userInput = Serial.parseFloat();
    clearQuery();
    if (userInput != 0 && abs(userInput) <= 23) {
      userStruct.distanceReq = userInput;
      int requestedDirection = (userInput > 0) ? -1 : 1;
      userStruct.direction = requestedDirection;
      stringDir = (requestedDirection == 1) ? "DOWN towards floor" : "UP towards ceiling";
      Serial.print("Moving ");
      Serial.print(userInput);
      Serial.print(" ");
      Serial.println(stringDir);
      Serial.println("Sending to Nano 2...");
      // sendCmdToMega = userStruct.cmd + "," + String(userStruct.direction) + "," + String(userStruct.distanceReq) + "," + String(userStruct.frontOrAft) + "," + String(userStruct.indivActuator);
      Serial.println("Struct being sent:");
      Serial.print("cmd: ");
      Serial.println(userStruct.cmd);
      Serial.print("frontOrAft: ");
      Serial.println(userStruct.frontOrAft);  // If you set this elsewhere
      Serial.print("direction: ");
      Serial.println(userStruct.direction);
      Serial.print("distanceReq: ");
      Serial.println(userStruct.distanceReq, 4);  // 4 decimal places
      Serial.print("indivActuator: ");
      Serial.println(userStruct.indivActuator);
      Serial.println("----------------------");
      // ----------------------------------

      esp_now_send(receiverAddress, (uint8_t *)&userStruct, sizeof(userStruct));
      delay(2000);  // gives some time for the nano to catch up.      delay(2000);  // gives some time for the nano to catch up.
      // Serial1.println(sendCmdToMega);
      break;  // Exit after successful move
    } else {
      Serial.println("Invalid input. Enter nonzero value up to 23 cm.");
      delay(200);
    }
  }
}
void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.print("Sensor data from Nano 2: ");
  Serial.println((const char *)incomingData);
}


void resetStruct(nanoToNano &val) {
  val.cmd[0] = '\0';
  val.direction = 0;
  val.distanceReq = 0.0;
  val.indivActuator = 0;
  val.frontOrAft[0] = '\0';
}

void clearQuery() {

  while (Serial.available() > 0) {
    Serial.read();  // Clear buffer
  }
}