#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h> // Wichtig für byte, PROGMEM etc.

// 1. Definition des Structs
struct outputs {
  const char* descr;
  const char* out;
};

static const outputs outputTable [] = {
    { "b203", "0" },
    { "cdplayer", "1" },
    { "tape2", "2" },
    { "amplifier", "3" },
    { "tuner", "4" },
    { "phono", "5" },
    { "tape1", "9" },
    { NULL, NULL }
    };

const int outputTableSize = sizeof(outputTable) / sizeof(outputTable[0]);

struct command {
    const char* btnName;
    uint16_t irRecCode;
    const char* address;
    const char* ITTcode;
    uint16_t cmdFlag;
    const char* serCmd;
    bool once;
    const char* device;
};

static const command cmdTable [] = {
    // Timer Controller B203
    // btnname, irReCode, address,  ITTcode,  cmdFlag, serCmd,  once, device
    // cmdFlag 0= CMD disabled, 1 = send adress+CMD 2 = send CMD
    // once 0 = IR command send repeated 1 = IR command send once
    { "b203poweroff", 0x0, "1000", "111111", 0, NULL, 1, "b203" },
    { "b203exit", 0x0, "0110", "001111", 0, NULL, 1, "b203" },
    { "b203time", 0x0, "0110", "011011", 0, NULL, 1, "b203" },
    { "b203event", 0x0, "0110", "110111", 0, NULL, 1, "b203"  },
    { "b203select", 0x0, "0110", "010111", 0, NULL, 1, "b203"  },
    { "b203test", 0x0, "0110", "101011", 0, NULL, 1, "b203" },
    { "b203prev", 0x0, "1000", "000010", 0, NULL, 1, "b203" },
    { "b203next", 0x0, "1000", "100010", 0, NULL, 1, "b203" },
    { "b203enter", 0x0, "1000", "000110", 0, NULL, 1, "b203" },
    { "b203reset", 0x0, NULL, NULL, 2, "RES", 1, "b203" },
    { "trueIR", 0x0, NULL, NULL, 1, "R0", 1, "b203"},
    { "falseIR", 0x0, NULL, NULL, 1, "R1", 1, "b203"},
    { "b203", 0x0, NULL, NULL, 1, "X", 1, "b203" },
    // Amplifier B251
    { "amptape1", 0x0, "1000", "111100", 0, NULL, 1, "amplifier" },
    { "amptuner", 0x0, "1000", "011100", 0, NULL, 1, "amplifier" },
    { "ampphono", 0x0, "1000", "101100", 0, NULL, 1, "amplifier" },
    { "amptape2", 0x0, "1000", "101010", 0, NULL, 1, "amplifier"  },
    { "ampcd", 0x0, "1000", "001010", 0, NULL, 1, "amplifier"  },
    { "ampaux", 0x0, "1000", "001100", 0, NULL, 1, "amplifier" },
    { "amprecmon", 0x0, "1000", "100100", 0, NULL, 1, "amplifier" },
    { "amptone", 0x0, "1000", "010100", 0, NULL, 1, "amplifier" },
    { "amp20db", 0x0, "1000", "110100", 0, NULL, 1, "amplifier" },
    { "ampbal_r", 0x0, "1000", "010010", 0, NULL, 0, "amplifier" },
    { "ampbal_l", 0x0, "1000", "110010", 0, NULL, 0, "amplifier" },
    { "ampvoldn", 0x0, "1000", "000010", 0, NULL, 0, "amplifier" },
    { "ampvolup", 0x0, "1000", "100010", 0, NULL, 0, "amplieifer" },
    { "ampvoldnx", 0x0, "1000", "011010", 0, NULL, 0, "amplifier"  },
    { "ampvolupx", 0x0, "1000", "111010", 0, NULL, 0, "amplifier"  },
    // Tuner B261
    { "tunpoweroff", 0xA91, "1000", "111111", 0, NULL, 1, "tuner" },
    { "tun1",  0x0,  "1000", "011101", 0, NULL, 1, "tuner" },
    { "tun2",	0x0, "1000", "101101", 0, NULL, 1, "tuner" },
    { "tun3",	0x0, "1000", "001101", 0, NULL, 1, "tuner" },
    { "tun4",	0x0, "1000", "110101", 0, NULL, 1, "tuner" },
    { "tun5",	0x0, "1000", "010101", 0, NULL, 1, "tuner" },
    { "tun6",	0x0, "1000", "100101", 0, NULL, 1, "tuner" },
    { "tun7",	0x0, "1000", "000101", 0, NULL, 1, "tuner" },
    { "tun8",	0x0, "1000", "111001", 0, NULL, 1, "tuner" },
    { "tun9", 0x0,  "1000", "011001", 0, NULL, 1, "tuner" },
    { "tun10",  0x0,  "1000", "101001", 0, NULL, 1, "tuner" },
    { "tun12",  0x0, "1000", "110001", 0, NULL, 1, "tuner" },
    { "tun13",  0x0, "1000", "010001", 0, NULL, 1, "tuner" },
    { "tun14",  0x0, "1000", "100001", 0, NULL, 1, "tuner" },
    { "tun15",  0x0, "1000", "000001", 0, NULL, 1, "tuner" },
    { "tun16",	0x0, "1000", "111110", 0, NULL, 1, "tuner" },
    { "tun17",	0x0, "1000", "011110", 0, NULL, 1, "tuner" },
    { "tun18",  0x0, "1000", "101110", 0, NULL, 1, "tuner" },
    { "tun19",  0x0,  "1000", "001110", 0, NULL, 1, "tuner" },
    { "tun20",  0x0,  "1000", "110110", 0, NULL, 1, "tuner" },
    { "tunscanlast", 0x0, "1000", "010110", 0, NULL, 1, "tuner" },
    { "tunscannext", 0x0, "1000", "100110", 0, NULL, 1, "tuner" },
    { "tun10on", 0x0, "1000", "000110", 0, NULL, 1, "tuner" },
    { "tunhighblend", 0x0, "1000", "011000", 0, NULL, 1, "tuner" },
    { "tunmute", 0x0, "1000", "101000", 0, NULL, 1, "tuner" },
    { "tunreccal", 0x0, "1000", "001000", 0, NULL, 1, "tuner" },
    { "tunstore", 0x0, "1000", "11000", 0, NULL, 1, "tuner" },
    { "tunmono", 0x0, "1000", "010000", 0, NULL, 0, "tuner" },
    { "tunstonly", 0x0, "1000", "100000", 0, NULL, 0, "tuner" },
    { "tunantenna", 0x0, "1000", "000000", 0, NULL, 1, "tuner" },
    // CD Player B226
    { "cdpoweroff", 0xA91, "1000", "111111", 0, "O", 1, "cdplayer" },
    { "cd1",  0x11,  "0000", "011101", 0, NULL, 1, "cdplayer" },
    { "cd2",	0x811, "0000", "101101", 0, NULL, 1, "cdplayer" },
    { "cd3",	0x411, "0000", "001101", 0, NULL, 1, "cdplayer" },
    { "cd4",	0xC11, "0000", "110101", 0, NULL, 1, "cdplayer" },
    { "cd5",	0x211, "0000", "010101", 0, NULL, 1, "cdplayer" },
    { "cd6",	0xA11, "0000", "100101", 0, NULL, 1, "cdplayer" },
    { "cd7",	0x611, "0000", "000101", 0, NULL, 1, "cdplayer" },
    { "cd8",	0xE11, "0000", "111001", 0, NULL, 1, "cdplayer" },
    { "cd9",   0x51,  "0000", "011001", 0, NULL, 1, "cdplayer" },
    { "cdplay", 0x4D1, "1000", "000100", 0, "P", 1, "cdplayer" },
    { "cdstop", 0x1D1, "0000", "000110", 0, "S", 1, "cdplayer" },
    { "cdpause", 0x9D1, "0000", "111101", 0, "W", 1, "cdplayer" },
    { "cdpause_on", 0x711, "0000", "101101", 0, NULL, 1, "cdplayer" },
    { "cdrew", 0xCD1, "0000", "100011", 0, NULL, 0, "cdplayer" },
    { "cdforw", 0x2D1, "0000", "000011", 0, NULL, 0, "cdplayer" },
    { "cdindscan", 0x391, "0000", "100110", 0, "J", 1, "cdplayer" },
    { "cdvolup", 0xF11, "1000", "100010", 0, NULL, 0, "cdplayer" },
    { "cdvoldn", 0xB91, "1000", "000010", 0, NULL, 0, "cdplayer" },
    { "cdpreemph_on", 0xD11, "0000", "001001", 0, NULL, 1, "cdplayer" },
    { "cdpreemph_off", 0x391, "0000", "101001", 0, NULL, 1, "cdplayer" },
    { "cdlocal_time", 0x31,  "0000", "010010", 0, "T", 1, "cdplayer" },
    { "cdtotal_time", 0x931, "0000", "100001", 0, "D", 1, "cdplayer" },
    { "cdlocate", 0x831, "0000", "111110", 0, "H", 1, "cdplayer" },
    { "cdautostp_on", 0xC31, "0000", "101110", 0, "U", 1, "cdplayer" },
    { "cdautostp_off", 0x231, "0000", "001110", 0, "V", 1, "cdplayer" },
    { "cdpause_wom", 0x431, "0000", "110110", 0, NULL, 1, "cdplayer" },
    { "cdload", 0xB11, "0000", "011110", 1, "E", 1, "cdplayer" },
    { "cd", 0x0, NULL, NULL, 1, "X", 1, "cdplayer" },
    // Tape1 Reel to Reel B77
    { "tape1poweroff", 0x0, "1000", "111111", 0, "NULL", 1, "tape1" },
    { "tape1play",  0x0,  "0000", "101011", 0, "P", 1, "tape1" },
    { "tape1stop",	0x0, "0000", "001111", 0, "S", 1, "tape1" },
    { "tape1pause",	0x0, "0000", "011011", 0, "W", 1, "tape1" },
    { "tape1playrec",	0x0, "0000", "101111", 0, "R", 1, "tape1" },
    { "tape1pauserec",	0x0, "0000", "011111", 0, NULL, 1, "tape1" },
    { "tape1rew",	0x0, "0000", "110111", 0, "B", 1, "tape1" },
    { "tape1forw",	0x0, "0000", "010111", 0, "F", 1, "tape1" },
    { "tape1mon",	0x0, "0000", "100111", 0, NULL, 1, "tape1" },
    // Tape2 Kassettendeck B215
    { "tape2poweroff", 0x0, "1000", "111111", 0, "O", 1, "tape2" },
    { "tape2play",  0x0,  "0000", "101011", 0, "P", 1, "tape2" },
    { "tape2stop",	0x0, "0000", "001111", 0, "S", 1, "tape2" },
    { "tape2pause",	0x0, "0000", "011011", 0, "W", 1, "tape2" },
    { "tape2playrec",	0x0, "0000", "101111", 0, "R", 1, "tape2" },
    { "tape2pauserec",	0x0, "0000", "011111", 0, "V", 1, "tape2" },
    { "tape2rew",	0x0, "0000", "110111", 0, "B", 1, "tape2" },
    { "tape2forw",	0x0, "0000", "010111", 0, "F", 1, "tape2" },
    { "tape2mon",	0x0, "0000", "100111", 0, NULL, 1, "tape2" },
    { "tape2loc1",	0x0, "0000", "000111", 0, "Q", 1, "tape2" },
    { "tape2loc2",	0x0, "0000", "111011", 0, "Z", 1, "tape2" },
    { "tape2loop",	0x0, "0000", "001011", 0, "L", 1, "tape2" },
    { "tape2", 0x0, NULL, NULL, 1, "X", 1, "tape2" },
    // Phono Plattenspieler B291
    { "phonopoweroff", 0x0, "1000", "111111", 0, NULL, 1, "phono" },
    { "phonorew",  0x0,  "0000", "100011", 0, NULL, 1, "phono" },
    { "phonoforw",	0x0, "0000", "000011", 0, NULL, 1, "phono" },
    { "phonolift",	0x0, "0000", "111101", 0, NULL, 1, "phono" },
    { NULL, 0x0, NULL,   NULL, 0, NULL, 0, NULL }
};

const int cmdTableSize = sizeof(cmdTable) / sizeof(cmdTable[0]);

#endif