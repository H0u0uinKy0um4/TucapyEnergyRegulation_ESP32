#pragma once
#include <Arduino.h>
#include <ModbusMaster.h>

namespace ModbusHandler {

// Konfigurace pinů
#define MODBUS_RX_PIN 16
#define MODBUS_TX_PIN 17
#define DE_RE_PIN 18 

// Registry
#define SLAVE_ID 247
#define REG_BATTERY_POWER 30258
#define REG_SOC           33000

// Data střídače (exportovaná pro WebUI)
extern float battery_P;
extern float battery_soc;
extern float battery_I;
extern float grid_I;
extern String status_msg;

// Funkce
void setup();
bool update(); // Volá se v loop()

} // namespace ModbusHandler
