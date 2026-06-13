#include "IRsend.h"

void setupIRoutPin() {
    pinMode(IRoutPin1, OUTPUT);
    pinMode(IRoutPin2, OUTPUT);
}

void IRoutPin(bool status) {
  digitalWrite(IRoutPin1, status);
  digitalWrite(IRoutPin2, status);
}

// Hilfsfunktion: Erzeugt den reinen unmodulierten 15µs Infrarot-Puls
void transmitRawPulse() {
  IRoutPin(HIGH);
  delayMicroseconds(PULSE_DURATION);
  IRoutPin(LOW);
}

// Hilfsfunktion, um ein einzelnes Bit mit dem exakten Timing zu senden
void sendSingleBit(bool bitValue) {
  transmitRawPulse();
  if (bitValue == 0) {
    delayMicroseconds(LOGIC_0_DELAY);
  } else {
    delayMicroseconds(LOGIC_1_DELAY);
  }
}

// REVOX-SENDEFUNKTION
void sendRevoxFrame(uint8_t Address, uint8_t ircmd, int repetitions) {

  byte adresse = Address -1;

  uint16_t dataPayload = ((ircmd & 0x3F) << 4) | (adresse & 0x0F);

  Serial.println(dataPayload, BIN);

  // 1. DATEN-BLOCKS (Wiederholungen / Repetitions)
  for (int r = 0; r < repetitions; r++) {

  // 2. VORBEREITUNGSPULS (Pre-Data / Preamble)
  // Wird einmalig am Anfang der gesamten Kette gesendet
  transmitRawPulse();

  delayMicroseconds(REVOX_LONG_PAUSE); // Erste 435 us Pause zum Aufwecken der Logik
  
  // Startpuls ist immer eine '0' -> verlangt die kurze 135 µs Pause
  sendSingleBit(0); 

    for (int b = 0; b <= 9; b++) {
      bool currentBit = dataPayload & 1; // Liest Bit 0
      sendSingleBit(currentBit);
      dataPayload >>= 1;                // Holt das nächste Bit nach vorne
    }

    // 4. FINALEE STOPP-Sequenz
    transmitRawPulse();
    delayMicroseconds(REVOX_LONG_PAUSE); // Symmetrische 435 us Pause vor dem Abschluss
    transmitRawPulse();

    // Zwischenpause zwischen den Blöcken (Lead-out)
    // Nur verzögern, wenn noch ein weiterer Wiederholungsblock folgt!
    if (r < (repetitions - 1)) {
      delayMicroseconds(FRAME_DELAY); // 10 us Pause bis zum nächsten Frame-Start
    }
  }
  
  // Kurze Nachlaufzeit zur Beruhigung der Empfängerstufe
  delayMicroseconds(FRAME_DELAY);
}