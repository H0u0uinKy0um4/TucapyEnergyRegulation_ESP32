#pragma once
#include <PubSubClient.h>
#include <WiFi.h>


#define WIFI_SSID "DvorNet"
#define WIFI_PASS "dvor62tuc"

//Server HA
const char* mqtt_server = "192.168.1.123";

WiFiClient espClient;
PubSubClient client(espClient);
const char* mqtt_user = "daja";
const char* mqtt_pass = "Hbsht1620";

namespace Protocols{

void maintainMQTT() {
  if (!client.connected()) {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (client.connect("esp32_bme", mqtt_user, mqtt_pass)) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }
}


void connectWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int att = 0;
    while (WiFi.status() != WL_CONNECTED && att < 30) {
        delay(500);
        Serial.print(".");
        att++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi pripojeno!");
        Serial.print("IP adresa: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi se NEPRIPOJILO vyprsel timeout!");
    }
}


void pushMqtt(const char* dir, float val)
{
    if (!client.connected()) return;

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%.3f", val);
    bool ok = client.publish(dir, buffer);
    if (!ok) {
        webLog("Mqtt error!\n",false);
    }
} 

void setup() {
  connectWiFi();
  client.setServer(mqtt_server, 1883);
}

}