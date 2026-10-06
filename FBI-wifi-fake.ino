#include <WiFi.h>
#include "esp_wifi.h"

// List of fake network names (SSIDs) to broadcast
const char* ssids[] = {
  "FBI Surveillance Van #9",
  "FBI Surveillance Drone",
  "AIVD Tapwagentje 04",
  "Do Not Connect",
  "Free Open Wi-Fi (No Virus)",
  "Give My Wi-Fi Back"
};

const int numSsids = sizeof(ssids) / sizeof(ssids[0]);

// Standard empty MAC header for Beacon frames
uint8_t packet[128] = {
  0x80, 0x00, // Frame Control
  0x00, 0x00, // Duration
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination (Broadcast)
  0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, // Source MAC (will be filled dynamically)
  0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, // BSSID (will be filled dynamically)
  0x00, 0x00, // Sequence number
  
  // -- Fixed parameters --
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
  0x64, 0x00, // Interval
  0x01, 0x04  // Capability info
};

void setup() {
  Serial.begin(115200);
  
  // Set WiFi to Station mode and disconnect before switching to AP mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_start();
  
  Serial.println("ESP32 FBI Fake Wi-Fi Spammer started by Ruben Sheep 🐑!");
}

void loop() {
  // Broadcast beacon frames for each SSID in the list
  for (int i = 0; i < numSsids; i++) {
    // Generate a random MAC address for each fake network
    uint8_t mac[6];
    for (int j = 0; j < 6; j++) {
      mac[j] = random(0, 256);
    }
    
    // Copy MAC address into packet template
    memcpy(&packet[10], mac, 6);
    memcpy(&packet[16], mac, 6);

    int ssidLen = strlen(ssids[i]);
    int pktLength = 37 + ssidLen; // 37 bytes header + SSID length + tags

    // Build tagged parameters (SSID tag)
    packet[36] = 0x00; // Tag number: SSID
    packet[37] = ssidLen; // Tag length
    memcpy(&packet[38], ssids[i], ssidLen);

    // Add necessary Wi-Fi management tags (Supported Rates)
    int pos = 38 + ssidLen;
    packet[pos++] = 0x01; packet[pos++] = 0x08; // Supported rates
    packet[pos++] = 0x82; packet[pos++] = 0x84; packet[pos++] = 0x8b; packet[pos++] = 0x96;
    packet[pos++] = 0x24; packet[pos++] = 0x30; packet[pos++] = 0x48; packet[pos++] = 0x6c;

    // Transmit the packet multiple times on Wi-Fi channel 6 (met extra 'false' argument voor de nieuwe SDK)
    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
    
    for (int k = 0; k < 3; k++) {
      esp_wifi_80211_tx(WIFI_IF_AP, packet, pktLength + 8, false);
      delay(2);
    }
  }
  
  // Brief pause between rounds
  delay(100);
}