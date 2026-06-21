#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h> // Wichtig für byte, PROGMEM etc.

struct portcnf {
  char name[5];
  char descr[12];
  char out[3];
  bool feedback;
};

struct command {
    char btnID[32];
    uint32_t irRecvCode;
    uint8_t address;
    uint8_t command;
    uint8_t cmdFlag;
    char serCmd[8];
    char bibusCmd[8];
    bool repeat;
    char device[32];
    bool isBibus;
};

#endif