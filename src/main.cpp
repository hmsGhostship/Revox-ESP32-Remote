/*********
  H. Haefner ESP32 IR Remote for Revox B203 and B2xx Series
  Complete project details at 
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

// Import required libraries
#include <Arduino.h>
#include <WiFi.h>
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
#include <IRRemote.hpp>

#include "SerialLink.h"
#include "config.h"

JsonDocument configDoc;
bool State = 0;
bool buttonHold = 0;
bool getFlag = 0;
unsigned long previousMillis = 0;
const long interval = 130;
char buttonName[18];
int irid =0;
String b203data;
const int PIN_RECV = 25;
const int PIN_CTS = 26;
const int PIN_RTS = 27;
const int OE_FXMA108 = 0;
bool Set_OE_FXMA108 = true;
const char* WificonfigPath = "/wifi_config.json";
const char* PortConfigPath = "/portconfig.json";
const char* ConfigPath = "/config.json";
const char* ssid_ap = "REVOXSETUP";
const char* password_ap = "esp32revox";
String wifi_ssid = "";
String wifi_pass = "";
const char* hostName = "revoxb203"; // Ihr Wunsch-Hostname
const char* htmlPath;

const int maxPortEntries = 20;               // Maximale RAM-Kapazität des Arrays
portcnf portArray[maxPortEntries];           // Das feste Speicher-Array im RAM
int portTableSize = 0;

const int maxCommandCapacity = 200; // Anpassen an Ihre maximale Zeilenanzahl
command configArray[maxCommandCapacity];
int configTableSize = 0;

unsigned long lastPingTime = 0;
const unsigned long pingInterval = 10000; // Alle 10 Sekunden pingen

bool b203ReadyToSend = true; // Steuert den XON/XOFF Fluss

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

int portExists(const char* searchName) {
    if (searchName == NULL) return 0;
    
    for (size_t i = 0; i < portTableSize; i++) {
        if (portArray[i].name != NULL && strcmp(portArray[i].name, searchName) == 0) {
            return 1; // Gefunden!
        }
    }
    return 0; // Nicht gefunden
}


void loadPortConfig() {
  // Datei öffnen
  File file = LittleFS.open(PortConfigPath, "r");
  if (!file) {
    Serial.println("Port Configuration konnte nicht geöffnet werden");
    return;
  }
  
  JsonDocument doc;

  // Datei parsen
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print("Fehler beim Parsen: ");
    Serial.println(error.f_str());
    file.close();
    return;
  }

  file.close(); // Datei schließen, da sie komplett im RAM (doc) liegt
  
  // Das geparste JSON als Array auslesen
  JsonArray array = doc.as<JsonArray>();
  
  // WEG 1: Die physikalisch maximale Kapazität des Arrays in Bytes ermitteln
  int maxArrayCapacity = sizeof(portArray) / sizeof(portArray[0]);

  // Echte JSON-Größe auslesen und portTableSize dynamisch anpassen
  if (array.size() > maxArrayCapacity) {
    portTableSize = maxArrayCapacity; // Schutz vor Speicherüberlauf (Buffer Overflow)
    Serial.print("Warnung: JSON zu groß! Begrenzt auf Maximum: ");
    Serial.println(maxArrayCapacity);
  } else {
    portTableSize = array.size();     // Setzt die Größe exakt auf die echten Einträge (z.B. 6)
  }
  
  // Array mit den echten Daten befüllen
  int index = 0;
  for (JsonObject obj : array) {
    if (index >= portTableSize) break; // Zusätzlicher Sicherheitsanker

    // Daten kopieren und Null-Terminierung garantieren
    strncpy(portArray[index].name, obj["name"] | "", sizeof(portArray[index].name) - 1);
    portArray[index].name[sizeof(portArray[index].name) - 1] = '\0';

    strncpy(portArray[index].descr, obj["descr"] | "", sizeof(portArray[index].descr) - 1);
    portArray[index].descr[sizeof(portArray[index].descr) - 1] = '\0';
    
    strncpy(portArray[index].out, obj["out"] | "", sizeof(portArray[index].out) - 1);
    portArray[index].out[sizeof(portArray[index].out) - 1] = '\0';
    
    portArray[index].feedback = obj["feedback"] | false;

    index++;
  }

  // Kontrollausgabe über die exakte, ermittelte Größe
  Serial.print("Erfolgreich geladen. Aktuelle portTableSize: ");
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
    Serial.print("Fehler beim Parsen von /config.json: ");
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
    Serial.print("Warnung: config.json zu groß! Begrenzt auf: ");
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
    Serial.print(F(" | command: 0x"));
    Serial.print(configArray[index].command, HEX);
    Serial.print(F(" | isBiBus: "));
    Serial.print(configArray[index].isBibus ? F("true") : F("false"));
    Serial.print(F(" | BiBusCmd: "));
    Serial.print(configArray[index].bibusCmd); // Als Text ausgeben, da String!
    Serial.print(F(" | cmdFlag: "));
    Serial.println(configArray[index].cmdFlag); // Als Text ausgeben, da String!

    index++;
  }

  // Kontrollausgabe im Seriellen Monitor
  Serial.print("Erfolgreich geladen. Aktuelle configTableSize: ");
  Serial.println(configTableSize);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    
    // 1. Erstelle einen sicheren, nullterminierten Arbeits-String
    String msg = String((char*)data).substring(0, len);
    
    // --- AB HIER ARBEITEN WIR NUR NOCH MIT DER "msg" ---

    if (msg.startsWith("button")) {
      String subMsg = msg.substring(6); // Schneidet "button" ab
      
      if (subMsg.startsWith("Push")) {
        // Kopiert den Rest sicher in buttonName und garantiert die Nullterminierung
        memset(buttonName, 0, sizeof(buttonName));
        subMsg.substring(4).toCharArray(buttonName, sizeof(buttonName));
        buttonHold = 1;
      } else if (subMsg.startsWith("Release")) {
        buttonHold = 0;
      }
    }

    else if (msg.startsWith("speakers")) {
      char b285Speaker[4] = {0}; // +1 für sichere Nullterminierung initialisiert
      msg.substring(8).toCharArray(b285Speaker, sizeof(b285Speaker));
      
      for (int i = 0; i < portTableSize; i++) {
        if ((strcmp("receiver", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
          Serial2.print(portArray[i].out);
          Serial2.print(b285Speaker);
          Serial2.print("\r");
          Serial.print(portArray[i].out);
          Serial.println(b285Speaker);
        }
      }
    }

    else if (msg.startsWith("volSlider")) {
      char b285Volume[5] = {0}; // Initialisiert mit Nullen
      msg.substring(9).toCharArray(b285Volume, sizeof(b285Volume));
      
      for (int i = 0; i < portTableSize; i++) {
        if ((strcmp("receiver", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
          Serial2.print(portArray[i].out);
          Serial2.print(b285Volume);
          Serial2.print("\r");
          Serial.print(portArray[i].out);
          Serial.println(b285Volume);
        }
      }
    }

    else if (msg.startsWith("setup")) {
      char setupBytes[8] = {0};
      msg.substring(5).toCharArray(setupBytes, sizeof(setupBytes));
      Serial2.print(setupBytes);
      Serial2.print("\r");
      Serial.println(setupBytes);
    }

    else if (msg.startsWith("getsettings")) {
      char settingsBytes[4] = {0};
      msg.substring(11).toCharArray(settingsBytes, sizeof(settingsBytes));
      Serial2.print(settingsBytes);
      Serial2.print("\r");
      Serial.println(settingsBytes);
      if (strcmp(settingsBytes, "0X") == 0) {
        getFlag = 1;
      }
    }

    else if (msg.startsWith("tape1")) {
      char b215settingsBytes[3] = {0};
      msg.substring(5).toCharArray(b215settingsBytes, sizeof(b215settingsBytes));
      
      for (int i = 0; i < portTableSize; i++) {
        if ((strcmp("tape1", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
          Serial2.print(portArray[i].out);
          Serial2.print(b215settingsBytes);
          Serial2.print("\r");
          Serial.print(portArray[i].out);
          Serial.print(b215settingsBytes);
          if (strcmp(b215settingsBytes, "X") == 0) {
            getFlag = 1;
          }
        }
      }
    }

    else if (msg.startsWith("cdplayer")) {
      char b226settingsBytes[3] = {0};
      msg.substring(8).toCharArray(b226settingsBytes, sizeof(b226settingsBytes));
      
      for (int i = 0; i < portTableSize; i++) {
        if ((strcmp("cdplayer", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
          Serial2.print(portArray[i].out);
          Serial2.print(b226settingsBytes);
          Serial2.print("\r");
          Serial.print(portArray[i].out);
          Serial.print(b226settingsBytes);
          if (strcmp(b226settingsBytes, "X") == 0) {
            getFlag = 1;
          }
        }
      }
    }

    else if (msg.startsWith("phono")) {
      char b291settingsBytes[3] = {0};
      msg.substring(5).toCharArray(b291settingsBytes, sizeof(b291settingsBytes));
      
      for (int i = 0; i < portTableSize; i++) {
        if ((strcmp("phono", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
          Serial2.print(portArray[i].out);
          Serial2.print(b291settingsBytes);
          Serial2.print("\r");
          Serial.print(portArray[i].out);
          Serial.print(b291settingsBytes);
          if (strcmp(b291settingsBytes, "X") == 0) {
            getFlag = 1;
          }
        }
      }
    }
    
    else if (msg.startsWith("receiver")) {
      char b285settingsBytes[3] = {0};
      msg.substring(8).toCharArray(b285settingsBytes, sizeof(b285settingsBytes));
      
      for (int i = 0; i < portTableSize; i++) {
        if ((strcmp("receiver", portArray[i].descr) == 0) && (portArray[i].out != "no")) {
          Serial2.print(portArray[i].out);
          Serial2.print(b285settingsBytes);
          Serial2.print("\r");
          Serial.print(portArray[i].out);
          Serial.print(b285settingsBytes);
          if (strcmp(b285settingsBytes, "X") == 0) {
            getFlag = 1;
          }
        }
      }
    }

    else if (msg.startsWith("testEvent")) {
      char testEventBytes[6] = {0};
      msg.substring(9).toCharArray(testEventBytes, sizeof(testEventBytes));
      Serial2.print(testEventBytes);
      Serial2.print("\r");
      Serial.println(testEventBytes);
    }

    else if (msg.startsWith("setDate")) {
      char setDateBytes[10] = {0};
      msg.substring(7).toCharArray(setDateBytes, sizeof(setDateBytes));
      Serial2.print(setDateBytes);
      Serial2.print("\r");
      Serial.println(setDateBytes);
    }

    else if (msg.startsWith("setTime")) {
      char setTimeBytes[10] = {0};
      msg.substring(7).toCharArray(setTimeBytes, sizeof(setTimeBytes));
      Serial2.print(setTimeBytes);
      Serial2.print("\r");
      Serial.println(setTimeBytes);
    }

    else if (msg.startsWith("setEvent")) {
      char setEventBytes[33] = {0};
      msg.substring(8).toCharArray(setEventBytes, sizeof(setEventBytes));
      Serial2.print(setEventBytes);
      Serial2.print("\r");
      Serial.println(setEventBytes);
    }

    else if (msg.startsWith("callEvent")) {
      char callEventBytes[6] = {0};
      msg.substring(9).toCharArray(callEventBytes, sizeof(callEventBytes));
      Serial2.print(callEventBytes);
      Serial2.print("\r");
      Serial.println(callEventBytes);
    }

    else if (msg.startsWith("delEvent")) {
      char delEventBytes[6] = {0};
      msg.substring(8).toCharArray(delEventBytes, sizeof(delEventBytes));
      Serial2.print(delEventBytes);
      Serial2.print("\r");
      Serial.println(delEventBytes);
    }

    else if (msg.startsWith("toggle")) {
      String subToggle = msg.substring(6);
      if (subToggle.startsWith("true")) {
        Serial2.print("0R0");
        Serial2.print("\r");
        Serial.println("0R0");
      } else if (subToggle.startsWith("false")) {
        Serial2.print("0R1");
        Serial2.print("\r");
        Serial.println("0R1");
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, 
      void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      //wsopen = 1;
      if (b203data.length() > 0 ) {
      ws.textAll(b203data);
      b203data = '\0';
      }
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
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
    
    // Auf dem Pfad `/` wird die Datei `htmlPath` aus dem `data` Ordner ausgeliefert
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, htmlPath, "text/html");
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

    // 1. GET-Route: Sendet die gesamte Datei an den Browser
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
      if (LittleFS.exists(ConfigPath)) {
        request->send(LittleFS, ConfigPath, "application/json");
      } else {
        request->send(200, "application/json", "[]");
      }
    });

    // 2. POST-Route: Empfängt das modifizierte JSON und speichert es ab
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {
      // Antwort erfolgt im Body-Handler unten
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  
      static File uploadFile;

      if (index == 0) {
        uploadFile = LittleFS.open(ConfigPath, "w");
        if (!uploadFile) {
          Serial.println("Fehler: ConfigPath konnte nicht zum Schreiben geöffnet werden!");
        }
      }

      if (uploadFile) {
        uploadFile.write(data, len);
      }

      if (index + len == total) {
        if (uploadFile) {
          uploadFile.close();
          Serial.println("Config-Datei im LittleFS erfolgreich aktualisiert.");
          loadCommandConfig(); 
        }
        request->send(200, "text/plain", "OK");
      }
    });

    // 1. Registrieren der Route für POST
    AsyncCallbackWebHandler* handler = &server.on("/api/save-data", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Antwort senden
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"JSON gespeichert\"}");
      
        // Erst NACHDEM die Datei im Body-Handler komplett geschrieben wurde, laden wir sie neu
        loadPortConfig();

        // Dynamischen Pfad für den nächsten Start/Aufruf der Startseite anpassen
        if (portExists("B285")) {
          htmlPath = "/receiver.html.gz";
        } else {
          htmlPath = "/amplifier.html.gz";
        }
    });

    // 2. Body-Handler für /api/save-data (Schreibt die Port-Konfiguration)
    handler->onBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      File file = LittleFS.open(PortConfigPath, (index == 0) ? "w" : "a");
      if (file) {
        file.write(data, len);
        file.close();
      }
    });

    // Wenn die angeforderte Seite nicht vorhanden ist
    server.serveStatic("/", LittleFS, "/");
    server.onNotFound(notFound);
}


void initLittleFS() { // Initialize LittleFS
  if (!LittleFS.begin(true)) { // true = format on fail
    Serial.println("LittleFS Mount Failed");
    return;
  }
  Serial.println("LittleFS Mounted Successfully");
}

void loadWIFIConfig() {
  File file = LittleFS.open(WificonfigPath, "r");
  if (!file) {
    Serial.println("Konnte die Config-Datei nicht finden. Erstelle Standardwerte...");
    return;
  }
  size_t size = file.size();
  std::unique_ptr<char[]> buf(new char[size]);
  file.readBytes(buf.get(), size);

  JsonDocument doc;
  deserializeJson(doc, buf.get());

  wifi_ssid = doc["ssid"].as<String>();
  wifi_pass = doc["password"].as<String>();
  
  file.close();

  // Hostnamen vergeben
  WiFi.setHostname(hostName); 
  
  // Verbinde mit dem WLAN
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  Serial.print("Verbinde mit WLAN");

  // NEU: Dem ESP Zeit geben, sich zu verbinden (10 Sekunden Timeout)
  int timeout_counter = 0;
  while (WiFi.status() != WL_CONNECTED && timeout_counter < 20) { 
    delay(500);
    Serial.print(".");
    timeout_counter++;
  }
  Serial.println();

  // Erst JETZT prüfen, ob es geklappt hat
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WLAN nicht verbunden. Starte Access Point...");
    WiFi.disconnect(); 
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid_ap, password_ap, 1, 0);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP Adresse: ");
    Serial.println(IP);
  } else {
    Serial.print("Erfolgreich verbunden! IP: ");
    Serial.println(WiFi.localIP());
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
    Serial.println("Konfiguration gespeichert!");
  }
}

struct B203SetData {
  char language [2];
  char easy [2];
  char timer [2];
  char poweron [2];
};

B203SetData b203settings = { "0", "0", "0", "0" };

void setup() {

  Serial.begin(115200);   // Serial port for debugging purposes
  delay(50);

  setupSLoutPin();

  pinMode(PIN_RTS, INPUT);
  pinMode(PIN_CTS, OUTPUT);

  pinMode(OE_FXMA108, OUTPUT);
  if (Set_OE_FXMA108 == true){
    digitalWrite(OE_FXMA108, HIGH);
  } else if (Set_OE_FXMA108 == false) {
    digitalWrite(OE_FXMA108, LOW);
  }
  
 Serial2.begin(REVOX_BAUD, SERIAL_8N1, RXD2, TXD2);

 if (Serial2) {
  Serial.println("Revox Serial startet");
 }

  Serial2.setTimeout(5000);
  
initLittleFS();

loadWIFIConfig();

  int timeout_counter = 0;
  while (WiFi.status() != WL_CONNECTED && timeout_counter < 20) { 
    delay(500);
    Serial.print(".");
    timeout_counter++;
  }
  Serial.println();
  // --- ENDE DES CODES ---

  // IR-receiver starten
  IrReceiver.begin(PIN_RECV, ENABLE_LED_FEEDBACK);
  Serial.println("IR Empfaenger aktiviert");


  WiFi.setSleep(false);
  
  //ESP Local IP Adresse
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

  // 1. Die index.html aus dem LittleFS-Verzeichnis ausliefern
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/config/wifi_config.html", String(), false);
  });

  // 2. Aktuelle Config-Werte als JSON an die Webseite senden
  server.on("/get-config", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    doc["ssid"] = wifi_ssid;
    doc["password"] = wifi_pass;
  
    String jsonString;
    serializeJson(doc, jsonString);
    request->send(200, "application/json", jsonString);
  });

  // 3. Empfängt die POST-Daten, speichert sie und startet den ESP neu
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String newSsid = request->getParam("ssid", true)->value();
      String newPass = request->getParam("password", true)->value();
   
      saveWIFIConfig(newSsid, newPass);
     
      request->send(200, "text/plain", "Konfiguration gespeichert. Gerät startet neu...");
      delay(1000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", "Fehlende Parameter");
    }
  });


  Serial.print("HTTP server started");
  server.begin();
  Serial.println();
  Serial.print("Revox-Remote ist bereit");
  Serial.println();

  esp_sleep_enable_wifi_wakeup();
  gpio_wakeup_enable((gpio_num_t)PIN_RECV, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  btStop();

}

void loop() {
  
  //goToLightSleep();

  ws.cleanupClients();

  if (millis() - lastPingTime >= pingInterval) {
      lastPingTime = millis();
      ws.pingAll(); 
  }
  
 // ==========================================   
  // REVOLUTIONIERT: URSPRÜNGLICHES LESEN + XON/XOFF PRÜFUNG
  // ==========================================   
  if (Serial2.available() > 0) {
      // Nutzt das bewährte, blockierungsfreie Warten von Arduino
      b203data = Serial2.readStringUntil('\n'); 
      
      // Falls Carriage Return am Ende steht, sauber entfernen
      if (b203data.endsWith("\r")) {
          b203data.remove(b203data.length() - 1);
      }
      
      // XOFF im String enthalten?
      if (b203data.indexOf((char)0x13) != -1) {
          b203ReadyToSend = false;
          Serial.println(F("[B203] XOFF empfangen - Senden blockiert"));
          b203data.replace(String((char)0x13), ""); // Steuerzeichen aus Text löschen
      }
      // XON im String enthalten?
      else if (b203data.indexOf((char)0x11) != -1) {
          b203ReadyToSend = true;
          Serial.println(F("[B203] XON empfangen - Senden freigegeben"));
          b203data.replace(String((char)0x11), ""); // Steuerzeichen aus Text löschen
      }

      // Wenn nach dem Filtern noch Text übrig ist, Flag setzen
      if (b203data.length() > 0) {
          getFlag = 1;
      }
  }
  
  // ==========================================   
  // DATENVERARBEITUNG & WEBSOCKET-VERSAND
  // ==========================================   
  if ((b203data.length() > 0) && (getFlag == 1)) {
      Serial.println(b203data);
      ws.textAll(b203data);
      
      b203data = ""; // Sicherer als '\0' bei Arduino-Strings
      getFlag = 0;
  }

  // ==========================================
  // WEB-/MANUELLER BUTTON-PFAD (MIT MAXIMALEM DEBUG)
  // ==========================================
  if (buttonHold == 1) {
      Serial.println(F("\n--- [DEBUG] Button-Pfad aktiv ---"));
      Serial.print(F("[DEBUG] Gesuchter buttonName: '"));
      Serial.print(buttonName);
      Serial.println(F("'"));

      int a = 0;
      bool buttonFound = false;

      while (strcmp(configArray[a].btnID, "none") != 0 && configArray[a].btnID[0] != '\0') {
          
          // Debugging für jeden Array-Durchlauf (optional, sehr gesprächig)
          /*
          Serial.print(F("[DEBUG] Prüfe Index ")); Serial.print(a);
          Serial.print(F(": configArray[a].btnID = '")); Serial.print(configArray[a].btnID); Serial.println(F("'"));
          */

          if (strcmp(buttonName, configArray[a].btnID) == 0) {
              buttonFound = true;
              Serial.print(F("[DEBUG] TREFFER! Button in Konfiguration gefunden an Index: "));
              Serial.println(a);
              Serial.print(F("[DEBUG] Zugeordnetes Ziel-Gerät (config): '"));
              Serial.print(configArray[a].device);
              Serial.println(F("'"));
              
              Serial.print(F("[DEBUG] Starte Port-Tabellen-Prüfung. Einträge gesamt: "));
              Serial.println(portTableSize);

              for (int i = 0; i < portTableSize; i++) {
                  String currentOut = portArray[i].out;
                  String currentDescr = portArray[i].descr;
                  
                  bool deviceMatch = (strcmp(configArray[a].device, currentDescr.c_str()) == 0) || 
                                     (strstr(configArray[a].device, currentDescr.c_str()) != NULL);

                  // Ausführlicher Vergleichs-Log für jeden Port
                  Serial.print(F("  -> Port [")); Serial.print(i); Serial.print(F("] Descr: '")); Serial.print(currentDescr);
                  Serial.print(F("', Out: '")); Serial.print(currentOut);
                  Serial.print(F("' -> Match? ")); Serial.println(deviceMatch ? F("JA") : F("NEIN"));

                  if (deviceMatch && currentOut != "no") {
                      Serial.println(F("     [DEBUG] Port-Bedingungen erfüllt! Verarbeite Befehl..."));
                      
                      if (strcmp(configArray[a].btnID, "b203reset") == 0) {
                          Serial.println(F("     [DEBUG] Führe b203reset aus..."));
                          if (configArray[a].command != 0x40) {
                              sendRevoxFrame(configArray[a].address, configArray[a].command, 1);
                          }
                          if (configArray[a].repeat == 0) {
                              buttonHold = 0;
                          }
                      }
                      else if (configArray[a].cmdFlag > 0) {
                          Serial.print(F("     [DEBUG] cmdFlag > 0 erkannt. Serieller Befehl. Status b203ReadyToSend: "));
                          Serial.println(b203ReadyToSend ? F("BEREIT") : F("BLOCKIERT (XOFF)"));
                          
                          if (b203ReadyToSend) { 
                              Serial2.print(currentOut);
                              Serial2.print(configArray[a].serCmd);
                              Serial2.print("\r");
                              
                              Serial.print(F("     [XON-SEND] "));
                              Serial.print(currentOut);
                              Serial.println(configArray[a].serCmd);
                          } else {
                              Serial.println(F("     [WARNUNG] Befehl verworfen, da B203 im XOFF-Status ist!"));
                          }

                          if (configArray[a].repeat == 0) {
                              buttonHold = 0;
                          }
                      } 
                      else if (configArray[a].cmdFlag == 0) {
                          Serial.print(F("     [DEBUG] cmdFlag == 0 erkannt. isBibus = "));
                          Serial.println(configArray[a].isBibus);
                          
                          if (configArray[a].command != 0x40) { 
                              if (configArray[a].isBibus == 1) {
                                  Serial.print(F("     [DEBUG] BiBus-Pfad aktiv. Status b203ReadyToSend: "));
                                  Serial.println(b203ReadyToSend ? F("BEREIT") : F("BLOCKIERT"));
                                  
                                  if (b203ReadyToSend) { 
                                  char sendBuffer[64]; 

                                  // 1. "0x" oder "0X" am Anfang überspringen
                                  const char* bibusPtr = configArray[a].bibusCmd;
                                  if (strncmp(bibusPtr, "0x", 2) == 0 || strncmp(bibusPtr, "0X", 2) == 0) {
                                    bibusPtr += 2;
                                  }

                                  // 2. Auf exakt 5 Zeichen formatieren (mit führenden Nullen auffüllen)
                                  char bibusFormatiert[6]; // 5 Zeichen + 1 Nullterminator
                                  int len = strlen(bibusPtr);

                                  if (len >= 5) {
                                    // Falls der String zu lang ist, nehmen wir die letzten 5 Zeichen
                                    snprintf(bibusFormatiert, sizeof(bibusFormatiert), "%s", bibusPtr + (len - 5));
                                  } else {
                                  // Falls er zu kurz ist (z.B. "40"), füllen wir links mit Nullen auf ("00040")
                                    snprintf(bibusFormatiert, sizeof(bibusFormatiert), "%05X", (unsigned int)strtol(bibusPtr, NULL, 16));
                                  }

                                  // 3. Zusammenbauen des neuen Sende-Strings (OHNE currentOut)
                                  // Format wird zu: B + 5-stelliger BiBus-Code + \r
                                  snprintf(sendBuffer, sizeof(sendBuffer), "B%s\r", bibusFormatiert);

                                  // 4. Absenden über die Schnittstellen
                                  Serial2.print(sendBuffer);

                                  Serial.print(F("     [BIBUS-SEND-5CHAR] "));
                                  Serial.println(sendBuffer);
                                  }

                                  } else {
                                    Serial.print(F("     [DEBUG] Native Revox Frame gesendet. Addr: 0x"));
                                    Serial.print(configArray[a].address, HEX);
                                    Serial.print(F(", Cmd: 0x"));
                                    Serial.println(configArray[a].command, HEX);
                                    sendRevoxFrame(configArray[a].address, configArray[a].command, 1);
                                  }
                              
                                  if (configArray[a].repeat == 0) {
                                    buttonHold = 0;
                                  }
                          }
                      }
                  } else if (deviceMatch && currentOut == "no") {
                      Serial.println(F("     [DEBUG] Gerät passte, aber Ausgang steht auf 'no'."));
                  }
              }
              break; // Aus der while-Schleife austreten
          }
          ++a;
      }
      
      if (!buttonFound) {
          Serial.print(F("[DEBUG] FEHLER: buttonName '"));
          Serial.print(buttonName);
          Serial.print(F("' wurde im configArray bis Index "));
          Serial.print(a);
          Serial.println(F(" NICHT gefunden (Suche beendet bei 'none')."));
      }
      Serial.println(F("--- [DEBUG] Button-Pfad beendet ---\n"));
  }

  // ==========================================
  // INFRAROT-PFAD (Vollständig geschlossen)
  // ==========================================
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis; 

      if (IrReceiver.decode()) {
          uint32_t combined = ((uint32_t)IrReceiver.decodedIRData.address << 16) | IrReceiver.decodedIRData.command;

          if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
              Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
              IrReceiver.printIRResultRawFormatted(&Serial, true);
              IrReceiver.resume(); 
          } else {
              IrReceiver.printIRResultShort(&Serial);   
          }
          Serial.println();

          if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
              if ((configArray[irid].repeat == 1) && (irid > 0) && (configArray[irid].command != 0x40)) {
                  sendRevoxFrame(configArray[irid].address, configArray[irid].command, 1);
                  Serial.println("Repeated");
                  Serial.print(configArray[irid].address);
                  Serial.println(configArray[irid].command);
              }
          } else {
              irid = 0;
              Serial.println(combined, HEX);
              while (strcmp(configArray[irid].btnID, "none") != 0 && configArray[irid].btnID[0] != '\0') {
                  if ((combined == configArray[irid].irRecvCode) && (combined != 0)) {
                      if ((configArray[irid].address < 0x11) && (configArray[irid].cmdFlag == 0)) {
                          if (configArray[irid].command != 0x40) {
                              sendRevoxFrame(configArray[irid].address, configArray[irid].command, 1);
                              Serial.println("First press");
                              Serial.print(configArray[irid].address);
                              Serial.println(configArray[irid].command);
                          }
                      }
                      break;
                  }
                  ++irid;
              }
          }
          IrReceiver.resume(); 
      } // <--- Schließt IrReceiver.decode()
  } // <--- Schließt die millis()-Zeitabfrage für IR
} // <