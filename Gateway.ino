#include "Arduino.h"
#include "LoRa_E22.h"
#include <AESLib.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// --------------------- WiFi & ThingsBoard Config ---------------------
const char* ssid = "Nothing Phone";
const char* password = "1919216153";

const char* tbServer = "demo.thingsboard.io";
const int tbPort = 1883;
const char* tbToken = "FqqxKixlaCMR6OITz8Fc";  // Replace with actual token

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// --------------------- LoRa Config ---------------------
#define MY_ADDH     0x00
#define MY_ADDL     0x01
#define MY_CHANNEL  17

LoRa_E22 e22ttl(D3, D4);

// --------------------- AES Parameters ---------------------
AESLib aesLib;

byte aes_key[] = {
  0x87, 0x1F, 0x3C, 0x46, 0x2A, 0xB0, 0x90, 0xEE,
  0x54, 0x75, 0x61, 0x97, 0xCC, 0x21, 0xD3, 0x42
};

byte aes_iv[N_BLOCK] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

// --------------------- Authorized DevEUIs ---------------------
const char* allowedDevEUIs[] = {
  "0000FFFECE80E001",
  "0000FFFE98EE4901"
};
const int numAllowed = sizeof(allowedDevEUIs) / sizeof(allowedDevEUIs[0]);

// --------------------- Signal Timeout Tracking ---------------------
unsigned long lastPacketTime = 0;
const unsigned long SIGNAL_TIMEOUT = 10000; // 10 seconds
bool signalLostReported = false;

// --------------------- Helper: Bytes to HEX String ---------------------
String bytesToHex(byte* data, int len) {
  String hex = "";
  for (int i = 0; i < len; i++) {
    if (data[i] < 0x10) hex += "0";
    hex += String(data[i], HEX);
  }
  return hex;
}

// --------------------- DevEUI Authorization ---------------------
bool isAuthorizedDevEUI(String payload) {
  for (int i = 0; i < numAllowed; i++) {
    if (payload.indexOf(allowedDevEUIs[i]) != -1) {
      return true;
    }
  }
  return false;
}

// --------------------- WiFi & MQTT Setup ---------------------
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
}

void connectMQTT() {
  mqttClient.setServer(tbServer, tbPort);
  while (!mqttClient.connected()) {
    Serial.print("Connecting to ThingsBoard...");
    if (mqttClient.connect("LoRaReceiver", tbToken, nullptr)) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 2s");
      delay(2000);
    }
  }
}

// --------------------- Setup ---------------------
void setup() {
  Serial.begin(9600);
  delay(500);
  e22ttl.begin();

  Serial.println(F("\n=== LoRa E22 RECEIVER ready ==="));
  Serial.print(F("Channel : ")); Serial.println(850 + MY_CHANNEL);

  connectWiFi();
  connectMQTT();
}

// --------------------- Main Loop ---------------------
void loop() {
  if (e22ttl.available() > 1) {
    ResponseContainer rc = e22ttl.receiveMessageRSSI();

    if (rc.status.code != 1) {
      Serial.println(rc.status.getResponseDescription());
      return;
    }

    lastPacketTime = millis();         // Reset watchdog
    signalLostReported = false;        // Clear flag

    int len = rc.data.length();
    byte encrypted[len];
    memcpy(encrypted, rc.data.c_str(), len);

    Serial.print("RX HEX : ");
    Serial.println(bytesToHex(encrypted, len));

    char decrypted[128];
    int decryptedLen = aesLib.decrypt(
      encrypted, len,
      (byte*)decrypted,
      aes_key, sizeof(aes_key), aes_iv
    );

    // Trim garbage characters
    int i;
    for (i = 0; i < decryptedLen; i++) {
      if (decrypted[i] < 32 || decrypted[i] > 126) break;
    }
    decrypted[i] = '\0';

    String decryptedStr = String(decrypted);
    Serial.print("Decrypted: ");
    Serial.println(decryptedStr);

    if (!isAuthorizedDevEUI(decryptedStr)) {
      Serial.println("UNAUTHORIZED device! Message ignored.");
      return;
    }

    Serial.println("AUTHORIZED device. Message accepted.");

    // Send to ThingsBoard
    String payload = decryptedStr;
    mqttClient.publish("v1/devices/me/telemetry", payload.c_str());
    Serial.println("Data sent to ThingsBoard.");

    // RSSI
    int8_t rssiDbm = static_cast<int8_t>(rc.rssi);
    Serial.printf("RSSI: %d dBm (raw %u)\n", rssiDbm, rc.rssi);
    Serial.println("----------------------------------");
  }

  // if (!signalLostReported && millis() - lastPacketTime > SIGNAL_TIMEOUT) {
  //   Serial.println("No signal received for over 10 seconds.");
  //   signalLostReported = true;
  // }

  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
}
