/**
 * @file inputs.cpp
 *
 * Button UX — “intent collection window” (human-friendly chords).
 * Pot: map raw ADC to 0..4095 for the host.
 */

#include "inputs.h"

#include <Arduino.h>

#include "config.h"

enum class IntentPhase : uint8_t { Idle, Collecting, Cooldown };

static IntentPhase g_phase = IntentPhase::Idle;
static unsigned long g_collect_start_ms = 0;
static uint8_t g_best_mask = 0;

static int g_last_sent_pot = -1;

static uint8_t g_candidate_mask = 0;
static unsigned long g_debounce_deadline_ms = 0;
static uint8_t g_stable_button_mask = 0;

static bool readButtonPressed(int pin) { return digitalRead(pin) == LOW; }

static uint8_t readRawButtonMask() {
  uint8_t mask = 0;
  if (readButtonPressed(kPinButton1)) {
    mask |= 1u;
  }
  if (readButtonPressed(kPinButton2)) {
    mask |= 2u;
  }
  if (readButtonPressed(kPinButton3)) {
    mask |= 4u;
  }
  return mask;
}

static uint8_t readDebouncedButtonMask() {
  const uint8_t raw = readRawButtonMask();
  const unsigned long now = millis();
  if (raw != g_candidate_mask) {
    g_candidate_mask = raw;
    g_debounce_deadline_ms = now + kButtonDebounceMs;
  }
  if (now >= g_debounce_deadline_ms) {
    g_stable_button_mask = g_candidate_mask;
  }
  return g_stable_button_mask;
}

static inline int buttonPopcount(uint8_t m) {
  return __builtin_popcount(static_cast<unsigned int>(m));
}

static void serialLogButtonsIfChanged(uint8_t mask) {
  static uint8_t s_last = 0xFF;
  if (mask == s_last) {
    return;
  }
  s_last = mask;
  if (mask == 0) {
    Serial.println("buttons: none");
    return;
  }
  Serial.print("buttons:");
  if ((mask & 1u) != 0) {
    Serial.print(" b1");
  }
  if ((mask & 2u) != 0) {
    Serial.print(" b2");
  }
  if ((mask & 4u) != 0) {
    Serial.print(" b3");
  }
  Serial.println();
}

static const char* maskToChord(uint8_t mask) {
  switch (mask) {
    case 1u:
      return "b1";
    case 2u:
      return "b2";
    case 4u:
      return "b3";
    case 3u:
      return "b1b2";
    case 6u:
      return "b2b3";
    default:
      return nullptr;
  }
}

static void emitChord(ChordHandler on_chord, uint8_t mask) {
  if (on_chord == nullptr) {
    return;
  }
  const char* chord = maskToChord(mask);
  if (chord != nullptr) {
    on_chord(chord);
  }
}

static void updateIntentFsm(ChordHandler on_chord) {
  const uint8_t mask = readDebouncedButtonMask();
  serialLogButtonsIfChanged(mask);

  switch (g_phase) {
    case IntentPhase::Idle: {
      if (mask != 0) {
        g_phase = IntentPhase::Collecting;
        g_collect_start_ms = millis();
        g_best_mask = mask;
      }
      break;
    }
    case IntentPhase::Collecting: {
      if (mask != 0) {
        if (buttonPopcount(mask) > buttonPopcount(g_best_mask)) {
          g_best_mask = mask;
        }
      }
      if (millis() - g_collect_start_ms >= kIntentCollectionMs) {
        emitChord(on_chord, g_best_mask);
        g_phase = IntentPhase::Cooldown;
      }
      break;
    }
    case IntentPhase::Cooldown: {
      if (mask == 0) {
        g_phase = IntentPhase::Idle;
      }
      break;
    }
  }
}

static int readPotRawAveraged() {
  const int n = kPotAnalogSampleCount;
  if (n <= 1) {
    return analogRead(kPinPotentiometer);
  }
  long sum = 0;
  for (int i = 0; i < n; ++i) {
    sum += analogRead(kPinPotentiometer);
  }
  return static_cast<int>((sum + n / 2) / n);
}

static int mapPotToHostRange(int sample) {
  const int lo = kPotAdcAtMin;
  const int hi = kPotAdcAtMax;
  if (hi <= lo) {
    return constrain(sample, 0, 4095);
  }
  long x = (static_cast<long>(sample - lo) * 4095L) / static_cast<long>(hi - lo);
  if (x < 0L) {
    x = 0L;
  }
  if (x > 4095L) {
    x = 4095L;
  }
  return static_cast<int>(x);
}

static void updatePotentiometer(PotHandler on_pot) {
  if (on_pot == nullptr) {
    return;
  }
  const int raw = readPotRawAveraged();
  const int mapped = mapPotToHostRange(raw);
  if (g_last_sent_pot < 0) {
    g_last_sent_pot = mapped;
    on_pot(mapped);
    return;
  }
  if (abs(mapped - g_last_sent_pot) >= kPotDeadband) {
    g_last_sent_pot = mapped;
    on_pot(mapped);
  }
}

void inputsConfigurePotAdc() {
  analogReadResolution(12);
#if defined(ARDUINO_ARCH_ESP32)
#if defined(ADC_ATTEN_DB_11)
  analogSetPinAttenuation(kPinPotentiometer, ADC_ATTEN_DB_11);
#elif defined(ADC_11db)
  analogSetPinAttenuation(kPinPotentiometer, ADC_11db);
#endif
  (void)analogRead(kPinPotentiometer);
#endif
}

void inputsBegin() {
  pinMode(kPinButton1, INPUT_PULLUP);
  pinMode(kPinButton2, INPUT_PULLUP);
  pinMode(kPinButton3, INPUT_PULLUP);
  pinMode(kPinPotentiometer, INPUT);
  inputsConfigurePotAdc();

  g_phase = IntentPhase::Idle;
  g_best_mask = 0;
  g_candidate_mask = readRawButtonMask();
  g_stable_button_mask = g_candidate_mask;
  g_debounce_deadline_ms = millis();
}

void inputsLoop(ChordHandler on_chord, PotHandler on_pot) {
  updateIntentFsm(on_chord);
  updatePotentiometer(on_pot);
}
