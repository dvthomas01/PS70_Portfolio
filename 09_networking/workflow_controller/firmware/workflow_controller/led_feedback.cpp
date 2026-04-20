/**
 * @file led_feedback.cpp
 *
 * Enable at most one hardware backend by setting a macro to 1:
 * - WORKFLOW_HW_NEOPIXEL: WS2812 on kPinNeoPixel (not present on stock XIAO ESP32-C3).
 * - WORKFLOW_HW_DIGITAL_LED: single LED + resistor on kPinDigitalStatusLed (e.g. D10).
 * If both are 0, all LED calls are no-ops; use Serial in the sketch for status.
 */

#include "led_feedback.h"

#include <Arduino.h>
#include <math.h>

#include "config.h"

#ifndef WORKFLOW_HW_NEOPIXEL
#define WORKFLOW_HW_NEOPIXEL 0
#endif
#ifndef WORKFLOW_HW_DIGITAL_LED
#define WORKFLOW_HW_DIGITAL_LED 0
#endif

#if WORKFLOW_HW_NEOPIXEL
#include <Adafruit_NeoPixel.h>
static Adafruit_NeoPixel g_strip(kNeoPixelCount, kPinNeoPixel,
                                 NEO_GRB + NEO_KHZ800);
#endif

#if WORKFLOW_HW_NEOPIXEL
static void neoApplyRgb(uint8_t r, uint8_t g, uint8_t b) {
  g_strip.setPixelColor(0, r, g, b);
  g_strip.show();
}
#endif

void LedFeedback::begin() {
#if WORKFLOW_HW_NEOPIXEL
  g_strip.begin();
  g_strip.setBrightness(40);
  g_strip.show();
#elif WORKFLOW_HW_DIGITAL_LED
  pinMode(kPinDigitalStatusLed, OUTPUT);
  digitalWrite(kPinDigitalStatusLed, LOW);
#else
  // No status LED hardware; Serial only.
#endif
  initialized_ = true;
}

void LedFeedback::setConnectingPulse(bool enabled) { connecting_pulse_ = enabled; }

void LedFeedback::requestFlashGreen() {
  flash_green_until_ms_ = millis() + kFlashMsGreen;
  flash_red_until_ms_ = 0;
}

void LedFeedback::requestFlashRed() {
  flash_red_until_ms_ = millis() + kFlashMsRed;
  flash_green_until_ms_ = 0;
}

void LedFeedback::update() {
  if (!initialized_) {
    return;
  }

  const unsigned long now = millis();

#if WORKFLOW_HW_NEOPIXEL
  if (flash_green_until_ms_ != 0 && now < flash_green_until_ms_) {
    neoApplyRgb(0, 255, 0);
    return;
  }
  flash_green_until_ms_ = 0;

  if (flash_red_until_ms_ != 0 && now < flash_red_until_ms_) {
    neoApplyRgb(255, 0, 0);
    return;
  }
  flash_red_until_ms_ = 0;

  if (connecting_pulse_) {
    const float t =
        static_cast<float>(now % kPulsePeriodMs) / static_cast<float>(kPulsePeriodMs);
    const float phase = t * 2.0f * static_cast<float>(M_PI);
    const float s = (sinf(phase) + 1.0f) * 0.5f;
    const auto level = static_cast<uint8_t>(20 + s * 110);
    neoApplyRgb(0, 0, level);
    return;
  }

  neoApplyRgb(0, 0, 0);

#elif WORKFLOW_HW_DIGITAL_LED
  if (flash_green_until_ms_ != 0 && now < flash_green_until_ms_) {
    digitalWrite(kPinDigitalStatusLed, HIGH);
    return;
  }
  flash_green_until_ms_ = 0;

  if (flash_red_until_ms_ != 0 && now < flash_red_until_ms_) {
    digitalWrite(kPinDigitalStatusLed, HIGH);
    return;
  }
  flash_red_until_ms_ = 0;

  if (connecting_pulse_) {
    digitalWrite(kPinDigitalStatusLed, (now / 400) % 2 ? HIGH : LOW);
    return;
  }

  digitalWrite(kPinDigitalStatusLed, LOW);

#else
  (void)now;
  if (flash_green_until_ms_ != 0 && now >= flash_green_until_ms_) {
    flash_green_until_ms_ = 0;
  }
  if (flash_red_until_ms_ != 0 && now >= flash_red_until_ms_) {
    flash_red_until_ms_ = 0;
  }
#endif
}
