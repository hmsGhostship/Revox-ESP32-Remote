#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h> // Wichtig für byte, PROGMEM etc.

struct portcnf {
  char name[5];
  char descr[10];
  char out[3];
  bool feedback;
};

struct command {
    const char* btnID;
    uint32_t irRecvCode;
    uint8_t address;
    uint8_t command;
    uint8_t cmdFlag;
    const char* serCmd;
    bool repeat;
    const char* device;
};

static const command cmdTable [] = {
    // Timer Controller B203
    // btnID, irRecvCode, address,  ITTcode,  cmdFlag, serCmd,  once, device
    // btnID = Button ID
    // irRecvCode == IR code of the transmitting remote control
    // address = IR/SerielLink ITT-address 
    // command = IR/SerialLink ITT-Code
    // cmdFlag 0= CMD disabled, 1 = send Port+CMD // SerialLink 0 = enabled, 1 (or >0) = disabled
    // serCmd = Serial Command
    // repeat 0 = IR command send once, 1 = IR command send repeated
    // device = Description of the Device
    { "b203poweroff", 0x500015, 0x0F, 0x00, 0, NULL, 0, "b203" },     //0
    { "b203exit", 0xD00007, 0x0A, 0x03, 0, NULL, 0, "b203" },         //1
    { "b203time", 0x2100021, 0x0A, 0x09, 0, NULL, 0, "b203" },        //2
    { "b203event", 0x210001A, 0x0A, 0x04, 0, NULL, 0, "b203"  },      //3
    { "b203select", 0x210000F, 0x0A, 0x05, 0, NULL, 0, "b203"  },     //4
    { "b203test", 0x2100018, 0x0A, 0x0A, 0, NULL, 0, "b203" },        //5
    { "b203prev", 0x210007A, 0x0F, 0x2F, 0, NULL, 0, "b203" },        //6
    { "b203next", 0x210007B, 0x0F, 0x2E, 0, NULL, 0, "b203" },        //7
    { "b203enter", 0x210007C, 0x0F, 0x27, 0, NULL, 0, "b203" },       //8
    { "b203reset", 0x210003C, 0x11, 0x40, 2, "RES", 0, "b203" },            //9
    { "trueIR", 0x0, 0x11, 0x40, 1, "R0", 0, "b203"},                       //12
    { "falseIR", 0x0, 0x11, 0x40, 1, "R1", 0, "b203"},                      //13
    // Amplifier B251
    { "ampweroff", 0x300015, 0x0F, 0x00, 0, NULL, 0, "amplifier" },   //14    
    { "amptape1", 0x30001D, 0x0F, 0x30, 0, NULL, 0, "amplifier" },    //15
    { "amptuner", 0x300018, 0x0F, 0x31, 0, NULL, 0, "amplifier" },    //16
    { "ampphono", 0x300020, 0x0F, 0x32, 0, NULL, 0, "amplifier" },    //17
    { "amptape2", 0x30006C, 0x0F, 0x2A, 0, NULL, 0, "amplifier"  },   //18
    { "ampcd", 0x30006E, 0x0F, 0x2B, 0, NULL, 0, "amplifier"  },      //19
    { "ampaux", 0x300075, 0x0F, 0x33, 0, NULL, 0, "amplifier" },      //20
    { "amprecmon", 0x300002, 0x0F, 0x36, 0, NULL, 0, "amplifier" },        //21
    { "amptone", 0x300060, 0x0F, 0x35, 0, NULL, 0, "amplifier" },     //22
    { "amp20db", 0x30004B, 0x0F, 0x34, 0, NULL, 0, "amplifier" },     //23
    { "ampbal_r", 0x1100034, 0x0F, 0x2D, 0, NULL, 1, "amplifier" },   //24
    { "ampbal_l", 0x1100033, 0x0F, 0x2C, 0, NULL, 1, "amplifier" },   //25
    { "ampvoldn", 0x300013, 0x0F, 0x2F, 0, NULL, 1, "amplifier" },   //26
    { "ampvolup", 0x300012, 0x0F, 0x2E, 0, NULL, 1, "amplifier" },   //27
    { "ampvoldnx", 0xB00051, 0x0F, 0x29, 0, NULL, 1, "amplifier"  },  //28
    { "ampvolupx", 0xB00050, 0x0F, 0x28, 0, NULL, 1, "amplifier"  },  //29
    // Tuner B261
    { "tunpoweroff", 0xA5001C, 0x0F, 0x00, 0, NULL, 0, "tuner" },     //30
    { "tun1", 0xA40001,  0x0F, 0x11, 0, NULL, 0, "tuner" },           //31
    { "tun2", 0xA40002, 0x0F, 0x12, 0, NULL, 0, "tuner" },            //32
    { "tun3", 0xA40003, 0x0F, 0x13, 0, NULL, 0, "tuner" },            //33
    { "tun4", 0xA40004, 0x0F, 0x14, 0, NULL, 0, "tuner" },                 //34
    { "tun5", 0xA40005, 0x0F, 0x15, 0, NULL, 0, "tuner" },                 //35
    { "tun6", 0xA40006, 0x0F, 0x16, 0, NULL, 0, "tuner" },                 //36
    { "tun7", 0xA40007, 0x0F, 0x17, 0, NULL, 0, "tuner" },                 //37
    { "tun8", 0xA40008, 0x0F, 0x18, 0, NULL, 0, "tuner" },                 //38
    { "tun9", 0xA40009,  0x0F, 0x19, 0, NULL, 0, "tuner" },                //39
    { "tun10", 0xA40000,  0x0F, 0x1A, 0, NULL, 0, "tuner" },               //40
    { "tun11", 0xA40011,  0x0F, 0x1B, 0, NULL, 0, "tuner" },               //41
    { "tun12", 0xA40010, 0x0F, 0x1C, 0, NULL, 0, "tuner" },                //42
    { "tun13", 0xA40013, 0x0F, 0x1D, 0, NULL, 0, "tuner" },                //43
    { "tun14", 0xA5009F, 0x0F, 0x1E, 0, NULL, 0, "tuner" },                //44
    { "tun15", 0xA50056, 0x0F, 0x1F, 0, NULL, 0, "tuner" },                //45
    { "tun16",0xA40043, 0x0F, 0x20, 0, NULL, 0, "tuner" },                 //46
    { "tun17",0xA50040, 0x0F, 0x21, 0, NULL, 0, "tuner" },                 //47
    { "tun18", 0xA50041, 0x0F, 0x22, 0, NULL, 0, "tuner" },                //48
    { "tun19", 0xA50042,  0x0F, 0x23, 0, NULL, 0, "tuner" },               //49
    { "tun20", 0xA50056,  0x0F, 0x24, 0, NULL, 0, "tuner" },               //50
    { "tunscanlast", 0xA50041, 0x0F, 0x25, 0, NULL, 0, "tuner" },          //51
    { "tunscannext", 0xA50042, 0x0F, 0x26, 0, NULL, 0, "tuner" },          //52
    { "tunvolup", 0xA5000A, 0x0F, 0x2E, 0, NULL, 1, "tuner" },             //53
    { "tunvoldn", 0xA5000B, 0x0F, 0x2F, 0, NULL, 1, "tuner" },             //54
    { "tun10on", 0xA4004A, 0x0F, 0x27, 0, NULL, 0, "tuner" },              //55
    { "tunhighblend", 0xA50003, 0x0F, 0x39, 0, NULL, 0, "tuner" },         //56
    { "tunmute", 0xA50059, 0x0F, 0x3A, 0, NULL, 0, "tuner" },              //57
    { "tunreccal", 0xA50081, 0x0F, 0x3B, 0, NULL, 0, "tuner" },            //58
    { "tunstore", 0xA5005E, 0x0F, 0x3C, 0, NULL, 0, "tuner" },              //59
    { "tunmono", 0xA6004C, 0x0F, 0x3D, 0, NULL, 0, "tuner" },              //60
    { "tunstonly", 0xA5009E, 0x0F, 0x3E, 0, NULL, 0, "tuner" },            //61
    { "tunantenna", 0xA50047, 0x0F, 0x3F, 0, NULL, 0, "tuner" },           //62
    // CD Player B226
    { "cdpoweroff", 0x93A0015, 0x0F, 0x00, 0, "O", 0, "cdplayer" },       //63
    { "cd1", 0x93A0000,  0x10, 0x11, 0, NULL, 0, "cdplayer" },            //64
    { "cd2", 0x93A0001, 0x10, 0x12, 0, NULL, 0, "cdplayer" },             //65
    { "cd3", 0x93A0002, 0x10, 0x13, 0, NULL, 0, "cdplayer" },             //66
    { "cd4", 0x93A0003, 0x10, 0x14, 0, NULL, 0, "cdplayer" },             //67
    { "cd5", 0x93A0004, 0x10, 0x15, 0, NULL, 0, "cdplayer" },             //68
    { "cd6", 0x93A0005, 0x10, 0x16, 0, NULL, 0, "cdplayer" },             //69
    { "cd7", 0x93A0006, 0x10, 0x17, 0, NULL, 0, "cdplayer" },             //70
    { "cd8", 0x93A0007, 0x10, 0x18, 0, NULL, 0, "cdplayer" },             //71
    { "cd9", 0x93A0008,  0x10, 0x19, 0, NULL, 0, "cdplayer" },            //72
    { "cd0", 0x93A0009,  0x10, 0x1A, 0, NULL, 0, "cdplayer" },            //73
    { "cdplay", 0x93A0032, 0x10, 0x037, 0, "P", 0, "cdplayer" },           //74
    { "cdstop", 0x93A000F, 0x10, 0x27, 0, "S", 0, "cdplayer" },           //75
    { "cdpause", 0x93A0039, 0x10, 0x10, 0, "W", 0, "cdplayer" },          //76
    { "cdpause_wom", 0xC5A003B, 0x10, 0x24, 0, NULL, 0, "cdplayer" },     //77
    { "cdpause_on", 0xC5A003A, 0x10, 0x1F, 0, NULL, 0, "cdplayer" },      //78
    { "cdrew", 0x93A0030, 0x10, 0x0E, 0, NULL, 1, "cdplayer" },           //79
    { "cdforw", 0x93A0023, 0x10, 0x0F, 0, NULL, 1, "cdplayer" },          //80
    { "cdindscan", 0x93A005C, 0x10, 0x26, 0, "J", 0, "cdplayer" },        //81
    { "cdvolup", 0xC5A002A, 0x0F, 0x2E, 0, NULL, 1, "cdplayer" },         //82
    { "cdvoldn", 0xC5A0029, 0x0F, 0x2F, 0, NULL, 1, "cdplayer" },         //83
    { "cdpreemph_on", 0x93A0079, 0x10, 0x1B, 0, NULL, 0, "cdplayer" },    //84
    { "cdpreemph_off", 0x93A007A, 0x10, 0x1C, 0, NULL, 0, "cdplayer" },   //85
    { "cdlocal_time", 0x93A0028,  0x10, 0x1D, 0, "T", 0, "cdplayer" },    //86
    { "cdtotal_time", 0x93A000B, 0x10, 0x1E, 0, "D", 0, "cdplayer" },     //87
    { "cdlocate", 0x93A000B, 0x10, 0x20, 0, "H", 0, "cdplayer" },         //88
    { "cdautostp_on", 0x93A000E, 0x10, 0x22, 0, "U", 0, "cdplayer" },     //89
    { "cdautostp_off", 0x93A0022, 0x10, 0x23, 0, "V", 0, "cdplayer" },    //90
    { "cdload", 0x93A0016, 0x10, 0x21, 0, "E", 0, "cdplayer" },           //91
    // Tape1 Reel to Reel B77
    { "tape2poweroff", 0xE000F, 0x0F, 0x00, 0, NULL, 0, "tape2" },      //92
    { "tape2play",  0x100023,  0x10, 0x0A, 0, "P", 0, "tape2" },          //93
    { "tape2stop",	0xE0018, 0x10, 0x03, 0, "S", 0, "tape2" },            //94
    { "tape2pause",	0xE0019, 0x10, 0x09, 0, "W", 0, "tape2" },            //95
    { "tape2playrec",	0xE0020, 0x10, 0x02, 0, "R", 0, "tape2" },        //96
    { "tape2pauserec",	0xE0030, 0x10, 0x01, 0, NULL, 0, "tape2" },       //97
    { "tape2rew",	0xE001B, 0x10, 0x04, 0, "B", 0, "tape2" },            //98
    { "tape2forw",	0xE001C, 0x10, 0x05, 0, "F", 0, "tape2" },            //98
    { "tape2volup", 0xE001A, 0x0F, 0x2E, 0, NULL, 1, "tape2" },           //100
    { "tape2voldn", 0xE004B, 0x0F, 0x2F, 0, NULL, 1, "tape2" },           //101
    { "tape2mon",	0xE000E, 0x10, 0x06, 0, NULL, 0, "tape2" },           //102
    // Tape2 Kassettendeck B215
    { "tape1poweroff", 0xE0016, 0x0F, 0x00, 0, "O", 0, "tape1" },         //103
    { "tape1play",  0x100032,  0x0F, 0x0A, 0, "P", 0, "tape1" },          //104
    { "tape1stop",	0x100038, 0x0F, 0x03, 0, "S", 0, "tape1" },           //105
    { "tape1pause",	0x100039, 0x0F, 0x09, 0, "W", 0, "tape1" },           //106
    { "tape1playrec",	0x100036, 0x0F, 0x02, 0, "R", 0, "tape1" },       //107
    { "tape1pauserec",	0xE001F, 0x0F, 0x01, 0, "V", 0, "tape1" },        //108
    { "tape1rew",	0x100033, 0x0F, 0x04, 0, "B", 0, "tape1" },           //109
    { "tape1forw",	0x100034, 0x0F, 0x05, 0, "F", 0, "tape1" },           //110
    { "tape1volup", 0xE0042, 0x0F, 0x2E, 0, NULL, 1, "tape1" },           //111
    { "tape1voldn", 0xE0043, 0x0F, 0x2F, 0, NULL, 1, "tape1" },           //112
    { "tape1mon",	0xE0017, 0x0F, 0x06, 0, NULL, 0, "tape1" },           //113
    { "tape1loc1",	0xE0040, 0x0F, 0x07, 0, "Q", 0, "tape1" },            //114
    { "tape1loc2",	0xE0041, 0x0F, 0x08, 0, "Z", 0, "tape1" },            //115
    { "tape1loop",	0x100037, 0x0F, 0x0B, 0, "L", 0, "tape1" },           //116
    // Phono Plattenspieler B291
    { "phonopoweroff", 0x540015, 0x0F, 0x00, 0, NULL, 0, "phono" },       //117
    { "phonoforw",  0x540034,  0x0F, 0x0F, 0, ">", 1, "phono" },          //118
    { "phonoback",	0x540033, 0x0F, 0x0E, 0, "<", 1, "phono" },           //119
    { "phonovoldn", 0x540012, 0x0F, 0x2F, 0, NULL, 1, "phono" },          //120
    { "phonovolup", 0x540013, 0x0F, 0x2E, 0, NULL, 1, "phono" },          //121
    { "phonolift",	0x540036, 0x0F, 0x10, 0, NULL, 0, "phono" },          //122
     // Receiver B285/B286
    { "recvpoweroff", 0x0, 0x0F, 0x00, 0, "O", 0, "receiver" },       //123
    { "recv1", 0x0,  0x0F, 0x11, 0, "E01", 0, "receiver" },           //124
    { "recv2", 0x0, 0x0F, 0x12, 0, "E02", 0, "receiver" },            //125
    { "recv3", 0x0, 0x0F, 0x13, 0, "E03", 0, "receiver" },            //126
    { "recv4", 0x0, 0x0F, 0x14, 0, "E04", 0, "receiver" },            //127
    { "recv5", 0x0, 0x0F, 0x15, 0, "E05", 0, "receiver" },            //128
    { "recv6", 0x0, 0x0F, 0x16, 0, "E06", 0, "receiver" },            //129
    { "recv7", 0x0, 0x0F, 0x17, 0, "E07", 0, "receiver" },            //130
    { "recv8", 0x0, 0x0F, 0x18, 0, "E08", 0, "receiver" },            //131
    { "recv9", 0x0,  0x0F, 0x19, 0, "E09", 0, "receiver" },           //132
    { "recv0", 0x0,  0x0F, 0x1A, 0, NULL, 0, "receiver" },            //133
    { "recv10", 0x0, 0x11, 0x40, 0, "E10", 0, "receiver" },                //124
    { "recv11", 0x0, 0x11, 0x40, 0, "E11", 0, "receiver" },                //124
    { "recv12", 0x0, 0x11, 0x40, 0, "E12", 0, "receiver" },                 //125
    { "recv13", 0x0, 0x11, 0x40, 0, "E13", 0, "receiver" },                 //126
    { "recv14", 0x0, 0x11, 0x40, 0, "E14", 0, "receiver" },                 //127
    { "recv15", 0x0, 0x11, 0x40, 0, "E15", 0, "receiver" },                 //128
    { "recv16", 0x0, 0x11, 0x40, 0, "E16", 0, "receiver" },                 //129
    { "recv17", 0x0, 0x11, 0x40, 0, "E17", 0, "receiver" },                 //130
    { "recv18", 0x0, 0x11, 0x40, 0, "E18", 0, "receiver" },                 //131
    { "recv19", 0x0, 0x11, 0x40, 0, "E19", 0, "receiver" },                //132
    { "recv20", 0x0, 0x11, 0x40, 0, "E20", 0, "receiver" },                //133
    { "recv21", 0x0, 0x11, 0x40, 0, "E21", 0, "receiver" },                //124
    { "recv22", 0x0, 0x11, 0x40, 0, "E22", 0, "receiver" },                //124
    { "recv23", 0x0, 0x11, 0x40, 0, "E23", 0, "receiver" },                 //125
    { "recv24", 0x0, 0x11, 0x40, 0, "E24", 0, "receiver" },                 //126
    { "recv25", 0x0, 0x11, 0x40, 0, "E25", 0, "receiver" },                 //127
    { "recv26", 0x0, 0x11, 0x40, 0, "E26", 0, "receiver" },                 //128
    { "recv27", 0x0, 0x11, 0x40, 0, "E27", 0, "receiver" },                 //129
    { "recv28", 0x0, 0x11, 0x40, 0, "E28", 0, "receiver" },                 //130
    { "recv29", 0x0, 0x11, 0x40, 0, "E29", 0, "receiver" },                 //131
    { "recvscannext", 0x0, 0x0F, 0x26, 0, "N", 0, "receiver" },       //134
    { "recventer", 0x0, 0x0F, 0x27, 0, "I", 0, "receiver" },          //135
    { "recvtapemon2", 0x0, 0x0F, 0x28, 0, "H", 0, "receiver"  },      //138
    { "recvloudness", 0x0, 0x0F, 0x29, 0, NULL, 0, "receiver"  },     //139
    { "recvtape2", 0x0, 0x0F, 0x2A, 0, "B", 0, "receiver"  },         //140
    { "recvcd", 0x0, 0x0F, 0x2B, 0, "C", 0, "receiver"  },            //141
    { "recvbal_l", 0x0, 0x0F, 0x2C, 0, NULL, 1, "receiver" },         //142
    { "recvbal_r", 0x0, 0x0F, 0x2D, 0, NULL, 1, "receiver" },         //143
    { "recvvolup", 0x0, 0x0F, 0x2E, 0, NULL, 1, "receiver" },         //144
    { "recvvoldn", 0x0, 0x0F, 0x2F, 0, NULL, 1, "receiver" },         //145
    { "recvtape1", 0x0, 0x0F, 0x30, 0, "A", 0, "receiver" },          //146
    { "recvphono", 0x0, 0x0F, 0x32, 0, "D", 0, "receiver" },          //147
    { "recv20db", 0x0, 0x0F, 0x34, 0, NULL, 0, "receiver" },          //148
    { "recvtone", 0x0, 0x0F, 0x35, 0, NULL, 0, "receiver" },          //149
    { "recvtapemon1", 0x0, 0x0F, 0x36, 0, "G", 0, "receiver" },       //150
    { "recvhighblend", 0x0, 0x0F, 0x39, 0, NULL, 0, "receiver" },     //151
    { "recvmute", 0x0, 0x0F, 0x3A, 0, NULL, 0, "receiver" },          //152
    { "recvspeakersa", 0x0, 0x0F, 0x3B, 0, NULL, 0, "receiver" },     //153
    { "recvstore", 0x0, 0x0F, 0x3C, 0, NULL, 0, "receiver" },          //154
    { "recvmono", 0x0, 0x0F, 0x3D, 0, NULL, 0, "receiver" },          //155
    { "recvspeakersb", 0x0, 0x0F, 0x3E, 0, NULL, 0, "receiver" },     //156
    { "recvtestmode", 0x0, 0x0F, 0x3F, 0, NULL, 0, "receiver" },      //157
    { NULL, 0x0, 0x11, 0x40, 0, NULL, 0, NULL }
};

const int cmdTableSize = sizeof(cmdTable) / sizeof(cmdTable[0]);

#endif