#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h> // Wichtig für byte, PROGMEM etc.

// 1. Definition des Structs
struct outputs {
  const char* descr;
  const char* out;
  bool feedback;
};

static const outputs outputTable [] = {
    { "b203", "0", 1 },
    { "cdplayer", "1", 1 },
    { "tape2", "2", 1 },
    { "amplifier", "3", 0 },
    { "tuner", "4", 0 },
    { "phono", "5", 0 },
    { "tape1", "9", 0 },
    { NULL, NULL }
    };

const int outputTableSize = sizeof(outputTable) / sizeof(outputTable[0]);

struct command {
    const char* btnID;
    uint32_t irRecvCode;
    const char* address;
    const char* ITTcode;
    uint16_t cmdFlag;
    const char* serCmd;
    bool repeat;
    const char* device;
};

static const command cmdTable [] = {
    // Timer Controller B203
    // btnID, irRecvCode, address,  ITTcode,  cmdFlag, serCmd,  once, device
    // btnID = Button ID
    // irRecvCode == IIR code of the transmitting remote control
    // address = IR/SerielLink ITT-address
    // ITTcode = IR/SerialLink ITT-Code
    // cmdFlag 0= CMD disabled, 1 = send adress+CMD // SerialLink 0 = enabled, 1 (or >0) = disabled
    // serCmd = Serial Command
    // repeat 0 = IR command send once, 1 = IR command send repeated
    // device = Description of the Device
    { "b203poweroff", 0x500015, "1000", "111111", 0, NULL, 0, "b203" },     //0
    { "b203exit", 0xD00007, "0110", "001111", 0, NULL, 0, "b203" },         //1
    { "b203time", 0x2100021, "0110", "011011", 0, NULL, 0, "b203" },        //2
    { "b203event", 0x210001A, "0110", "110111", 0, NULL, 0, "b203"  },      //3
    { "b203select", 0x210000F, "0110", "010111", 0, NULL, 0, "b203"  },     //4
    { "b203test", 0x2100018, "0110", "101011", 0, NULL, 0, "b203" },        //5
    { "b203prev", 0x210007A, "1000", "000010", 0, NULL, 0, "b203" },        //6
    { "b203next", 0x210007B, "1000", "100010", 0, NULL, 0, "b203" },        //7
    { "b203enter", 0x210007C, "1000", "000110", 0, NULL, 0, "b203" },       //8
    { "b203reset", 0x210003C, NULL, NULL, 2, "RES", 0, "b203" },            //9
    { "b203voldn", 0x500013, "1000", "000010", 0, NULL, 1, "b203" },        //10
    { "b203volup", 0x500012, "1000", "100010", 0, NULL, 1, "b203" },        //11
    { "trueIR", 0x0, NULL, NULL, 1, "R0", 0, "b203"},                       //12
    { "falseIR", 0x0, NULL, NULL, 1, "R1", 0, "b203"},                      //13
    // Amplifier B251
    { "ampweroff", 0x300015, "1000", "111111", 0, NULL, 0, "amplifier" },   //14    
    { "amptape1", 0x30001D, "1000", "111100", 0, NULL, 0, "amplifier" },    //15
    { "amptuner", 0x300018, "1000", "011100", 0, NULL, 0, "amplifier" },    //16
    { "ampphono", 0x300020, "1000", "101100", 0, NULL, 0, "amplifier" },    //17
    { "amptape2", 0x30006C, "1000", "101010", 0, NULL, 0, "amplifier"  },   //18
    { "ampcd", 0x30006E, "1000", "001010", 0, NULL, 0, "amplifier"  },      //19
    { "ampaux", 0x300075, "1000", "001100", 0, NULL, 0, "amplifier" },      //20
    { "amprecmon", 0x30001D, "1000", "100100", 0, NULL, 0, "amplifier" },   //22
    { "amptone", 0x300060, "1000", "010100", 0, NULL, 0, "amplifier" },     //22
    { "amp20db", 0x30004B, "1000", "110100", 0, NULL, 0, "amplifier" },     //23
    { "ampbal_r", 0x1100034, "1000", "010010", 0, NULL, 1, "amplifier" },   //24
    { "ampbal_l", 0x1100033, "1000", "110010", 0, NULL, 1, "amplifier" },   //25
    { "ampvoldn", 0x2100013, "1000", "000010", 0, NULL, 1, "amplifier" },   //26
    { "ampvolup", 0x2100012, "1000", "100010", 0, NULL, 1, "ampliifer" },   //27
    { "ampvoldnx", 0xB00051, "1000", "011010", 0, NULL, 1, "amplifier"  },  //28
    { "ampvolupx", 0xB00050, "1000", "111010", 0, NULL, 1, "amplifier"  },  //29
    // Tuner B261
    { "tunpoweroff", 0xA5001C, "1000", "111111", 0, NULL, 0, "tuner" },     //30
    { "tun1", 0xA40001,  "1000", "011101", 0, NULL, 0, "tuner" },           //31
    { "tun2", 0xA40002, "1000", "101101", 0, NULL, 0, "tuner" },            //32
    { "tun3", 0xA40003, "1000", "001101", 0, NULL, 0, "tuner" },            //33
    { "tun4", 0xA40004, "1000", "110101", 0, NULL, 0, "tuner" },                 //34
    { "tun5", 0xA40005, "1000", "010101", 0, NULL, 0, "tuner" },                 //35
    { "tun6", 0xA40006, "1000", "100101", 0, NULL, 0, "tuner" },                 //36
    { "tun7", 0xA40007, "1000", "000101", 0, NULL, 0, "tuner" },                 //37
    { "tun8", 0xA40008, "1000", "111001", 0, NULL, 0, "tuner" },                 //38
    { "tun9", 0xA40009,  "1000", "011001", 0, NULL, 0, "tuner" },                //39
    { "tun10", 0xA40000,  "1000", "101001", 0, NULL, 0, "tuner" },               //40
    { "tun11", 0xA40011,  "1000", "001001", 0, NULL, 0, "tuner" },               //41
    { "tun12", 0xA40010, "1000", "110001", 0, NULL, 0, "tuner" },                //42
    { "tun13", 0xA40013, "1000", "010001", 0, NULL, 0, "tuner" },                //43
    { "tun14", 0xA5009F, "1000", "100001", 0, NULL, 0, "tuner" },                //44
    { "tun15", 0xA50056, "1000", "000001", 0, NULL, 0, "tuner" },                //45
    { "tun16",0xA40043, "1000", "111110", 0, NULL, 0, "tuner" },                 //46
    { "tun17",0xA50040, "1000", "011110", 0, NULL, 0, "tuner" },                 //47
    { "tun18", 0xA50041, "1000", "101110", 0, NULL, 0, "tuner" },                //48
    { "tun19", 0xA50042,  "1000", "001110", 0, NULL, 0, "tuner" },               //49
    { "tun20", 0xA50056,  "1000", "110110", 0, NULL, 0, "tuner" },               //50
    { "tunscanlast", 0xA50041, "1000", "010110", 0, NULL, 0, "tuner" },          //51
    { "tunscannext", 0xA50042, "1000", "100110", 0, NULL, 0, "tuner" },          //52
    { "tunvolup", 0xA5000A, "1000", "100010", 0, NULL, 1, "tuner" },             //53
    { "tunvoldn", 0xA5000B, "1000", "000010", 0, NULL, 1, "tuner" },             //54
    { "tun10on", 0xA4004A, "1000", "000110", 0, NULL, 0, "tuner" },              //55
    { "tunhighblend", 0xA50003, "1000", "011000", 0, NULL, 0, "tuner" },         //56
    { "tunmute", 0xA50059, "1000", "101000", 0, NULL, 0, "tuner" },              //57
    { "tunreccal", 0xA50081, "1000", "001000", 0, NULL, 0, "tuner" },            //58
    { "tunstore", 0xA5005E, "1000", "11000", 0, NULL, 0, "tuner" },              //59
    { "tunmono", 0xA6004C, "1000", "010000", 0, NULL, 1, "tuner" },              //60
    { "tunstonly", 0xA5009E, "1000", "100000", 0, NULL, 1, "tuner" },            //61
    { "tunantenna", 0xA50047, "1000", "000000", 0, NULL, 0, "tuner" },           //62
    // CD Player B226
    { "cdpoweroff", 0x93A0015, "1000", "111111", 0, "O", 0, "cdplayer" },       //63
    { "cd1", 0x93A0000,  "0000", "011101", 0, NULL, 0, "cdplayer" },            //64
    { "cd2", 0x93A0001, "0000", "101101", 0, NULL, 0, "cdplayer" },             //65
    { "cd3", 0x93A0002, "0000", "001101", 0, NULL, 0, "cdplayer" },             //66
    { "cd4", 0x93A0003, "0000", "110101", 0, NULL, 0, "cdplayer" },             //67
    { "cd5", 0x93A0004, "0000", "010101", 0, NULL, 0, "cdplayer" },             //68
    { "cd6", 0x93A0005, "0000", "100101", 0, NULL, 0, "cdplayer" },             //69
    { "cd7", 0x93A0006, "0000", "000101", 0, NULL, 0, "cdplayer" },             //70
    { "cd8", 0x93A0007, "0000", "111001", 0, NULL, 0, "cdplayer" },             //71
    { "cd9", 0x93A0008,  "0000", "011001", 0, NULL, 0, "cdplayer" },            //72
    { "cd0", 0x93A0009,  "0000", "101001", 0, NULL, 0, "cdplayer" },            //73
    { "cdplay", 0x93A0032, "0000", "000100", 0, "P", 0, "cdplayer" },           //74
    { "cdstop", 0x93A000F, "0000", "000110", 0, "S", 0, "cdplayer" },           //75
    { "cdpause", 0x93A0039, "0000", "111101", 0, "W", 0, "cdplayer" },          //76
    { "cdpause_wom", 0xC5A003B, "0000", "110110", 0, NULL, 0, "cdplayer" },     //77
    { "cdpause_on", 0xC5A003A, "0000", "000001", 0, NULL, 0, "cdplayer" },      //78
    { "cdrew", 0x93A0030, "0000", "100011", 0, NULL, 1, "cdplayer" },           //79
    { "cdforw", 0x93A0023, "0000", "000011", 0, NULL, 1, "cdplayer" },          //80
    { "cdindscan", 0x93A005C, "0000", "100110", 0, "J", 0, "cdplayer" },        //81
    { "cdvolup", 0xC5A002A, "1000", "100010", 0, NULL, 1, "cdplayer" },         //82
    { "cdvoldn", 0xC5A0029, "1000", "000010", 0, NULL, 1, "cdplayer" },         //83
    { "cdpreemph_on", 0x93A0079, "0000", "001001", 0, NULL, 0, "cdplayer" },    //84
    { "cdpreemph_off", 0x93A007A, "0000", "101001", 0, NULL, 0, "cdplayer" },   //85
    { "cdlocal_time", 0x93A0028,  "0000", "010010", 0, "T", 0, "cdplayer" },    //86
    { "cdtotal_time", 0x93A000B, "0000", "100001", 0, "D", 0, "cdplayer" },     //87
    { "cdlocate", 0x93A000B, "0000", "111110", 0, "H", 0, "cdplayer" },         //88
    { "cdautostp_on", 0x93A000E, "0000", "101110", 0, "U", 0, "cdplayer" },     //89
    { "cdautostp_off", 0x93A0022, "0000", "001110", 0, "V", 0, "cdplayer" },    //90
    { "cdload", 0x93A0016, "0000", "011110", 0, "E", 0, "cdplayer" },           //91
    // Tape1 Reel to Reel B77
    { "tape1poweroff", 0xE000F, "1000", "111111", 0, "NULL", 0, "tape1" },      //92
    { "tape1play",  0x100023,  "1000", "101011", 0, "P", 0, "tape1" },          //93
    { "tape1stop",	0xE0018, "1000", "001111", 0, "S", 0, "tape1" },            //94
    { "tape1pause",	0xE0019, "1000", "011011", 0, "W", 0, "tape1" },            //95
    { "tape1playrec",	0xE0020, "1000", "101111", 0, "R", 0, "tape1" },        //96
    { "tape1pauserec",	0xE0030, "1000", "011111", 0, NULL, 0, "tape1" },       //97
    { "tape1rew",	0xE001B, "1000", "110111", 0, "B", 0, "tape1" },            //98
    { "tape1forw",	0xE001C, "1000", "010111", 0, "F", 0, "tape1" },            //98
    { "tape1volup", 0xE001A, "1000", "100010", 0, NULL, 1, "tape1" },           //100
    { "tape1voldn", 0xE004B, "1000", "000010", 0, NULL, 1, "tape1" },           //101
    { "tape1mon",	0xE000E, "1000", "100111", 0, NULL, 0, "tape1" },           //102
    // Tape2 Kassettendeck B215
    { "tape2poweroff", 0xE0016, "1000", "111111", 0, "O", 0, "tape2" },         //103
    { "tape2play",  0x100032,  "0000", "101011", 0, "P", 0, "tape2" },          //104
    { "tape2stop",	0x100038, "0000", "001111", 0, "S", 0, "tape2" },           //105
    { "tape2pause",	0x100039, "0000", "011011", 0, "W", 0, "tape2" },           //106
    { "tape2playrec",	0x100036, "0000", "101111", 0, "R", 0, "tape2" },       //107
    { "tape2pauserec",	0xE001F, "0000", "011111", 0, "V", 0, "tape2" },        //108
    { "tape2rew",	0x100033, "0000", "110111", 0, "B", 0, "tape2" },           //109
    { "tape2forw",	0x100034, "0000", "010111", 0, "F", 0, "tape2" },           //110
    { "tape2volup", 0xE0042, "1000", "100010", 0, NULL, 1, "tape2" },           //111
    { "tape2voldn", 0xE0043, "1000", "000010", 0, NULL, 1, "tape2" },           //112
    { "tape2mon",	0xE0017, "0000", "100111", 0, NULL, 0, "tape2" },           //113
    { "tape2loc1",	0xE0040, "0000", "000111", 0, "Q", 0, "tape2" },            //114
    { "tape2loc2",	0xE0041, "0000", "111011", 0, "Z", 0, "tape2" },            //115
    { "tape2loop",	0x100037, "0000", "001011", 0, "L", 0, "tape2" },           //116
    // Phono Plattenspieler B291
    { "phonopoweroff", 0x540015, "1000", "111111", 0, NULL, 0, "phono" },       //117
    { "phonorew",  0x540034,  "0000", "100011", 0, NULL, 0, "phono" },          //118
    { "phonoforw",	0x540033, "0000", "000011", 0, NULL, 0, "phono" },          //119
    { "phonovoldn", 0x540012, "1000", "000010", 0, NULL, 1, "phono" },          //120
    { "phonovolup", 0x540013, "1000", "100010", 0, NULL, 1, "phono" },          //121
    { "phonolift",	0x540036, "0000", "111101", 0, NULL, 0, "phono" },          //122
    { NULL, 0x0, NULL,   NULL, 0, NULL, 0, NULL }
};

const int cmdTableSize = sizeof(cmdTable) / sizeof(cmdTable[0]);

#endif