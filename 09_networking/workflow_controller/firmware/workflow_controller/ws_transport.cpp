/**
 * @file ws_transport.cpp
 */

#include "ws_transport.h"

#include <WebSocketsClient.h>
#include <WiFi.h>
#include <cstdio>

#include "config.h"
#include "secrets.h"

static WebSocketsClient g_ws;
static bool g_ws_configured = false;
static unsigned long g_last_reconnect_attempt_ms = 0;

static void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  (void)payload;
  (void)length;
  switch (type) {
    case WStype_DISCONNECTED:
    case WStype_CONNECTED:
    case WStype_TEXT:
    case WStype_ERROR:
    default:
      break;
  }
}

void wsTransportBegin() {
  if (g_ws_configured) {
    return;
  }
  g_ws.onEvent(onWebSocketEvent);
  g_ws_configured = true;
}

void wsTransportLoop() {
  g_ws.loop();

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (g_ws.isConnected()) {
    return;
  }

  const unsigned long now = millis();
  if (now - g_last_reconnect_attempt_ms < kWebSocketReconnectIntervalMs) {
    return;
  }
  g_last_reconnect_attempt_ms = now;
  g_ws.begin(secrets::WEBSOCKET_HOST, kWebSocketPort, kWebSocketPath);
}

bool wsIsConnected() { return g_ws.isConnected(); }

bool wsSendChord(const char* chord) {
  char payload[160];
  const int n =
      snprintf(payload, sizeof(payload),
               "{\"schema_version\":%d,\"type\":\"button_chord\",\"chord\":\"%s\"}",
               kSchemaVersion, chord);
  if (n <= 0 || static_cast<size_t>(n) >= sizeof(payload)) {
    return false;
  }
  return g_ws.sendTXT(payload);
}

bool wsSendPotentiometer(int value) {
  if (value < 0) {
    value = 0;
  }
  if (value > 4095) {
    value = 4095;
  }
  char payload[128];
  const int n = snprintf(payload, sizeof(payload),
                         "{\"schema_version\":%d,\"type\":\"potentiometer\","
                         "\"value\":%d}",
                         kSchemaVersion, value);
  if (n <= 0 || static_cast<size_t>(n) >= sizeof(payload)) {
    return false;
  }
  return g_ws.sendTXT(payload);
}
