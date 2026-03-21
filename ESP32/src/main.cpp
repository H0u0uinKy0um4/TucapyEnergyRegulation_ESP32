#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "ota.h"
#include "logger.h"
#include "webui.h"
#include "modbus_handler.h"

#define WIFI_SSID "DvorNet"
#define WIFI_PASS "dvor62tuc"
#define FIRMWARE_VERSION "2026-03-21 21:55"

// --- OVLÁDÁNÍ VÝSTUPŮ ---
#define OUT1 19
#define OUT2 21
#define OUT3 22
#define OUT4 23
#define OUT5 25
#define OUT6 26
#define OUT7 27
#define OUT8 32
const int outputs[] = {OUT1, OUT2, OUT3, OUT4, OUT5, OUT6, OUT7, OUT8};
const int n_outputs=8;
#define HORNI_PROUD 2
#define SPODNI_PROUD 0  

#define HORNI_SOC 80
#define SPODNI_SOC 60

bool power_mode=false;
int idx=0;

WebSocketsServer webSocket(81);
WebServer server(80);
String logBuffer = "";

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_CONNECTED) {
        webSocket.sendTXT(num, logBuffer);
    }
}

void handleRoot() {
  server.send(200, "text/html", WebUI::getDashboardHTML(
    ModbusHandler::battery_P, 
    ModbusHandler::battery_I, 
    ModbusHandler::grid_I, 
    ModbusHandler::battery_soc, 
    ModbusHandler::status_msg, 
    logBuffer, 
    FIRMWARE_VERSION));
}

void turn_on()
{
    if(idx<n_outputs)
    {
        digitalWrite(outputs[idx],HIGH);
        idx++;
    }
    return;
}
void turn_off()
{

    if (idx>0)idx--;
    else return;
    digitalWrite(outputs[idx],LOW);
    return;
}


void setup() {
    Serial.begin(115200);
    

    // Inicializace Modbus
    ModbusHandler::setup();
    
    // Inicializace výstupů
    for (int pin : outputs) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    server.on("/", handleRoot);
    server.begin();

    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);

    webLog("Start systemu - verze " + String(FIRMWARE_VERSION));
}

void loop() {
    static unsigned long lastOTA = millis();
    server.handleClient();
    webSocket.loop();

    // OTA kontrola az po par minutach
    if (millis() > 60000 && (millis() - lastOTA > 60000)) {
        lastOTA = millis(); 
        OTA::check();
    }

    // Čtení dat z měniče
    if(ModbusHandler::update())
    {
        if(ModbusHandler::battery_soc<SPODNI_SOC)power_mode=false;
        if(ModbusHandler::battery_soc>=HORNI_SOC)power_mode=true;
        if(power_mode && (ModbusHandler::battery_I>=HORNI_PROUD))turn_on();
        if(power_mode && (ModbusHandler::battery_I<=SPODNI_PROUD))turn_off();
    }
    
}