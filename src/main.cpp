/*********
  H. Haefner ESP32 IR Remote for Revox B203 and B2xx Series
  Complete project details at 
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

// Import required libraries
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h" 
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#define RXD2 17
#define TXD2 16
#define REVOX_BAUD 1200

//#define DECODE_DENON        // Includes Sharp - requires around 250 bytes of program memory on ATmega328
//#define DECODE_JVC          // ~ 200 bytes
//#define DECODE_KASEIKYO     // Includes Panasonic ~ 300 bytes
//#define DECODE_LG           // ~ 400 bytes
#define DECODE_NEC          // Includes Apple and Onkyo ~ 250 bytes
//#define DECODE_SAMSUNG      // ~ 300 bytes
#define DECODE_SONY         // ~ 175 bytes
//#define DECODE_RC5          // RC5 + MARANTZ: ~ 425 bytes
//#define DECODE_RC6          // ~ 375 bytes


#define USE_LittleFS
#include <FS.h>
#ifdef USE_LittleFS
  #define SPIFFS LITTLEFS
  #include <LITTLEFS.h> 
#else
  #include <SPIFFS.h>
#endif
#include <ArduinoJson.h>
#define DISABLE_LED_FEEDBACK_BLINKING // Spart Strom und GPIOs
#include <IRRemote.hpp>

#include "SerialLink.h"
#include "config.h"
#define IR_RECEIVE_PIN GPIO_NUM_25
#define ENABLE_LOW_POWER 

JsonDocument configDoc;
bool buttonHold = 0;
bool getFlag = 0;
unsigned long previousMillis = 0;
const long interval = 130;
char buttonName[18];
int irid = 0;
String b203Buffer = ""; // Sammelt die einzelnen Zeichen
String b203data = "";   // Hält die letzte fertige Zeile für den WebSocket bereit
String pendingWsCommand = "";     // Speichert den verzögerten Befehl
unsigned long wsWakeupTime = 0;   // Merkt sich, wann das Paket ankam
const int PIN_RECV = 25;
const int PIN_CTS = 26;
const int PIN_RTS = 27;
const int OE_FXMA108 = 0;
bool Set_OE_FXMA108 = true;
const char WificonfigPath[] = "/wifi_config.json";
const char PortConfigPath[] = "/portconfig.json";
const char ConfigPath[] = "/config.json";
const char ssid_ap[] = "REVOXSETUP";
const char password_ap[] = "esp32revox";
String wifi_ssid = "";
String wifi_pass = "";
const char hostName[] = "revoxb203"; // Ihr Wunsch-Hostname
String htmlPath = "/amplifier.html";

const int maxPortEntries = 20;               // Maximale RAM-Kapazität des Arrays
portcnf portArray[maxPortEntries];           // Das feste Speicher-Array im RAM
int portTableSize = 0;

const int maxCommandCapacity = 200; // Anpassen an Ihre maximale Zeilenanzahl
command configArray[maxCommandCapacity];
int configTableSize = 0;

unsigned long lastPingTime = 0;
const unsigned long pingInterval = 10000; // Alle 10 Sekunden pingen

unsigned long lastActivity = 0;

bool b203ReadyToSend = true; // Steuert den XON/XOFF Fluss


unsigned long lastButtonRepeatTime = 0;
bool isFirstButtonPress = true; // Globaler Merker für alle Funktionen

bool blockSerialSending = false;
unsigned long serialBlockStartTime = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


// 1. Sichere Port-Prüfung (Verhindert Speicher-Fehlzugriffe)
int portExists(const char* searchName) {
    if (searchName == NULL || searchName[0] == '\0') return 0;
    
    for (size_t i = 0; i < portTableSize; i++) {
        // KORRIGIERT: Prüft, ob der Eintrag im RAM überhaupt befüllt ist (erstes Zeichen nicht \0)
        if (portArray[i].name[0] != '\0' && strcmp(portArray[i].name, searchName) == 0) {
            return 1; // Gefunden!
        }
    }
    return 0; // Nicht gefunden
}

// 2. Speicheroptimierte Konfigurations-Ladefunktion
void loadPortConfig() {
  if (!LittleFS.exists(PortConfigPath)) {
    Serial.println(F("Port-Konfiguration existiert nicht im Dateisystem!"));
    return;
  }

  File file = LittleFS.open(PortConfigPath, "r");
  if (!file) {
    Serial.println(F("WARNUNG: Port Configuration blockiert! Lade-Vorgang abgebrochen."));
    return;
  }
  
  if (file.size() == 0) {
    Serial.println(F("WARNUNG: portConfig.json ist aktuell 0 Bytes groß. Lade-Vorgang abgebrochen."));
    file.close();
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print(F("Fehler beim Parsen der JSON: "));
    Serial.println(error.f_str());
    file.close();
    return;
  }

  file.close(); 
  
  JsonArray array = doc.as<JsonArray>();
  if (array.isNull() || array.size() == 0) {
    Serial.println("WARNUNG: Gelesenes JSON-Array ist leer! RAM wird nicht ueberschrieben.");
    return;
  }

  int maxArrayCapacity = sizeof(portArray) / sizeof(portArray[0]);
  int temporaereGroesse = 0;

  if (array.size() > maxArrayCapacity) {
    temporaereGroesse = maxArrayCapacity;
    Serial.print(F("Warnung: JSON zu groß! Begrenzt auf: "));
    Serial.println(maxArrayCapacity);
  } else {
    temporaereGroesse = array.size();
  }
  
  int index = 0;
  for (JsonObject obj : array) {
    if (index >= temporaereGroesse) break;

    strncpy(portArray[index].name, obj["name"] | "", sizeof(portArray[index].name) - 1);
    portArray[index].name[sizeof(portArray[index].name) - 1] = '\0';

    strncpy(portArray[index].descr, obj["descr"] | "", sizeof(portArray[index].descr) - 1);
    portArray[index].descr[sizeof(portArray[index].descr) - 1] = '\0';
    
    String outString = "";
    if (obj["out"].is<int>()) {
      outString = String(obj["out"].as<int>());
    } else {
      outString = obj["out"] | "";
    }
    
    strncpy(portArray[index].out, outString.c_str(), sizeof(portArray[index].out) - 1);
    portArray[index].out[sizeof(portArray[index].out) - 1] = '\0';
    
    portArray[index].feedback = obj["feedback"] | false;

    // KORRIGIERT: printf-Texte in den Flash-Speicher verschoben (RAM entlastet)
    Serial.printf(PSTR("RAM-Port geladen [%d]: %s (%s) -> Port: %s\n"), 
                  index, portArray[index].name, portArray[index].descr, portArray[index].out);

    index++;
  }

  portTableSize = temporaereGroesse;
  Serial.print(F("Erfolgreich geladen. Aktuelle portTableSize: "));
  Serial.println(portTableSize);
}

void loadCommandConfig() {
  // Datei öffnen
  File file = LittleFS.open(ConfigPath, "r");
  if (!file) {
    Serial.println("Fehler: /config.json konnte nicht geöffnet werden");
    return;
  }

  // Altes Dokument leeren, falls die Funktion mehrfach aufgerufen wird
  configDoc.clear();

  // Datei parsen
  DeserializationError error = deserializeJson(configDoc, file);
  if (error) {
    Serial.print(F("Fehler beim Parsen von /config.json: "));
    Serial.println(error.f_str());
    file.close();
    return;
  }

  file.close(); // Datei schließen

  // Das geparste JSON als Array auslesen
  JsonArray array = configDoc.as<JsonArray>();

  // Schutz vor Speicherüberlauf (Buffer Overflow)
  if (array.size() > maxCommandCapacity) {
    configTableSize = maxCommandCapacity;
    Serial.print(F("Warnung: config.json zu groß! Begrenzt auf: "));
    Serial.println(maxCommandCapacity);
  } else {
    configTableSize = array.size();
  }

  // Array mit den echten Daten befüllen
  int index = 0;
  for (JsonObject obj : array) {
    if (index >= configTableSize) break;

    // 1. Strings sicher kopieren mit strlcpy
    strlcpy(configArray[index].btnID, obj["btnID"] | "", sizeof(configArray[index].btnID));
    strlcpy(configArray[index].serCmd, obj["serCmd"] | "", sizeof(configArray[index].serCmd));
    strlcpy(configArray[index].device, obj["device"] | "", sizeof(configArray[index].device));
    
    // bibusCmd ist im struct ein char[8], daher ebenfalls strlcpy nutzen!
    strlcpy(configArray[index].bibusCmd, obj["bibusCmd"] | "", sizeof(configArray[index].bibusCmd));

    // 2. Numerische Werte (Hex-Strings) konvertieren und zuweisen
    const char* irStr = obj["irRecvCode"] | "0";
    configArray[index].irRecvCode = strtol(irStr, nullptr, 16);

    const char* addrStr = obj["address"] | "0";
    configArray[index].address = (uint8_t)strtol(addrStr, nullptr, 16);

    const char* addrRepStr = obj["addressRep"] | "0";
    configArray[index].addressRep = (uint8_t)strtol(addrRepStr, nullptr, 16);

    const char* cmdStr = obj["command"] | "0";
    configArray[index].command = (uint8_t)strtol(cmdStr, nullptr, 16);

    // 3. Normale numerische und boolesche Werte direkt zuweisen
    configArray[index].cmdFlag    = obj["cmdFlag"] | 0;
    configArray[index].repeat     = obj["repeat"] | false;
    configArray[index].isBibus    = obj["isBibus"] | false;

    // --- OPTIONAL: Debug-Ausgabe zur Kontrolle im Seriellen Monitor ---
    Serial.print(F("ID: "));
    Serial.print(configArray[index].btnID);
    Serial.print(F(" -> IR: 0x"));
    Serial.print(configArray[index].irRecvCode, HEX);
    Serial.print(F(" | Addr: 0x"));
    Serial.print(configArray[index].address, HEX);
    Serial.print(F(" | AddrRep: 0x"));
    Serial.print(configArray[index].addressRep, HEX);
    Serial.print(F(" | command: 0x"));
    Serial.print(configArray[index].command, HEX);
    Serial.print(F(" | isBiBus: "));
    Serial.print(configArray[index].isBibus ? F("true") : F("false"));
    Serial.print(F(" | BiBusCmd: "));
    Serial.print(configArray[index].bibusCmd); // Als Text ausgeben, da String!
    Serial.print(F(" | cmdFlag: "));
    Serial.println(configArray[index].cmdFlag);

    index++;
  }

  // Kontrollausgabe im Seriellen Monitor
  Serial.print(F("Erfolgreich geladen. Aktuelle configTableSize: "));
  Serial.println(configTableSize);
}

void processButtonPath() {
  if (buttonHold == 1) {
      
      // SPERRWALL MIT DETAIL-LOGGING
      if (isFirstButtonPress) {
          if (millis() - lastButtonRepeatTime < 350) {
              return; // Schranke ist noch zu, Schleife abbrechen
          }
      } 
      else {
          if (millis() - lastButtonRepeatTime < 200) {
              return; 
          }
      }

      // DETAILED LOG: Wird nur gedruckt, wenn die Funktion die 350ms/200ms-Drossel passiert
      Serial.println(F("\n--------------------------------------------------"));
      Serial.print(F("[LOOP-LOG] processButtonPath() triggert fuer Taste: '")); Serial.print(buttonName); Serial.println(F("'"));
      Serial.print(F("[LOOP-LOG] Modus: ")); Serial.println(isFirstButtonPress ? F("ERST-DURCHLAUF (First Press Bremse)") : F("WIEDERHOLUNG (Button Hold)"));

      int a = 0;
      bool buttonFound = false;

      // KORRIGIERT: Mit [0] fuer die sichere Array-Inhaltspruefung
      while (configArray[a].btnID[0] != '\0' && strcmp(configArray[a].btnID, "none") != 0 && a < maxCommandCapacity) {
          
          if (strcmp(buttonName, configArray[a].btnID) == 0) {
              buttonFound = true;
              lastButtonRepeatTime = millis();

              for (int i = 0; i < portTableSize; i++) {
                  String currentOut = portArray[i].out;
                  String currentDescr = portArray[i].descr;
                  
                  bool deviceMatch = (strcmp(configArray[a].device, currentDescr.c_str()) == 0) || 
                                     (strstr(configArray[a].device, currentDescr.c_str()) != NULL);

                  if (deviceMatch && currentOut != "no") {
                      
                      if (strcmp(configArray[a].btnID, "b203reset") == 0) {
                          Serial.println(F("[LOOP-LOG] -> Verarbeite 'b203reset'"));
                          if (configArray[a].command != 0x40) {
                              sendRevoxFrame(configArray[a].address, configArray[a].command, 1);
                          }
                          if (configArray[a].repeat == 0) { 
                              buttonHold = 0; 
                              isFirstButtonPress = true; 
                          }
                      }
                      else if (configArray[a].cmdFlag > 0) {
                          Serial.print(F("[LOOP-LOG] -> Sende seriellen String-Befehl via Serial2: ")); Serial.println(configArray[a].serCmd);
                          if (b203ReadyToSend) { 
                              Serial2.print(currentOut);
                              Serial2.print(configArray[a].serCmd);
                              Serial2.print("\r");
                          } else {
                              Serial.println(F("[LOOP-LOG] ACHTUNG: Serial2 blockiert durch XOFF!"));
                          }
                          if (configArray[a].repeat == 0) { 
                              buttonHold = 0; 
                              isFirstButtonPress = true; 
                          }
                      } 
                      else if (configArray[a].cmdFlag == 0) {
                          if (configArray[a].command != 0x40) { 
                              
                              if (configArray[a].isBibus == 1) {
                                  if (b203ReadyToSend) { 
                                      // KORREKTUR: Als Arrays mit Puffergroesse definiert, loest den Compiler-Fehler
                                      char sendBuffer[64]; 
                                      char bibusFormatiert[16]; 
                                      
                                      const char* bibusPtr = configArray[a].bibusCmd;
                                      if (strncmp(bibusPtr, "0x", 2) == 0 || strncmp(bibusPtr, "0X", 2) == 0) { bibusPtr += 2; }
                                      
                                      int len = strlen(bibusPtr);
                                      if (len >= 5) { snprintf(bibusFormatiert, sizeof(bibusFormatiert), "%s", bibusPtr + (len - 5)); } 
                                      else { snprintf(bibusFormatiert, sizeof(bibusFormatiert), "%05X", (unsigned int)strtol(bibusPtr, NULL, 16)); }
                                      
                                      snprintf(sendBuffer, sizeof(sendBuffer), "B%s\r", bibusFormatiert);
                                      
                                      Serial.print(F("[LOOP-LOG] -> Sende BIBUS Befehl: ")); Serial.print(sendBuffer);
                                      Serial2.print(sendBuffer);
                                  }
                              } else {
                                  // NATIVE REVOX SENDUNG
                                  if (isFirstButtonPress) {
                                      Serial.print(F("[LOOP-LOG] -> Sende Standard-Frame (First): Addr 0x")); Serial.println(configArray[a].address, HEX);
                                      sendRevoxFrame(configArray[a].address, configArray[a].command, 1);
                                      isFirstButtonPress = false; 
                                  } else {
                                      if (configArray[a].addressRep != 0) {
                                          Serial.print(F("[LOOP-LOG] -> Sende REPEAT-Frame (addressRep): Addr 0x")); Serial.println(configArray[a].addressRep, HEX);
                                          sendRevoxFrame(configArray[a].addressRep, configArray[a].command, 1);
                                      } else {
                                          Serial.print(F("[LOOP-LOG] -> Sende REPEAT-Frame (Standard): Addr 0x")); Serial.println(configArray[a].address, HEX);
                                          sendRevoxFrame(configArray[a].address, configArray[a].command, 1);
                                      }
                                  }
                              }
                              
                              if (configArray[a].repeat == 0) {
                                  Serial.println(F("[LOOP-LOG] Info: Taste unterstuezt kein Halten (repeat=0). Beende Befehl."));
                                  buttonHold = 0;
                                  isFirstButtonPress = true; 
                              }
                          }
                      }
                  }
              }
              break; 
          }
          ++a;
      }
      
      if (!buttonFound) { 
          Serial.print(F("[LOOP-LOG] FEHLER: ID '")); Serial.print(buttonName); Serial.println(F("' wurde im Loop-Schleifendurchlauf verpasst!"));
          buttonHold = 0; 
          isFirstButtonPress = true; 
      }
      Serial.println(F("--------------------------------------------------\n"));
  }
}

void processDirectCommands() {
    // Buttons wurden komplett entfernt – hier landen nur noch Slider und Setup!
    if (pendingWsCommand.length() > 0 && (millis() - wsWakeupTime >= 50)) {
        
        if (!b203ReadyToSend) {
            Serial.println(F("[DIRECT-LOG] Direkt-Befehl wartet, da B203 im XOFF-Status ist!"));
            return; 
        }

        String msg = pendingWsCommand;
        pendingWsCommand = ""; 
        lastActivity = millis();

        // DETAILED LOG: Eingangsprüfung für Slider & Setup
        Serial.println(F("\n=================================================="));
        Serial.print(F("[DIRECT-LOG] DIREKT-BEFEHL VERARBEITUNG | Inhalt: '")); Serial.print(msg); Serial.println(F("'"));
        Serial.println(F("--------------------------------------------------"));

        if (msg.startsWith("speakers")) {
            // ZURÜCKGESETZT: Ganz normaler String-Puffer ohne Zahlenformatierung
            char b285Speaker[8] = {0}; 
            msg.substring(8).toCharArray(b285Speaker, sizeof(b285Speaker));

            Serial.print(F("[DIRECT-LOG] Typ: speakers | Wert: '")); Serial.print(b285Speaker); Serial.println(F("'"));
            for (int i = 0; i < portTableSize; i++) {
                if ((strcmp("receiver", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
                    Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '"));
                    Serial.print(portArray[i].out);
                    Serial.print(F("L"));
                    Serial.print(b285Speaker);
                    Serial.println(F("\\r'"));
                    Serial2.print(portArray[i].out);
                    Serial2.print("L");
                    Serial2.print(b285Speaker);
                    Serial2.print("\r");
                }
            }
        }
        else if (msg.startsWith("volSlider")) {
            char b285Volume[8] = {0}; 
            // 10 statt 9, um das 'V' aus dem WebSocket-Inhalt abzuschneiden
            msg.substring(10).toCharArray(b285Volume, sizeof(b285Volume));
            
            // Führende Null für die Lautstärke erzwingen (00-99)
            int volNum = atoi(b285Volume);
            char b285VolumeFormatiert[4] = {0}; 
            snprintf(b285VolumeFormatiert, sizeof(b285VolumeFormatiert), "%02d", volNum);

            Serial.print(F("[DIRECT-LOG] Typ: volSlider | Formatiert: '")); Serial.print(b285VolumeFormatiert); Serial.println(F("'"));
            for (int i = 0; i < portTableSize; i++) {
                if ((strcmp("receiver", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
                    
                    // KORREKTUR: Hier wird das 'V' nun explizit in die Sende-Kette eingefügt!
                    Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); 
                    Serial.print(portArray[i].out); 
                    Serial.print(F("V")); // Zeigt das V im Log an
                    Serial.print(b285VolumeFormatiert); 
                    Serial.println(F("\\r'"));
                    
                    // Der physische Sende-Befehl an den B203
                    Serial2.print(portArray[i].out); 
                    Serial2.print('V'); // Schickt das V an den B203
                    Serial2.print(b285VolumeFormatiert); 
                    Serial2.print("\r");
                }
            }
        }
        else if (msg.startsWith("setup")) {
            char setupBytes[12] = {0}; msg.substring(5).toCharArray(setupBytes, sizeof(setupBytes));
            Serial.print(F("[DIRECT-LOG] Typ: setup | Parameter: '")); Serial.print(setupBytes); Serial.println(F("'"));
            Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(setupBytes); Serial.println(F("\\r'"));
            Serial2.print(setupBytes); Serial2.print("\r");
        }
        else if (msg.startsWith("getsettings")) {
            char settingsBytes[8] = {0}; msg.substring(11).toCharArray(settingsBytes, sizeof(settingsBytes));
            Serial.print(F("[DIRECT-LOG] Typ: getsettings | Modus: '")); Serial.print(settingsBytes); Serial.println(F("'"));
            Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(settingsBytes); Serial.println(F("\\r'"));
            Serial2.print(settingsBytes); Serial2.print("\r");
            if (strcmp(settingsBytes, "0X") == 0) { 
                getFlag = 1; 
                Serial.println(F("[DIRECT-LOG] Info: getFlag wurde auf 1 gesetzt."));
            }
        }
        else if (msg.startsWith("tape1")) {
            char b215settingsBytes[6] = {0}; msg.substring(5).toCharArray(b215settingsBytes, sizeof(b215settingsBytes));
            Serial.print(F("[DIRECT-LOG] Typ: tape1 | Befehlsbytes: '")); Serial.print(b215settingsBytes); Serial.println(F("'"));
            for (int i = 0; i < portTableSize; i++) {
                if ((strcmp("tape1", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
                    Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(portArray[i].out); Serial.print(b215settingsBytes); Serial.println(F("\\r'"));
                    Serial2.print(portArray[i].out); Serial2.print(b215settingsBytes); Serial2.print("\r");
                }
            }
        }
        else if (msg.startsWith("cdplayer")) {
            char b226settingsBytes[6] = {0}; msg.substring(8).toCharArray(b226settingsBytes, sizeof(b226settingsBytes));
            Serial.print(F("[DIRECT-LOG] Typ: cdplayer | Befehlsbytes: '")); Serial.print(b226settingsBytes); Serial.println(F("'"));
            for (int i = 0; i < portTableSize; i++) {
                if ((strcmp("cdplayer", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
                    Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(portArray[i].out); Serial.print(b226settingsBytes); Serial.println(F("\\r'"));
                    Serial2.print(portArray[i].out); Serial2.print(b226settingsBytes); Serial2.print("\r");
                }
            }
        }
        else if (msg.startsWith("phono")) {
            char b291settingsBytes[6] = {0}; msg.substring(5).toCharArray(b291settingsBytes, sizeof(b291settingsBytes));
            Serial.print(F("[DIRECT-LOG] Typ: phono | Befehlsbytes: '")); Serial.print(b291settingsBytes); Serial.println(F("'"));
            for (int i = 0; i < portTableSize; i++) {
                if ((strcmp("phono", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
                    Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(portArray[i].out); Serial.print(b291settingsBytes); Serial.println(F("\\r'"));
                    Serial2.print(portArray[i].out); Serial2.print(b291settingsBytes); Serial2.print("\r");
                }
            }
        }
        else if (msg.startsWith("receiver")) {
            char b285settingsBytes[6] = {0};
            msg.substring(8).toCharArray(b285settingsBytes, sizeof(b285settingsBytes));

            Serial.print(F("[DIRECT-LOG] Typ: receiver | Befehlsbytes: '"));
            Serial.print(b285settingsBytes);
            Serial.println(F("'"));

            for (int i = 0; i < portTableSize; i++) {
                if ((strcmp("receiver", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
                    Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '"));
                    Serial.print(portArray[i].out); Serial.print(b285settingsBytes);
                    Serial.println(F("\\r'"));
                    Serial2.print(portArray[i].out); Serial2.print(b285settingsBytes);
                    Serial2.print("\r");
                }
            }
        }
        else if (msg.startsWith("testEvent")) {
            char testEventBytes[10] = {0}; msg.substring(9).toCharArray(testEventBytes, sizeof(testEventBytes));
            Serial.print(F("[DIRECT-LOG] Typ: testEvent | Bytes: '")); Serial.print(testEventBytes); Serial.println(F("'"));
            Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(testEventBytes); Serial.println(F("\\r'"));
            Serial2.print(testEventBytes); Serial2.print("\r");
        }
        else if (msg.startsWith("setDate")) {
            char setDateBytes[16] = {0}; msg.substring(7).toCharArray(setDateBytes, sizeof(setDateBytes));
            Serial.print(F("[DIRECT-LOG] Typ: setDate | Datum: '")); Serial.print(setDateBytes); Serial.println(F("'"));
            Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(setDateBytes); Serial.println(F("\\r'"));
            Serial2.print(setDateBytes); Serial2.print("\r");
        }
        else if (msg.startsWith("setTime")) {
            char setTimeBytes[16] = {0}; msg.substring(7).toCharArray(setTimeBytes, sizeof(setTimeBytes));
            Serial.print(F("[DIRECT-LOG] Typ: setTime | Uhrzeit: '")); Serial.print(setTimeBytes); Serial.println(F("'"));
            Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(setTimeBytes); Serial.println(F("\\r'"));
            Serial2.print(setTimeBytes); Serial2.print("\r");
        }
        else if (msg.startsWith("setEvent")) {
            char setEventBytes[40] = {0}; msg.substring(8).toCharArray(setEventBytes, sizeof(setEventBytes));
            Serial.print(F("[DIRECT-LOG] Typ: setEvent | Timer-String: '")); Serial.print(setEventBytes); Serial.println(F("'"));
            Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(setEventBytes); Serial.println(F("\\r'"));
            Serial2.print(setEventBytes); Serial2.print("\r");
        }
        else if (msg.startsWith("callEvent")) {
            char callEventBytes[10] = {0}; msg.substring(9).toCharArray(callEventBytes, sizeof(callEventBytes));
            Serial.print(F("[DIRECT-LOG] Typ: callEvent | Event-ID: '")); Serial.print(callEventBytes); Serial.println(F("'"));
            Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(callEventBytes); Serial.println(F("\\r'"));
            Serial2.print(callEventBytes); Serial2.print("\r");
        }
        else if (msg.startsWith("delEvent")) {
            char delEventBytes[10] = {0}; msg.substring(8).toCharArray(delEventBytes, sizeof(delEventBytes));
            Serial.print(F("[DIRECT-LOG] Typ: delEvent | Event-ID: '")); Serial.print(delEventBytes); Serial.println(F("'"));
            Serial.print(F("[DIRECT-LOG] -> Sende via Serial2: '")); Serial.print(delEventBytes); Serial.println(F("\\r'"));
            Serial2.print(delEventBytes); Serial2.print("\r");
        }
        else if (msg.startsWith("toggle")) {
            String subToggle = msg.substring(6);
            Serial.print(F("[DIRECT-LOG] Typ: toggle | Status: '")); Serial.print(subToggle); Serial.println(F("'"));
            if (subToggle.startsWith("true")) { 
                Serial.println(F("[DIRECT-LOG] -> Sende via Serial2: '0R0\\r'"));
                Serial2.print("0R0\r"); 
            } 
            else if (subToggle.startsWith("false")) { 
                Serial.println(F("[DIRECT-LOG] -> Sende via Serial2: '0R1\\r'"));
                Serial2.print("0R1\r"); 
            }
        }
        else {
            Serial.println(F("[DIRECT-LOG] ACHTUNG: Befehl wurde von keiner if-Bedingung erkannt!"));
        }
        Serial.println(F("=================================================="));
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    
    lastActivity = millis(); 
    String msg((char*)data, len);
    
    if (msg.startsWith("button")) {
      String subMsg = msg.substring(6);
      
      if (subMsg.startsWith("Push")) {
        memset(buttonName, 0, sizeof(buttonName));
        
        String nameStr = subMsg.substring(4);
        strncpy(buttonName, nameStr.c_str(), sizeof(buttonName) - 1);
        
        buttonHold = 1;
        isFirstButtonPress = true; 
        lastButtonRepeatTime = millis(); 
        wsWakeupTime = millis();

        // DETAILED LOG: Eingangsprüfung
        Serial.println(F("\n=================================================="));
        Serial.print(F("[WS-LOG] NETZWERK-PUSH EMPFANGEN fuer ID: '")); Serial.print(buttonName); Serial.println(F("'"));
        Serial.print(F("[WS-LOG] Status: buttonHold = ")); Serial.print(buttonHold);
        Serial.print(F(" | isFirstButtonPress = ")); Serial.println(isFirstButtonPress);
        Serial.println(F("=================================================="));

        int a = 0;
        bool matchFound = false;
        
        while (configArray[a].btnID[0] != '\0' && strcmp(configArray[a].btnID, "none") != 0 && a < maxCommandCapacity) {
          if (strcmp(buttonName, configArray[a].btnID) == 0) {
              matchFound = true;
              
              // DETAILED LOG: Treffer in der Konfigurationstabelle
              Serial.print(F("[WS-LOG] -> TREFFER in configArray an Index [")); Serial.print(a); Serial.println(F("]"));
              Serial.print(F("[WS-LOG]    Zugeordnetes Geraet: '")); Serial.print(configArray[a].device);
              Serial.print(F("' | ReVox-Kommando: 0x")); Serial.println(configArray[a].command, HEX);
              
              for (int i = 0; i < portTableSize; i++) {
                  String currentOut = portArray[i].out;
                  String currentDescr = portArray[i].descr;
                  bool deviceMatch = (strcmp(configArray[a].device, currentDescr.c_str()) == 0) || 
                                     (strstr(configArray[a].device, currentDescr.c_str()) != NULL);

                  if (deviceMatch && currentOut != "no") {
                      if (configArray[a].cmdFlag == 0 && configArray[a].isBibus == 0) {
                          if (configArray[a].command != 0x40) {
                              
                              // DETAILED LOG: Direkt vor dem physischen Senden
                              Serial.print(F("[WS-LOG]    Sende DIREKT-FRAME: Addr 0x")); Serial.print(configArray[a].address, HEX);
                              Serial.print(F(", Cmd 0x")); Serial.println(configArray[a].command, HEX);
                              
                              sendRevoxFrame(configArray[a].address, configArray[a].command, 1);
                              
                              Serial.println(F("[WS-LOG]    DIREKT-FRAME erfolgreich abgesetzt."));
                          } else {
                              Serial.println(F("[WS-LOG]    Uebersprungen: Befehl hat Blockier-Command 0x40"));
                          }
                      } else {
                          Serial.print(F("[WS-LOG]    Kein Direkt-Send: cmdFlag=")); Serial.print(configArray[a].cmdFlag);
                          Serial.print(F(" | isBibus=")); Serial.println(configArray[a].isBibus);
                      }
                  }
              }
              break; 
          }
          ++a;
        }
        
        if (!matchFound) {
            Serial.print(F("[WS-LOG] ACHTUNG: Die empfangene ID '")); Serial.print(buttonName);
            Serial.println(F("' existiert NICHT im configArray! Bitte HTML-ID pruefen."));
        }
      } 
      else if (subMsg.startsWith("Release")) {
        buttonHold = 0;
        isFirstButtonPress = true; 
        lastActivity = millis();
        
        Serial.println(F("\n=================================================="));
        Serial.print(F("[WS-LOG] NETZWERK-RELEASE EMPFANGEN fuer ID: '")); Serial.print(buttonName); Serial.println(F("'"));
        Serial.println(F("[WS-LOG] Status zurückgesetzt: buttonHold = 0 | isFirstButtonPress = true"));
        Serial.println(F("=================================================="));
      }
    }
    else {
      pendingWsCommand = msg;
      wsWakeupTime = millis();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, 
      void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      
      // Neue Seite ist da -> Sperre wieder aufheben!
      blockSerialSending = false;
      
      // Beim Neu-Verbinden einer Seite werfen wir alte Geisterdaten 
      // aus der vorherigen Session konsequent weg, anstatt sie an den neuen Client zu schicken!
      if (b203data.length() > 0) {
          Serial.print(F("[WS-CLEANUP] Altes Datenpaket beim Connect verworfen: "));
          Serial.println(b203data);
          b203data = ""; // Puffer löschen, OHNE an den Client zu senden!
      }
      break;
      
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      
      // Sperre für serielle Daten SOFORT aktivieren!
      blockSerialSending = true;
      serialBlockStartTime = millis();
      
      b203Buffer = ""; 
      b203data = "";   
      Serial.println(F("[B203-CLEANUP] Sämtliche Datenpuffer gelöscht. Senden blockiert."));
      break;
      
    case WS_EVT_DATA:
      Serial.println(F("\n[WLAN-WAKEUP] WebSocket-Daten empfangen!"));
      handleWebSocketMessage(arg, data, len);
      break;
      
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

// Was passiert, wenn eine Seite nicht gefunden werden kann
// Es wird die Datei `404.html` ausgeliefert
void notFound(AsyncWebServerRequest *request) {
    AsyncWebServerResponse* response = 
      request   ->  beginResponse(LittleFS, "/404.html", "text/html");
      response  ->  setCode(404);
      request   ->  send(response);
}

void setupServerRoutes(){

    // 1. HAUPTROUTEN (HTML-Seiten als GZIP)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
      String pathToSend = String(htmlPath);
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, pathToSend, "text/html");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });
    
    server.on("/b203", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/b203.html.gz", "text/html");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });
    
    server.on("/cd", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/cd.html.gz", "text/html");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });
    
    server.on("/phono", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/phono.html.gz", "text/html");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });
    
    server.on("/tape1", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/tape1.html.gz", "text/html");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });
    
    server.on("/tape2", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/tape2.html.gz", "text/html");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });
    
    server.on("/tuner", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/tuner.html.gz", "text/html");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });
    
    server.on("/receiver", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/receiver.html.gz", "text/html");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
      // Lädt die .gz Datei, gibt dem Browser aber das korrekte Icon-Format an
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/favicon.ico.gz", "image/x-icon");
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });

    // NEU: Eigene, saubere Route für das WiFi-Setup (Kollision auf / behoben!)
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/config/wifi_config.html", String(), false);
    });

    // 2. CONFIG ROUTEN (GET)
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
      if (LittleFS.exists(ConfigPath)) {
        request->send(LittleFS, ConfigPath, "application/json");
      } else {
        request->send(200, F("application/json"), F("[]"));
      }
    });

    // CONFIG ROUTEN (POST)
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      static File uploadFile;
      if (index == 0) {
        uploadFile = LittleFS.open(ConfigPath, "w");
      }
      if (uploadFile) {
        uploadFile.write(data, len);
      }
      if (index + len == total) {
        if (uploadFile) {
          uploadFile.close();
          Serial.println(F("Config-Datei im LittleFS erfolgreich aktualisiert."));
          loadCommandConfig(); 
        }
        request->send(200, F("text/plain"), F("OK"));
      }
    });

    // 3. POST-ROUTE FÜR /api/save-data (Mit Schutzwall und RAM-Sicherung)
    server.on("/api/save-data", HTTP_POST, [](AsyncWebServerRequest *request) {
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      static String jsonBuffer = "";
      
      if (index == 0) {
        jsonBuffer = "";
        jsonBuffer.reserve(total); // Speicher vorab reservieren (Heap-Schutz)
      }
      
      for (size_t i = 0; i < len; i++) {
        jsonBuffer += (char)data[i];
      }
      
      if (index + len == total) {
        // SCHUTZWALL gegen Geister-Requests integriert
        if (jsonBuffer.length() < 15 || jsonBuffer == "[]" || jsonBuffer == "[\n]") {
          Serial.println(F("WARNUNG: Leerer/ungueltiger POST blockiert! Datei wird NICHT ueberschrieben."));
          request->send(200, F("application/json"), F("{\"status\":\"ignored\",\"message\":\"Leere Daten ignoriert\"}"));
          jsonBuffer = "";
          return;
        }

        File portFile = LittleFS.open(PortConfigPath, "w");
        if (portFile) {
          portFile.print(jsonBuffer);
          portFile.close();
          Serial.println(F("Port-Konfiguration erfolgreich im LittleFS gespeichert."));
          loadPortConfig(); 
          if (portExists("B285")) {
            htmlPath = "/receiver.html.gz";
          } else {
            htmlPath = "/amplifier.html.gz";
          }
        }
        request->send(200, F("application/json"), F("{\"status\":\"success\",\"message\":\"JSON gespeichert\"}"));
        jsonBuffer = "";
      }
    });

    // 4. STATISCHE DATEIEN

    server.serveStatic("/css/", LittleFS, "/css/");
    server.serveStatic("/js/", LittleFS, "/js/");
    server.serveStatic("/style.css", LittleFS, "/style.css");
    server.serveStatic("/script.js", LittleFS, "/script.js");

    // Fallback für dynamische JSONs und HTML-Seiten

    server.onNotFound([](AsyncWebServerRequest *request) {
      String url = request->url();

      if (url == "/" || url.length() == 0) {
        String pathToSend = String(htmlPath);
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, pathToSend, "text/html");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
        return;
      }
      
      if (url.endsWith(".json")) {
        if (LittleFS.exists(url)) {
          request->send(LittleFS, url, "application/json");
          return;
        }
      }

      if (url.endsWith(".html") || url.endsWith(".hml")) {
        String baseName = url.substring(0, url.lastIndexOf('.'));
        String gzipPath = baseName + ".html.gz";
        if (LittleFS.exists(gzipPath)) {
          AsyncWebServerResponse *response = request->beginResponse(LittleFS, gzipPath, "text/html");
          response->addHeader("Content-Encoding", "gzip");
          request->send(response);
          return;
        }
      }
      
      notFound(request); 
    });
}

void initLittleFS() { // Initialize LittleFS
  if (!LittleFS.begin(true)) { // true = format on fail
    Serial.println(F("LittleFS Mount Failed"));
    return;
  }
  Serial.println(F("LittleFS Mounted Successfully"));
}

void loadWIFIConfig() {
  bool configLoaded = false;
  
  // 1. Datei öffnen und auswerten
  File file = LittleFS.open(WificonfigPath, "r");
  if (!file) {
    Serial.println(F("Konnte die Config-Datei nicht finden. Weiche auf Access Point aus..."));
  } else {
    size_t size = file.size();
    if (size > 0) {
      std::unique_ptr<char[]> buf(new char[size]);
      file.readBytes(buf.get(), size);

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, buf.get());
      
      if (!error) {
        wifi_ssid = doc["ssid"].as<String>();
        wifi_pass = doc["password"].as<String>();
        configLoaded = true;
      } else {
        Serial.print(F("Fehler beim Parsen der WLAN-JSON: "));
        Serial.println(error.f_str());
      }
    }
    file.close();
  }

  // 2. Hostnamen vergeben
  WiFi.setHostname(hostName); 

  // 3. Verbindungsversuch nur starten, wenn die Config erfolgreich geladen wurde
  if (configLoaded && wifi_ssid.length() > 0) {
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    Serial.print(F("Verbinde mit WLAN"));

    // Dem ESP Zeit geben, sich zu verbinden (10 Sekunden Timeout)
    int timeout_counter = 0;
    while (WiFi.status() != WL_CONNECTED && timeout_counter < 20) { 
      delay(500);
      Serial.print(F("."));
      timeout_counter++;
    }
    Serial.println();
  }

  // 4. Status prüfen: Wenn nicht verbunden (oder keine Config da), AP starten!
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WLAN nicht verbunden oder keine Config. Starte Access Point..."));
    WiFi.disconnect(); 
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid_ap, password_ap, 1, 0);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print(F("AP IP Adresse: "));
    Serial.println(IP);
    
    // WICHTIG: Im AP-Modus erzwingen wir volle Leistung für eine stabile Verbindung!
    esp_wifi_set_ps(WIFI_PS_NONE); 
    
  } else {
    Serial.print(F("Erfolgreich verbunden! IP: "));
    Serial.println(WiFi.localIP());

    // JETZT WIEDER STROMSPAREND: Light Sleep für das WLAN reaktivieren
    WiFi.setSleep(true); 
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    Serial.println(F("[POWER] WLAN-Stromsparmodus (Light Sleep) REAKTIVIERT."));
  }
}

// Funktion zum Speichern der Konfiguration in LittleFS
void saveWIFIConfig(String ssid, String pass) {
  JsonDocument doc;
  doc["ssid"] = ssid;
  doc["password"] = pass;

  //File file = LittleFS.open("/config/wifi_config.json", "w");
  File file = LittleFS.open(WificonfigPath, "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println(F("Konfiguration gespeichert!"));
  }
}

struct B203SetData {
  char language [4];
  char easy [4];
  char timer [4];
  char poweron [4];
};

B203SetData b203settings = { "0", "0", "0", "0" };

void setup() {

  Serial.begin(115200);   // Serial port for debugging purposes
  delay(50);

  setupSLoutPin();

  pinMode(PIN_RTS, INPUT);
  pinMode(PIN_CTS, OUTPUT);

  // KORRIGIERT: Pegelwandler-Enable kompakter und sauberer schalten
  pinMode(OE_FXMA108, OUTPUT);
  digitalWrite(OE_FXMA108, Set_OE_FXMA108 ? HIGH : LOW);
  
  Serial2.begin(REVOX_BAUD, SERIAL_8N1, RXD2, TXD2);

  if (Serial2) {
    Serial.println(F("Revox Serial startet"));
  }

  Serial2.setTimeout(5000);
  
  initLittleFS();
  loadWIFIConfig();

  // IR-receiver starten
  pinMode(IR_RECEIVE_PIN, INPUT_PULLUP); 
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  Serial.println(F("IR Empfaenger aktiviert"));

  // GPIO-WAKEUP ÜBER DEFINE STEUERN
  #ifdef ENABLE_LOW_POWER
    gpio_wakeup_enable((gpio_num_t)IR_RECEIVE_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    Serial.println(F("[POWER] GPIO Wakeup fuer IR-Pin eingerichtet."));
  #endif

  Serial.print(F("ESP IP-Adresse: "));
  Serial.println(WiFi.localIP());

  loadPortConfig();
  loadCommandConfig();  // Lädt Commands in configArray

  if (portExists("B285")) {
    htmlPath = "/receiver.html.gz";
  } else {
    htmlPath = "/amplifier.html.gz";
  }

  setupServerRoutes();
  initWebSocket();

  // Aktuelle Config-Werte als JSON an die Webseite senden
  server.on("/get-config", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    doc["ssid"] = wifi_ssid;
    doc["password"] = wifi_pass;
  
    String jsonString;
    serializeJson(doc, jsonString);
    request->send(200, F("application/json"), jsonString); // F-Makro ergänzt
  });

  Serial.print(F("HTTP server started"));
  server.begin();
  Serial.println();
  Serial.print(F("Revox-Remote ist bereit"));
  Serial.println();

  btStop(); // Schaltet Bluetooth ab, um Strom zu sparen – sehr gut!
}

void loop() {

  // ==========================================   
  // 0. SYSTEM-STATUS & NETZWERK
  // ==========================================   

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_WIFI) {
      Serial.println(F("[WLAN-WAKEUP]"));
  }

  ws.cleanupClients();

  if (millis() - lastPingTime >= pingInterval) {
      lastPingTime = millis();
      ws.pingAll(); 
  }
  
  // ==========================================   
  // 1. NON-BLOCKING LESEN & SOFORTIGE XON/XOFF PRÜFUNG (MIT SPERRE)
  // ==========================================   

  while (Serial2.available() > 0) {
      char inChar = (char)Serial2.read();

      if (inChar == (char)0x13) {          
          b203ReadyToSend = false;
          Serial.println(F("[B203] XOFF empfangen - Senden blockiert"));
          continue; 
      } 
      else if (inChar == (char)0x11) {     
          b203ReadyToSend = true;
          Serial.println(F("[B203] XON empfangen - Senden freigegeben"));
          continue; 
      }

      // Wenn das Zeilenende erreicht ist, verarbeiten wir den Puffer
      if (inChar == '\r' || inChar == '\n') {
          if (b203Buffer.length() > 0) {
              b203data = b203Buffer;
              b203data.trim();
              b203Buffer = ""; 
            
              // SICHERHEITSHÜRDE: Wenn blockiert, Daten einfach vernichten und ignorieren!
              if (blockSerialSending) {
                  Serial.print(F("[B203 -> BLOCK] Daten während Seitenwechsel verworfen: "));
                  Serial.println(b203data);
                  b203data = "";
                
                  // Sicherheitsnetz: Nach 3 Sekunden die Sperre automatisch lösen, falls der Connect fehlschlägt
                  if (millis() - serialBlockStartTime > 3000) {
                      blockSerialSending = false;
                  }
                  continue; 
              }
            
              // Reguläres Senden, wenn Clients da sind und keine Sperre aktiv ist
              if (b203data.length() > 0 && ws.count() > 0) {
                  Serial.print(F("[B203 -> WEB] Sende: ")); 
                  Serial.println(b203data);
                  ws.textAll(b203data); 
              }
          }
      } 
      else {
          // KORREKTUR: Normales Zeichen empfangen -> In den Puffer schreiben
          b203Buffer += inChar;
      }
  }

  // ==========================================
  // 2. + 3. AUSGELAGERTE FUNKTIONS-AUFRUFE
  // ==========================================

  processDirectCommands();  // Verarbeitet Web-Direktbefehle (Ehemals Teil 2)
  processButtonPath();     // Verarbeitet Web- & manuelle Buttons (Ehemals Teil 3)

  // ==========================================
  // 4. INFRAROT-PFAD (ERWEITERTES LOGGING)
  // ==========================================

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis; 

      if (IrReceiver.decode()) {
          uint32_t combined = ((uint32_t)IrReceiver.decodedIRData.address << 16) | IrReceiver.decodedIRData.command;

          // DETAILED IR-LOG: Basis-Daten beim Empfang
          Serial.println(F("\n=================================================="));
          Serial.println(F("[IR-LOG] INFRAROT-SIGNAL ERKANNT!"));
          Serial.print(F("[IR-LOG] Protokoll: ")); Serial.println(IrReceiver.getProtocolString());
          Serial.print(F("[IR-LOG] Adresse: 0x")); Serial.print(IrReceiver.decodedIRData.address, HEX);
          Serial.print(F(" | Kommando: 0x")); Serial.println(IrReceiver.decodedIRData.command, HEX);
          Serial.print(F("[IR-LOG] Kombinierter Code (combined): 0x")); Serial.println(combined, HEX);
          
          if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
              Serial.println(F("[IR-LOG] Signal ungenau oder unbekanntes Protokoll (Noise)"));
              IrReceiver.resume(); 
              Serial.println(F("=================================================="));
          } else {
              Serial.print(F("[IR-LOG] Kurzinfo: "));
              IrReceiver.printIRResultShort(&Serial);   
              Serial.println();
          }

          if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
              Serial.println(F("[IR-LOG] Typ: WIEDERHOLUNG (Taste gedrueckt gehalten)"));
              
              if ((configArray[irid].repeat == 1) && (irid > 0) && (configArray[irid].command != 0x40)) {
                  if (configArray[irid].addressRep != 0) {
                      Serial.print(F("[IR-LOG] -> Sende REPEAT-Frame mit addressRep: 0x")); Serial.println(configArray[irid].addressRep, HEX);
                      sendRevoxFrame(configArray[irid].addressRep, configArray[irid].command, 1);
                  } else {
                      Serial.print(F("[IR-LOG] -> Sende REPEAT-Frame mit Standard-Addr: 0x")); Serial.println(configArray[irid].address, HEX);
                      sendRevoxFrame(configArray[irid].address, configArray[irid].command, 1);
                  }
              } else {
                  Serial.println(F("[IR-LOG] -> Keine Sende-Aktion (repeat=0 oder blockiert)"));
              }
              Serial.println(F("=================================================="));
          } 
          else {
              Serial.println(F("[IR-LOG] Typ: ERST-DRUCK (Neues Signal)"));
              irid = 0;
              lastActivity = millis();
              bool irMatchFound = false;
              
              // KORRIGIERT: Beibehaltung von [0] fuer sichere String-Pruefung
              while (configArray[irid].btnID[0] != '\0' && strcmp(configArray[irid].btnID, "none") != 0 && irid < maxCommandCapacity) {
                  
                  if ((combined == configArray[irid].irRecvCode) && (combined != 0)) {
                      irMatchFound = true;
                      
                      Serial.print(F("[IR-LOG] -> TREFFER in Konfiguration bei Index [")); Serial.print(irid); Serial.println(F("]"));
                      Serial.print(F("[IR-LOG]    ID: '")); Serial.print(configArray[irid].btnID);
                      Serial.print(F("' | Geraet: '")); Serial.print(configArray[irid].device); Serial.println(F("'"));
                      
                      if ((configArray[irid].address < 0x11) && (configArray[irid].cmdFlag == 0)) {
                          if (configArray[irid].command != 0x40) {
                              Serial.print(F("[IR-LOG]    Sende ReVox-Frame: Addr 0x")); Serial.print(configArray[irid].address, HEX);
                              Serial.print(F(", Cmd 0x")); Serial.println(configArray[irid].command, HEX);
                              
                              sendRevoxFrame(configArray[irid].address, configArray[irid].command, 1);
                          } else {
                              Serial.println(F("[IR-LOG]    Uebersprungen: Befehl hat Blockier-Command 0x40"));
                          }
                      } else {
                          Serial.println(F("[IR-LOG]    Keine native ReVox-Sendung (cmdFlag > 0 oder Addr >= 0x11)"));
                      }
                      break; 
                  }
                  ++irid;
              }
              
              if (!irMatchFound) {
                  Serial.println(F("[IR-LOG] Info: Dieser IR-Code ist in der configArray-Tabelle nicht hinterlegt."));
              }
              Serial.println(F("=================================================="));
          }
          IrReceiver.resume(); 
      }
  }
 
  // ==========================================
  // 5. DYNAMISCHES SCHLAF-FENSTER
  // ==========================================

  if (buttonHold > 0 || Serial2.available() > 0 || b203Buffer.length() > 0) {
      lastActivity = millis(); 
  }

  if (millis() - lastActivity >= 2000) {
      delay(10); 
  } else {
      delay(1);  
  }
}