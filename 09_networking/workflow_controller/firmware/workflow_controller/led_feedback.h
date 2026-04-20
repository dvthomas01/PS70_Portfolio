/**
 * @file led_feedback.h
 * @brief Connection / event feedback: NeoPixel (optional), digital LED (optional), or no-op.
 *
 * On stock XIAO ESP32-C3 there is no onboard WS2812; use Serial Monitor and/or set
 * WORKFLOW_HW_DIGITAL_LED in led_feedback.cpp if you wire an LED to D10.
 */
#pragma once

#include <stdint.h>

class LedFeedback {
 public:
  void begin();

  /**
   * @brief When true, show “still connecting” (NeoPixel blue pulse or digital blink).
   */
  void setConnectingPulse(bool enabled);

  void requestFlashGreen();
  void requestFlashRed();

  /** Call each loop iteration; non-blocking. */
  void update();

 private:
  static constexpr int kFlashMsGreen = 220;
  static constexpr int kFlashMsRed = 600;
  static constexpr int kPulsePeriodMs = 1800;

  bool connecting_pulse_ = true;
  unsigned long flash_green_until_ms_ = 0;
  unsigned long flash_red_until_ms_ = 0;
  bool initialized_ = false;
};
