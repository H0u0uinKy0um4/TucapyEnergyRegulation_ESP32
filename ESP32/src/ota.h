#pragma once
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>

#define VERSION_URL  "https://raw.githubusercontent.com/FredericBastitat/TucapyEnergyRegulation/main/version.txt"
#define FIRMWARE_URL "https://raw.githubusercontent.com/FredericBastitat/TucapyEnergyRegulation/main/firmware.bin"

namespace OTA {

Preferences prefs;

String getCurrentSHA() {
    prefs.begin("ota", true);
    String sha = prefs.getString("sha", "");
    prefs.end();
    return sha;
}

void saveSHA(String sha) {
    prefs.begin("ota", false);
    prefs.putString("sha", sha);
    prefs.end();
}

String fetchString(String url) {
    HTTPClient http;
    http.begin(url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int code = http.GET();
    Serial.printf("HTTP GET %s -> %d\n", url.c_str(), code);
    String result = "";
    if (code == 200) {
        result = http.getString();
        result.trim();
    } else {
        Serial.printf("Chyba: %s\n", http.errorToString(code).c_str());
    }
    http.end();
    return result;
}

void check() {
    Serial.println("Kontroluji verzi...");
    String latestSHA = fetchString(VERSION_URL);
    if (latestSHA.isEmpty()) {
        Serial.println("Nepodarilo se ziskat verzi.");
        return;
    }

    String currentSHA = getCurrentSHA();
    Serial.println("Ulozena SHA:  " + currentSHA);
    Serial.println("GitHub SHA:   " + latestSHA);

    if (latestSHA == currentSHA) {
        Serial.println("✓ Firmware je aktualni.");
        return;
    }

    Serial.println("Novy firmware nalezen, stahuji...");
    HTTPClient http;
    http.begin(FIRMWARE_URL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (http.GET() == 200) {
        int size = http.getSize();
        if (size <= 0) size = UPDATE_SIZE_UNKNOWN;
        WiFiClient* stream = http.getStreamPtr();

        if (Update.begin(size)) {
            Update.writeStream(*stream);
            if (Update.end(true)) {
                saveSHA(latestSHA);
                Serial.println("Update OK, restartuji...");
                ESP.restart();
            } else {
                Update.printError(Serial);
            }
        } else {
            Update.printError(Serial);
        }
    }
    http.end();
}

} // namespace OTA