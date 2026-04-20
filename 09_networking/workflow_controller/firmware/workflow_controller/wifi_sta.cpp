/**
 * @file wifi_sta.cpp
 */

#include "wifi_sta.h"

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "secrets.h"

void wifiStaBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASSWORD);
}

void wifiStaLoop() {
  static unsigned long last_attempt_ms = 0;
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  const unsigned long now = millis();
  if (now - last_attempt_ms < kWiFiReconnectIntervalMs) {
    return;
  }
  last_attempt_ms = now;
  WiFi.disconnect(true, false);
  delay(50);
  WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASSWORD);
}
