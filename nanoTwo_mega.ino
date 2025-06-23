#include <WiFi.h>
#include <esp_now.h>
uint8_t nano1Address[] = { 0x74, 0x4D, 0xBD, 0xA0, 0xAC, 0xB0 };  // Nano connected to the workstation

// for comms to the Mega
#define RX_PIN 3
#define TX_PIN 2
// bool handshakeSuccess = false;
int maxRetries = 5;
int attempt = 0;

static bool handshakeAttempted = false;
static bool handshakeSuccess = false;
char msg[64];
const unsigned long timeout = 5000;  // 5 seconds timeout
struct nanoToMega {
  char cmd[16];
  char frontOrAft[2];
  int direction;
  float distanceReq;
  int indivActuator;
};

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");  // does receiver get package
}
void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  nanoToMega received;  // the struct
  memcpy(&received, incomingData, sizeof(received));
  Serial.println("Received struct:");
  Serial.print("cmd: ");
  Serial.println(received.cmd);
  Serial.print("frontOrAft: ");
  Serial.println(received.frontOrAft);
  Serial.print("direction: ");
  Serial.println(received.direction);
  Serial.print("distanceReq: ");
  Serial.println(received.distanceReq, 4);  // 4 decimal places
  Serial.print("indivActuator: ");
  Serial.println(received.indivActuator);
  // Concatenate fields into a command string
  char sendCmdToMega[64];
  snprintf(sendCmdToMega, sizeof(sendCmdToMega), "%s,%d,%.2f,%s,%d",
           received.cmd,
           received.direction,
           received.distanceReq,
           received.frontOrAft,
           received.indivActuator);

  // Send to Mega
  Serial1.println(sendCmdToMega);
  Serial.print("Forwarded to Mega: ");
  Serial.println(sendCmdToMega);
  if (strcmp(received.cmd, "SENSOR_CHECK") == 0) {
    // Wait for Mega's reply (example: up to 1 second)
    unsigned long start = millis();
    String megaReply = "";
    while (millis() - start < 1000) {
      if (Serial1.available()) {
        megaReply = Serial1.readStringUntil('\n');
        break;
      }
    }
    // Send Mega's reply back to Nano 1 via ESP-NOW
    if (megaReply.length() > 0) {
      esp_now_send(nano1Address, (uint8_t *)megaReply.c_str(), megaReply.length() + 1);
    }
  }
  while (Serial1.available() > 0) {
    Serial1.read();  // clear after sending.
  }
}
// For debugging:


void setup() {
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(1000);
  WiFi.mode(WIFI_STA);
  Serial.println("ESP-NOW Nano 2 Ready...");
  if (esp_now_init() != ESP_OK) {  // initializes.
    Serial.println("ESP-NOW init failed");
    while (1)
      ;
  }
  esp_now_register_send_cb(onSent);  // call back function to see if message was successful
  esp_now_register_recv_cb(onReceive);
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, nano1Address, 6);  // 6 for the mac address length
  peerInfo.channel = 0;
  peerInfo.encrypt = false;  // for now
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    while (1)
      ;
  }
}

void loop() {

  //Serial.println("Now attempting to establish connection w Mega.");
  if (!handshakeAttempted) {
    strcpy(msg, "Checking Nano 2 and Mega cxn...");
    esp_now_send(nano1Address, (uint8_t *)msg, strlen(msg) + 1);
    Serial1.println("Ping!");
    unsigned long start = millis();
    memset(msg, 0, sizeof(msg));
    while (millis() - start < timeout) {
      if (Serial1.available()) {
        String incoming = Serial1.readStringUntil('\n');
        Serial.print("Received from Mega: ");
        Serial.println(incoming);
        if (incoming.indexOf("Pong!") >= 0) {
          Serial.println("Handshake complete.");
          esp_now_send(nano1Address, (uint8_t *)msg, strlen(msg) + 1);
          memset(msg, 0, sizeof(msg));
          handshakeSuccess = true;
          while (Serial1.available() > 0) Serial1.read();
          break;
        }
      }
      delay(100);
    }
    handshakeAttempted = true;
  }

  if (handshakeSuccess) {
    strcpy(msg, "Nano and Mega cxn established");
    esp_now_send(nano1Address, (uint8_t *)msg, strlen(msg) + 1);

    // Now wait for ESP-NOW messages from Nano 1, handled in onReceive()
    // No need to call onReceive() manually!
  }

  delay(1000);  // Prevent busy looping
}
