#ifndef SERIALLINK_H
#define SERIALLINK_H

#include <Arduino.h>

// Die exakten ITT-Timing-Konstanten
const int PULSE_DURATION  = 20;   // Jeder physikalische Nadelimpuls dauert exakt 15 µs
const int LOGIC_0_DELAY   = 130;  // Pause nach dem Puls für eine '0' (exakt 285 µs)
const int LOGIC_1_DELAY   = 280;  // Pause nach dem Puls für eine '1' (exakt 135 µs)
const int FRAME_DELAY     = 10000; // 10 ms Lead-out nach dem finalen Stopppuls
// Symmetrische Revox-Weckpausen (Pre- und Post-Data)
const unsigned long REVOX_LONG_PAUSE = 435; // 435 Mikrosekunden

// SerielLink Pin-Definitionen
const int SLoutPin1 = 5;
const int SLoutPin2 = 4;

// Funktionsprototypen
void setupSLoutPin();
void sendRevoxFrame(uint8_t Address, uint8_t ircmd, int repetitions);
//void sendIR(String Address, String ircmd, int repetitions);

#endif