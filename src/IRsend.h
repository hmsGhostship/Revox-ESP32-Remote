#ifndef IRSEND_H
#define IRSEND_H

#include <Arduino.h>

void initAddr();


// Pin-Definitionen
const int IRoutPin = 4;

// Funktionsprototypen
void setupIRoutPin();
void sendIR(String address, String ITTcode);

#endif