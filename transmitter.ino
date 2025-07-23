// to be uploaded in mid one 

#include "Arduino.h"
#include "LoRa_E22.h"
#include <AESLib.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// ---------------------- USER CONFIG ------------------------
#define NODE_ID            1       
#define DESTINATION_ADDL   1
#define CHANNEL            17
#define SLOT_DURATION_MS   2000      // 2 sec per slot
#define TDMA_CYCLE_MS      6000      // Total cycle = 2 nodes
#define WIFI_SSID          "Your SSID"
#define WIFI_PASS          "Password"

// -------------------- AES Setup ----------------------------
AESLib aesLib;
byte aes_key[] = {
  0x87, 0x1F, 0x3C, 0x46, 0x2A, 0xB0, 0x90, 0xEE,
  0x54, 0x75, 0x61, 0x97, 0xCC, 0x21, 0xD3, 0x42
};
byte aes_iv[N_BLOCK] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

// ------------------- LoRa & EEPROM -------------------------
LoRa_E22 e22ttl(D3, D4);  
String inputMessage;

// -------------------- WiFi & NTP ---------------------------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);  // +5:30 offset (IST)

// -------------------- Timing -------------------------------
unsigned long lastSendAttempt = 0;
const unsigned long SEND_INTERVAL_MS = 500;  // Check every 500ms inside slot

// -------------------- DevEUI EEPROM ------------------------
String getDevEUIFromEEPROM() {
  byte devEUI[8];
  String devEUIstr = "";
  for (int i = 0; i < 8; i++) {
    devEUI[i] = EEPROM.read(i);
    if (devEUI[i] < 0x10) devEUIstr += "0";
    devEUIstr += String(devEUI[i], HEX);
  }
  devEUIstr.toUpperCase();
  return devEUIstr;
}

// --------------------- Sensor Mocks ------------------------
float readIron()  { return 2.7; }
float readCarbon()      { return 3.8; }
float readNitrate()  { return 7.2; }

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  delay(500);
  e22ttl.begin();

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  timeClient.begin();
  while (!timeClient.update()) timeClient.forceUpdate();

  Serial.println("\nTDMA AES Sender Ready");
  Serial.print("DevEUI: "); Serial.println(getDevEUIFromEEPROM());
}

void loop() {
  timeClient.update();

  unsigned long epochMillis = timeClient.getEpochTime() * 1000 + (millis() % 1000); // Rough ms estimate
  unsigned long cycleTime = epochMillis % TDMA_CYCLE_MS;
  unsigned long nodeStart = NODE_ID * SLOT_DURATION_MS;
  unsigned long nodeEnd = nodeStart + SLOT_DURATION_MS;

  if (cycleTime >= nodeStart && cycleTime < nodeEnd) {
    if (millis() - lastSendAttempt > SEND_INTERVAL_MS) {
      lastSendAttempt = millis();

      // Compose data
      String devEUI = getDevEUIFromEEPROM();
      inputMessage = "{\"devEUI\":\"" + devEUI + "\"," +
                     "\"Fe\":" + String(readIron(), 1) +
                     ",\"C\":" + String(readCarbon(), 2) +
                     ",\"No2\":" + String(readNitrate(), 1) + "}";

      Serial.print("RAW : ");
      Serial.println(inputMessage);

      // Encrypt
      byte encrypted[128];
      int cipherLength = aesLib.encrypt(
        (const byte*)inputMessage.c_str(),
        inputMessage.length(),
        encrypted,
        aes_key, sizeof(aes_key), aes_iv
      );

      if (cipherLength <= 0) {
        Serial.println("Encryption failed");
        return;
      }

      Serial.print("AES : ");
      for (int i = 0; i < cipherLength; i++) {
        Serial.printf("%02X", encrypted[i]);
      }
      Serial.println();

      // Send via LoRa
      ResponseStatus rs = e22ttl.sendFixedMessage(0, DESTINATION_ADDL, CHANNEL, encrypted, cipherLength);
      Serial.printf("TX  : %s (code %d)\n", rs.getResponseDescription(), rs.code);
    }
  }

  delay(100);  // Check every 100ms
}

// ---------------------------------------------------------------------Below code is for writing the NodeID in EPROM of the Node---------------------------------------------------------------------------------------------
// #include <EEPROM.h>

// void setup() {
//   Serial.begin(9600);
//   EEPROM.begin(512);

//   // Set this to 0,1, etc. based on number of nodes you have
//   byte nodeId = 1;

//   EEPROM.write(8, nodeId);
//   EEPROM.commit();

//   Serial.print("Node ID ");
//   Serial.print(nodeId);
//   Serial.println(" written to EEPROM.");
// }

// void loop() {}
