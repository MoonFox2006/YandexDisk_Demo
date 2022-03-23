#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <StreamString.h>
#include "YandexDisk.h"

#define EXAMPLE 1

const char WIFI_SSID[] PROGMEM = "***SSID***";
const char WIFI_PSWD[] PROGMEM = "***PSWD***";

const char OAUTH[] PROGMEM = "***token***";

YandexDisk<SPIFFS> yd(FPSTR(OAUTH));

static void halt(const __FlashStringHelper *msg) {
  Serial.println(msg);
  Serial.flush();
  ESP.deepSleep(0);
}

#if EXAMPLE < 3
static void testUpload() {
#if EXAMPLE == 1
  StreamString ss;

  ss.println(F("First line"));
  ss.println(F("Second line"));
  ss.println(F("Third line"));
//  ss.resetPointer();
  if (yd.upload(F("/test.txt"), *(Stream*)&ss, true)) {
#else
  if (yd.upload(F("/test.txt"), F("/test.txt"), true)) {
#endif
    Serial.println(F("File uploaded successfully"));
  } else {
    Serial.println(F("Error uploading file!"));
  }
}

static void testDownload() {
  if (yd.download(F("/test.txt"), Serial)) {
    Serial.println(F("*** OK ***"));
  } else {
    Serial.println(F("Error downloading file!"));
  }
}

#else
static void testAppend() {
  StreamString ss;

  if (yd.download(F("/test.txt"), *(Stream*)&ss)) {
    ss.println(ss.length());
    if (yd.upload(F("/test.txt"), *(Stream*)&ss, true)) {
      Serial.println(F("File updated successfully"));
    } else {
      Serial.println(F("Error uploading file!"));
    }
  } else {
    Serial.println(F("Error downloading file!"));
  }
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.println();

  if (! SPIFFS.begin()) {
    if ((! SPIFFS.format()) || (! SPIFFS.begin()))
      halt(F("SPIFFS error!"));
  }

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(FPSTR(WIFI_SSID), FPSTR(WIFI_PSWD));
  Serial.print(F("Connecting to WiFi"));
  while (! WiFi.isConnected()) {
    Serial.print('.');
    delay(500);
  }
  Serial.print(F(" OK (IP "));
  Serial.print(WiFi.localIP());
  Serial.println(')');

#if EXAMPLE < 3
  testUpload();
  testDownload();
#else
  testAppend();
#endif

  Serial.flush();
  ESP.deepSleep(0);
}

void loop() {}
