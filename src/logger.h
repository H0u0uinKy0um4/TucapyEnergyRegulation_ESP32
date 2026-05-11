#pragma once
#include <Arduino.h>
#include <time.h>
#define MAX_LOG_SIZE 2048

extern String logBuffer;



class Time
{
    public:
        static Time& getInstance()
        {
            static Time instance;
            return instance;
        }   
        void begin()
        {
            configTime(3600, 3600, "pool.ntp.org", "time.google.com");
            tm time_check;
            
            Serial.println("Setting up time...");
            unsigned long startWait = millis();
            while (!getLocalTime(&time_check) && (millis() - startWait < 10000))
            {
                delay(500);
                Serial.print(".");
            }

            if (getLocalTime(&time_check)) {
                Serial.println("\nTime was setup.");
            } else {
                Serial.println("\nTime setup failed (timeout).");
            }
        }

        bool update() {
        return getLocalTime(&time);
        }

        int hour()const{return time.tm_hour;}
        int day()const{return time.tm_mday;}
        int min()const{return time.tm_min;}
        int sec()const{return time.tm_sec;}

        String convert()
        {
            char buf[9];
            sprintf(buf, "%02d:%02d:%02d", time.tm_hour, time.tm_min,time.tm_sec);
            return String(buf);
        }

    private:
    tm time = {};
};

#define TIME Time::getInstance()



inline void webLog(String msg, bool toSerial = false) {
    TIME.update();
    String logEntry = "[" + TIME.convert() + "s] " + msg;
    if (toSerial) {
        Serial.println(logEntry);
    }
    
    logBuffer += logEntry + "\n";
    if (logBuffer.length() > MAX_LOG_SIZE) {
        logBuffer = logBuffer.substring(logBuffer.length() - MAX_LOG_SIZE);
    }
}



