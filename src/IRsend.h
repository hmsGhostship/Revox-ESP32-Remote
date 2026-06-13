#ifndef IRSEND_H
#define IRSEND_H

#include <Arduino.h>

// Die exakten ITT-Timing-Konstanten
const int PULSE_DURATION  = 20;   // Jeder physikalische Nadelimpuls dauert exakt 15 µs
const int LOGIC_0_DELAY   = 130;  // Pause nach dem Puls für eine '0' (exakt 285 µs)
const int LOGIC_1_DELAY   = 280;  // Pause nach dem Puls für eine '1' (exakt 135 µs)
const int FRAME_DELAY     = 10000; // 10 ms Lead-out nach dem finalen Stopppuls
// Symmetrische Revox-Weckpausen (Pre- und Post-Data)
const unsigned long REVOX_LONG_PAUSE = 435; // 435 Mikrosekunden

// =========================================================================
// ADRESSEN BASIEREND AUF DER TASTEN-MATRIX DER FERNBEDIENUNG (4 Bits)
// =========================================================================
const uint8_t ADDR_STANDARD    = 0x0F; // Standard-Tastendruck (z.B. Verstärker primär)
const uint8_t ADDR_PUNKT_EASY  = 0x0A; // Gedrückte EASY / • Taste (Adresse 10 dezimal)
const uint8_t ADDR_STERN_FIRST = 0x01; // Gedrückte * Taste (1. Frame sendet Adresse 1)
const uint8_t ADDR_STERN_REP   = 0x10; // Gedrückte * Taste (Folge-Frames senden Adresse 16 / 0x10)
const uint8_t UNUSED_ADDRESS   = 0x11; // Nicht genutzte Adressse

// SerielLink Pin-Definitionen
const int IRoutPin1 = 5;
const int IRoutPin2 = 4;

// Funktionsprototypen
void setupIRoutPin();
void sendRevoxFrame(uint8_t Address, uint8_t ircmd, int repetitions);
//void sendIR(String Address, String ircmd, int repetitions);

#endif