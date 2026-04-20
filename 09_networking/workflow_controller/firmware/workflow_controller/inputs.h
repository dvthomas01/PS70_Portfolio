/**
 * @file inputs.h
 * @brief Buttons (chording) and potentiometer sampling.
 */
#pragma once

typedef void (*ChordHandler)(const char* chord);
typedef void (*PotHandler)(int adc_value);

void inputsBegin();
void inputsLoop(ChordHandler on_chord, PotHandler on_pot);

/** 12-bit resolution + full-scale 0–3.3 V range. Re-call after Wi-Fi starts (radio init can reset ADC). */
void inputsConfigurePotAdc();
