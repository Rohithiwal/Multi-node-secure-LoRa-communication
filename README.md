# Secure LoRa Telemetry Network

This project implements a secure telemetry system using NodeMCU ESP8266 boards and LoRa E22-900T22S modules. It features encrypted communication using AES, TDMA-based transmission scheduling, and cloud integration via MQTT and ThingsBoard.

## Architecture Overview

- **2 Transmitter Nodes**: Each collects mock sensor data, encrypts it using AES-128, and sends it via LoRa using a time-slot (TDMA) schedule.
- **1 Gateway Node**: Receives encrypted LoRa packets, decrypts them, verifies the sender’s DevEUI, and uploads the data to ThingsBoard via MQTT.
- **Communication Type**: Point-to-point fixed addressing with LoRa E22 modules (configured via `sendFixedMessage()`).
- **Security**: AES-128 encryption with IV and a whitelist of authorized DevEUIs.
- **Cloud**: Data is published to [ThingsBoard](https://thingsboard.io) over Wi-Fi using the MQTT protocol.

## Features

- AES-128 encrypted LoRa data transmission
- TDMA (Time Division Multiple Access) slot scheduling
- DevEUI-based device authentication
- Gateway uplinks telemetry to ThingsBoard Cloud
- RSSI reporting and connection monitoring
- Modular design with EEPROM support for persistent Node IDs

## Hardware Used

- NodeMCU ESP8266 (3 units)
- LoRa E22-900T22S modules
- 5V battery

## Project Structure

```
/project-root
├── node1/         # Code for first sensor node
├── node2/         # Code for second sensor node
├── gateway/       # Code for LoRa-to-Cloud gateway
└── README.md
```

## How It Works

1. Each sensor node gets a fixed slot in a 6-second TDMA cycle (2 nodes × 2s slots).
2. Nodes read sensor values (mocked), format them into JSON with DevEUI, encrypt using AES, and transmit over LoRa.
3. Gateway receives LoRa messages, decrypts them, checks the DevEUI against an allowed list, and publishes valid data to ThingsBoard.
4. The ThingsBoard dashboard can be used to view the live telemetry.

## Setup Instructions

### Transmitter Nodes
- Write the `NODE_ID` to EEPROM (0 or 1 for different nodes).
- Upload the respective code (`node1` or `node2`).
- Replace mock sensor functions with real sensor logic as needed.

### Gateway Node
- Replace `ssid`, `password`, and `tbToken` with your Wi-Fi and ThingsBoard credentials.
- Upload the code.
- Ensure `allowedDevEUIs[]` includes the DevEUIs used in the sensor nodes.

### DevEUI Setup
- DevEUIs are stored in EEPROM. You can set them manually using a small EEPROM write sketch.

### Power
- Nodes can be powered using 3.7V Li-ion batteries with a boost converter outputting 5V to NodeMCU `Vin`.
- Gateway can be powered via USB or regulated 5V.

## Dependencies

- [LoRa_E22](https://github.com/xreef/LoRa_E22)
- [AESLib](https://github.com/DavyLandman/AESLib)
- [ESP8266WiFi](https://github.com/esp8266/Arduino)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [NTPClient](https://github.com/arduino-libraries/NTPClient)

## Example JSON Payload

```
{
  "devEUI": "0123456789ABCDEF",
  "Fe": 2.7,
  "C": 3.8,
  "No2": 7.2
}
```

## To Do

- Replace mock sensor data with real environmental or industrial sensors
- Add OTA updates or config syncing
- Extend to support more nodes using dynamic TDMA slots
- Implement downlink commands via MQTT → LoRa

## License

This project is open-source under the MIT License.
