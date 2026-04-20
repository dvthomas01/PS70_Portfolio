/**
 * @file config.h
 * @brief Pinout and timing constants for the XIAO ESP32-C3 shortcuts station.
 *
 * Stock Seeed XIAO ESP32-C3 has no onboard WS2812 NeoPixel. The small red LED on
 * the board is typically a charge/power indicator, not a user RGB. See:
 * https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started
 *
 * Feedback options (pick one or none in led_feedback.cpp macros):
 * - NeoPixel: only if you wired WS2812 to kPinNeoPixel (or use a board that has one).
 * - Digital: LED + resistor on kPinDigitalStatusLed (Seeed blink tutorial uses D10).
 * - Neither: use Serial Monitor only (always available over USB).
 */
#pragma once

#include <Arduino.h>

// --- Hardware feedback (edit in led_feedback.cpp: WORKFLOW_HW_* ) ---
constexpr int kPinNeoPixel = 8;
constexpr int kNeoPixelCount = 1;

/** Seeed “blink” examples use D10 + external LED + resistor. */
constexpr int kPinDigitalStatusLed = D10;

// --- Pinout (Seeed XIAO ESP32-C3) ---
constexpr int kPinButton1 = D5;
constexpr int kPinButton2 = D1;
constexpr int kPinButton3 = D2;
constexpr int kPinPotentiometer = D0;

// --- Button / chord timing ---
/** How long the raw button mask must stay stable before it is trusted. */
constexpr unsigned long kButtonDebounceMs = 12;
/**
 * Intent collection window: time from first debounced press to firing the “best”
 * simultaneous mask (see inputs.cpp). Allows human chord timing (~30–70 ms skew).
 */
constexpr unsigned long kIntentCollectionMs = 60;
/** Potentiometer: mapped 0..4095 delta before another WebSocket send (smaller = finer volume steps). */
constexpr int kPotDeadband = 20;
/** Analog reads averaged per update (Wi‑Fi on ESP32 makes ADC noisy; reduces false 0 spikes). */
constexpr int kPotAnalogSampleCount = 12;
/**
 * Calibrate to your wiring: note analogRead() at full CCW (min volume) and full CW (max).
 * The firmware maps that range to 0..4095 for the host; macOS maps 0..4095 → 0–100%.
 * Example (3V3 pot on D0): ~20–80 at CCW, ~4050–4095 at CW — use your measured pair.
 */
constexpr int kPotAdcAtMin = 20;
constexpr int kPotAdcAtMax = 4095;
constexpr unsigned long kReachabilityCheckIntervalMs = 5000;
constexpr unsigned long kWiFiReconnectIntervalMs = 8000;
constexpr unsigned long kWebSocketReconnectIntervalMs = 2500;

// --- WebSocket (port/path are not secret; host is in secrets.h) ---
constexpr uint16_t kWebSocketPort = 8765;
constexpr char kWebSocketPath[] = "/";

// --- Reachability probe ---
constexpr char kReachabilityUrl[] =
    "http://connectivitycheck.gstatic.com/generate_204";

// --- Protocol ---
constexpr int kSchemaVersion = 1;

/** How often to print Wi-Fi status on Serial (0 = disable). */
constexpr unsigned long kSerialStatusIntervalMs = 3000;
