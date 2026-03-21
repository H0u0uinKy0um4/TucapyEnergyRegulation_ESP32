#include <Arduino.h>
#include <WiFi.h>
#include "ota.h"

#define WIFI_SSID "DvorNet"
#define WIFI_PASS "dvor62tuc"

void setup() {
    Serial.begin(115200);
    delay(1000);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Pripojuji WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi OK: " + WiFi.localIP().toString());

    OTA::check();
}

void loop() {
    // tvůj kód zde...

    OTA::check();
    delay(2000); // kontroluj každých 60s
}