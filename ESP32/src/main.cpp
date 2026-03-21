#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ModbusMaster.h>
#include "ota.h"
#include "logger.h"
#include "webui.h"

#define WIFI_SSID "DvorNet"
#define WIFI_PASS "dvor62tuc"

// --- KLASICKÉ ZAPOJENÍ ---
#define MODBUS_RX_PIN 16
#define MODBUS_TX_PIN 17
#define DE_RE_PIN 18 

#define SLAVE_ID 247
#define REG_BATTERY_POWER 30258
#define REG_SOC           33000
#define REG_BATTERY_I     30255
#define REG_GRID_I        11000

float battery_P = 0, battery_soc = 0, battery_I = 0, grid_I = 0;
String status_msg = "Klasicke reseni...";

ModbusMaster node;
WebSocketsServer webSocket(81);
WebServer server(80);
String logBuffer = "";

// Nektere menice potrebuji malou prodlevu po prepnuti DE/RE
void preTransmission() { 
    digitalWrite(DE_RE_PIN, HIGH); 
    delayMicroseconds(500); 
}
void postTransmission() { 
    delayMicroseconds(500);
    digitalWrite(DE_RE_PIN, LOW); 
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_CONNECTED) {
        webSocket.sendTXT(num, logBuffer);
    }
}

template<typename T>
bool readRegister(uint16_t reg_addr, uint8_t reg_count, T &value, const char* name)
{
    uint8_t res = node.readHoldingRegisters(reg_addr, reg_count);
    if (res == node.ku8MBSuccess)
    {
        if (reg_count == 1)
            value = (T)node.getResponseBuffer(0);
        else if (reg_count == 2)
            value = (T)(((uint32_t)node.getResponseBuffer(0) << 16) | node.getResponseBuffer(1));
        return true;
    }
    webLog("Chyba " + String(name) + ": 0x" + String(res, HEX));
    value = 0;
    return false;
}

bool readBatteryData()
{
    bool ok = true;
    
    // Zkusíme číst první blok 30258-30263 (6 registrů: Výkon, Proud aku, Proud sítě)
    // Toto je efektivnější než 3 samostatné dotazy
    uint8_t res = node.readHoldingRegisters(REG_BATTERY_POWER, 6);
    if (res == node.ku8MBSuccess) {
        int32_t raw_P = (int32_t)(((uint32_t)node.getResponseBuffer(0) << 16) | node.getResponseBuffer(1));
        battery_P = raw_P / 1000.0;
        
        int16_t rawI_bat = (int16_t)node.getResponseBuffer(2);
        battery_I = (rawI_bat / 10.0) * -1.0;
        
        int32_t raw_Grid = (int32_t)(((uint32_t)node.getResponseBuffer(4) << 16) | node.getResponseBuffer(5));
        grid_I = (raw_Grid / 1000.0) * -1.0;
    } else {
        webLog("Chyba bloku 30k: 0x" + String(res, HEX));
        // Pokud blok selže, zkusíme aspoň SOC
        ok = false; 
    }

    delay(100); // Krátká pauza pro střídač před dalším dotazem

    uint16_t rawSOC = 0;
    if (readRegister<uint16_t>(REG_SOC, 1, rawSOC, "SOC"))
        battery_soc = rawSOC / 100.0;
    else ok = false;

    return ok;
}

void handleRoot() {
  server.send(200, "text/html", WebUI::getDashboardHTML(battery_P, battery_I, grid_I, battery_soc, status_msg, logBuffer));
}

void setup() {
    Serial.begin(115200);

    // Standardni Serial2 konfigurace
    Serial2.begin(9600, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);

    pinMode(DE_RE_PIN, OUTPUT);
    digitalWrite(DE_RE_PIN, LOW);
  
    node.begin(SLAVE_ID, Serial2);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    server.on("/", handleRoot);
    server.begin();

    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);

    webLog("Standardní start (Serial2, Piny 16/17, DE:4)");
}

void loop() {
    static unsigned long lastOTA = millis();
    static unsigned long lastRead = 0;

    server.handleClient();
    webSocket.loop();

    // OTA kontrola az po par minutach
    if (millis() > 300000 && (millis() - lastOTA > 3600000)) {
        lastOTA = millis(); 
        OTA::check();
    }

    if (millis() - lastRead > 3000) {
        lastRead = millis();
        
        webLog("Zkouším číst měnič...");

        if (readBatteryData()) {
            status_msg = "SPOJENO OK";
            webLog(">>> Modbus komunikace OK!");
            webLog("P: " + String(battery_P) + " kW, SOC: " + String(battery_soc) + " %, I: " + String(battery_I) + " A, Grid: " + String(grid_I) + " A");
        } else {
            status_msg = "Chyba komunikace";
            webLog("Neodpovídá. Zkontrolovat zapojeni.");
        }
    }
}