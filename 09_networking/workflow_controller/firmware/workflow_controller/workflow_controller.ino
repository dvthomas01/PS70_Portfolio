/**
 * @file workflow_controller.ino
 * @brief AI Workflow Command Center — XIAO ESP32-C3 firmware (Arduino IDE sketch).
 *
 * Serial Monitor (115200): Wi-Fi status and button state. Optional LED: led_feedback.cpp.
 */

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "inputs.h"
#include "led_feedback.h"
#include "reachability.h"
#include "wifi_sta.h"
#include "ws_transport.h"

LedFeedback g_led;

static bool g_reachability_ok = false;
static unsigned long g_last_reach_check_ms = 0;
static unsigned long g_last_serial_status_ms = 0;
static bool g_pot_adc_reconfigured_after_wifi = false;

static void waitForSerialBriefly() {
  const unsigned long start = millis();
  while (millis() - start < 3000) {
    if (Serial) {
      break;
    }
    delay(50);
  }
}

static void printStatusLine() {
  if (WiFi.status() == WL_CONNECTED) {
    String ip_str = WiFi.localIP().toString();
    Serial.printf("[%lu ms] wifi OK ip=%s\n", static_cast<unsigned long>(millis()),
                  ip_str.c_str());
  } else {
    Serial.printf("[%lu ms] wifi DISC\n", static_cast<unsigned long>(millis()));
  }
}

static void onChord(const char* chord) {
  const bool path_ok = (WiFi.status() == WL_CONNECTED) && g_reachability_ok &&
                       wsIsConnected();
  if (!path_ok) {
    g_led.requestFlashRed();
    return;
  }
  if (wsSendChord(chord)) {
    g_led.requestFlashGreen();
  } else {
    g_led.requestFlashRed();
  }
}

static void onPot(int value) {
  // Volume is local (Mac over LAN). Do not require internet reachability: campus/captive
  // Wi-Fi often breaks the HTTP 204 probe while WebSocket to the laptop still works.
  const bool path_ok = (WiFi.status() == WL_CONNECTED) && wsIsConnected();
  if (!path_ok) {
    return;
  }
  (void)wsSendPotentiometer(value);
}

void setup() {
  Serial.begin(115200);
  waitForSerialBriefly();
  Serial.println("workflow_controller: start");

  g_led.begin();
  inputsBegin();
  wifiStaBegin();
  wsTransportBegin();

  g_last_reach_check_ms = millis() - kReachabilityCheckIntervalMs;
  g_last_serial_status_ms = millis() - kSerialStatusIntervalMs;
}

void loop() {
  wifiStaLoop();
  wsTransportLoop();

  const bool wifi_ok = (WiFi.status() == WL_CONNECTED);
  if (wifi_ok) {
    if (!g_pot_adc_reconfigured_after_wifi) {
      inputsConfigurePotAdc();
      g_pot_adc_reconfigured_after_wifi = true;
    }
    const unsigned long now = millis();
    if (now - g_last_reach_check_ms >= kReachabilityCheckIntervalMs) {
      g_last_reach_check_ms = now;
      g_reachability_ok = checkInternetReachable204();
    }
  } else {
    g_reachability_ok = false;
    g_pot_adc_reconfigured_after_wifi = false;
  }

  const bool show_connecting_pulse = !wifi_ok || !g_reachability_ok;
  g_led.setConnectingPulse(show_connecting_pulse);

  if (kSerialStatusIntervalMs > 0) {
    const unsigned long now = millis();
    if (now - g_last_serial_status_ms >= kSerialStatusIntervalMs) {
      g_last_serial_status_ms = now;
      printStatusLine();
    }
  }

  inputsLoop(onChord, onPot);
  g_led.update();
}
