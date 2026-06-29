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

unsigned long lastActivity = 0;

bool b203ReadyToSend = true; // Steuert den XON/XOFF Fluss

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        // Der ESP32 wurde gerade durch WLAN-Daten geweckt und verarbeitet sie jetzt!
        Serial.println(F("\n[WLAN-WAKEUP] WebSocket-Daten empfangen!"));
    }
}

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
  // 1. Prüfen, ob die Datei überhaupt existiert
  if (!LittleFS.exists(PortConfigPath)) {
    Serial.println("Port-Konfiguration existiert nicht im Dateisystem!");
    return;
  }

  // 2. Datei öffnen
  File file = LittleFS.open(PortConfigPath, "r");
  if (!file) {
    // WICHTIG: Wenn die Datei blockiert ist, brechen wir SOFORT ab!
    // Die alten Ports bleiben im RAM geschützt.
    Serial.println("WARNUNG: Port Configuration blockiert oder konnte nicht geöffnet werden! Lade-Vorgang abgebrochen.");
    return;
  }
  
  // 3. Prüfen, ob die Datei vielleicht temporär 0 Bytes groß ist (Sicherheitsanker)
  if (file.size() == 0) {
    Serial.println("WARNUNG: portConfig.json ist im Dateisystem aktuell 0 Bytes groß. Lade-Vorgang abgebrochen.");
    file.close();
    return;
  }

  JsonDocument doc;

  // 4. Datei parsen
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print("Fehler beim Parsen der JSON: ");
    Serial.println(error.f_str());
    file.close();
    // WICHTIG: Bei Fehlern NIEMALS weitermachen!
    return;
  }

  file.close(); // Datei sofort schließen, Daten liegen sicher im doc-RAM
  
  // 5. Erst WENN das JSON fehlerfrei gelesen wurde, verarbeiten wir die Daten
  JsonArray array = doc.as<JsonArray>();
  if (array.isNull() || array.size() == 0) {
    Serial.println("WARNUNG: Gelesenes JSON-Array ist leer oder ungültig! RAM wird nicht überschrieben.");
    return;
  }

  int maxArrayCapacity = sizeof(portArray) / sizeof(portArray[0]);
  int temporaereGroesse = 0;

  if (array.size() > maxArrayCapacity) {
    temporaereGroesse = maxArrayCapacity;
    Serial.print("Warnung: JSON zu groß! Begrenzt auf: ");
    Serial.println(maxArrayCapacity);
  } else {
    temporaereGroesse = array.size();
  }
  
  // Daten temporär zwischenspeichern, um das echte Array erst bei Erfolg zu beschreiben
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

    Serial.printf("RAM-Port geladen [%d]: %s (%s) -> Port: %s\n", 
                  index, portArray[index].name, portArray[index].descr, portArray[index].out);

    index++;
  }

  // Erst ganz am Ende, wenn alles fehlerfrei durchlief, setzen wir die globale Größe
  portTableSize = temporaereGroesse;
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

    // 2. CONFIG ROUTEN
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
      if (LittleFS.exists(ConfigPath)) {
        request->send(LittleFS, ConfigPath, "application/json");
      } else {
        request->send(200, "application/json", "[]");
      }
    });

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
          Serial.println("Config-Datei im LittleFS erfolgreich aktualisiert.");
          loadCommandConfig(); 
        }
        request->send(200, "text/plain", "OK");
      }
    });

    // 3. POST-ROUTE FÜR /api/save-data (Mit String-Puffer)
    server.on("/api/save-data", HTTP_POST, [](AsyncWebServerRequest *request) {
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      static String jsonBuffer = "";
      if (index == 0) {
        jsonBuffer = "";
      }
      for (size_t i = 0; i < len; i++) {
        jsonBuffer += (char)data[i];
      }
      if (index + len == total) {
        File portFile = LittleFS.open(PortConfigPath, "w");
        if (portFile) {
          portFile.print(jsonBuffer);
          portFile.close();
          Serial.println("Port-Konfiguration erfolgreich im LittleFS gespeichert.");
          loadPortConfig(); 
          if (portExists("B285")) {
            htmlPath = "/receiver.html.gz";
          } else {
            htmlPath = "/amplifier.html.gz";
          }
        }
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"JSON gespeichert\"}");
        jsonBuffer = "";
      }
    });

    // 4. STATISCHE DATEIEN (NUR STRUKTUREN FREIGEBEN)
    server.serveStatic("/css/", LittleFS, "/css/");
    server.serveStatic("/js/", LittleFS, "/js/");
    server.serveStatic("/style.css", LittleFS, "/style.css");
    server.serveStatic("/script.js", LittleFS, "/script.js");
    server.serveStatic("/favicon.ico", LittleFS, "/favicon.ico");

    // WICHTIG: Die drei server.serveStatic für .json WURDEN HIER ENTFERNT!

    // Fallback für dynamische JSONs mit Cache-Breaker (?v=...) und HTML-Seiten
    server.onNotFound([](AsyncWebServerRequest *request) {
      String url = request->url(); // Gibt den reinen Pfad OHNE das "?v=..." zurück!

      // Schutz vor leeren oder reinen Slash-Anfragen
      if (url == "/" || url.length() == 0) {
        String pathToSend = String(htmlPath);
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, pathToSend, "text/html");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
        return;
      }
      
      // NEU: Flexibles Laden aller JSON-Dateien (ignoriert den Cache-Breaker im Browser)
      if (url.endsWith(".json")) {
        if (LittleFS.exists(url)) {
          request->send(LittleFS, url, "application/json");
          return;
        }
      }

      // Fallback für explizite HTML-Endungen
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
    
    // TIPP: Hier KEIN Stromsparen aktivieren, damit das Konfigurations-WLAN (AP) stabil bleibt!
    
  } else {
    Serial.print("Erfolgreich verbunden! IP: ");
    Serial.println(WiFi.localIP());

    // === ERGÄNZUNG FÜR LIGHT SLEEP ===
    // Nur wenn die Verbindung zum Heim-WLAN steht, aktivieren wir das automatische Stromsparen.
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    Serial.println("WLAN-Stromsparmodus (Light Sleep) erfolgreich aktiviert.");
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

  // STROMSPAREN ÜBER DEFINE STEUERN
  #ifdef ENABLE_LOW_POWER
    if (WiFi.status() == WL_CONNECTED) {
      esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
      Serial.println("[POWER] WLAN-Stromsparmodus (Light Sleep) AKTIVIERT.");
    }
  #else
    esp_wifi_set_ps(WIFI_PS_NONE); // Erzwinge volle Leistung
    Serial.println("[POWER] Stromsparmodus DEAKTIVIERT (Volle Leistung).");
  #endif

  // IR-receiver starten
  pinMode(IR_RECEIVE_PIN, INPUT_PULLUP); 
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  Serial.println("IR Empfaenger aktiviert");

  // GPIO-WAKEUP ÜBER DEFINE STEUERN
  #ifdef ENABLE_LOW_POWER
    gpio_wakeup_enable((gpio_num_t)IR_RECEIVE_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    Serial.println("[POWER] GPIO Wakeup fuer IR-Pin eingerichtet.");
  #endif

  Serial.print("ESP IP-Adresse: ");
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

      // 3. POST-ROUTE FÜR /api/save-data (Mit Schutzwall gegen leere Geister-Requests)
    server.on("/api/save-data", HTTP_POST, [](AsyncWebServerRequest *request) {
      // Antwort erfolgt im Body-Handler unten
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  
      static String jsonBuffer = "";

      if (index == 0) {
        jsonBuffer = "";
      }

      for (size_t i = 0; i < len; i++) {
        jsonBuffer += (char)data[i];
      }

      if (index + len == total) {
        // SCHUTZWALL: Wenn der Buffer leer, nur "[]" ist oder weniger als 15 Zeichen hat, 
        // ignorieren wir den Schreibvorgang komplett! Das blockiert den Geister-Request.
        if (jsonBuffer.length() < 15 || jsonBuffer == "[]" || jsonBuffer == "[\n]") {
          Serial.println("WARNUNG: Leerer oder ungültiger Geister-POST blockiert! Datei wird NICHT überschrieben.");
          request->send(200, "application/json", "{\"status\":\"ignored\",\"message\":\"Leere Daten ignoriert\"}");
          jsonBuffer = "";
          return;
        }

        // Wenn die Daten valide sind, wird wie gewohnt geschrieben:
        File portFile = LittleFS.open(PortConfigPath, "w");
        if (portFile) {
          portFile.print(jsonBuffer);
          portFile.close();
          Serial.println("Port-Konfiguration erfolgreich im LittleFS gespeichert.");
          Serial.print("Gespeicherter Inhalt: ");
          Serial.println(jsonBuffer); 
          
          loadPortConfig(); 

          if (portExists("B285")) {
            htmlPath = "/receiver.html.gz";
          } else {
            htmlPath = "/amplifier.html.gz";
          }
        } else {
          Serial.println("Fehler: PortConfigPath konnte nicht zum Schreiben geöffnet werden!");
        }
        
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"JSON gespeichert\"}");
        jsonBuffer = "";
      }
    });


  Serial.print("HTTP server started");
  server.begin();
  Serial.println();
  Serial.print("Revox-Remote ist bereit");
  Serial.println();

  btStop();

}

void loop() {
  // ==========================================   
  // 0. SYSTEM-STATUS (OPTIONAL: WLAN-WAKEUP ANZEIGE)
  // ==========================================   
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_WIFI) {
      // Kann einkommentiert werden, falls Sie Netzwerk-Wakeups sehen wollen
      // Serial.println(F("[WLAN-WAKEUP]"));
  }

  // ==========================================   
  // WEBSOCKET & NETWORK MAINTENANCE
  // ==========================================   
  ws.cleanupClients();

  if (millis() - lastPingTime >= pingInterval) {
      lastPingTime = millis();
      ws.pingAll(); 
  }
  
  // ==========================================   
  // REVOLUTIONIERT: URSPRÜNGLICHES LESEN + XON/XOFF PRÜFUNG
  // ==========================================   
  if (Serial2.available() > 0) {
      b203data = Serial2.readStringUntil('\n'); 
      
      if (b203data.endsWith("\r")) {
          b203data.remove(b203data.length() - 1);
      }
      
      if (b203data.indexOf((char)0x13) != -1) {
          b203ReadyToSend = false;
          Serial.println(F("[B203] XOFF empfangen - Senden blockiert"));
          b203data.replace(String((char)0x13), ""); 
      }
      else if (b203data.indexOf((char)0x11) != -1) {
          b203ReadyToSend = true;
          Serial.println(F("[B203] XON empfangen - Senden freigegeben"));
          b203data.replace(String((char)0x11), ""); 
      }

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
      
      b203data = ""; 
      getFlag = 0;
  }

  // ==========================================
  // WEB-/MANUELLER BUTTON-PFAD (IHR ORIGINALER CODE)
  // ==========================================
  if (buttonHold == 1) {
      Serial.println(F("\n--- [DEBUG] Button-Pfad aktiv ---"));
      Serial.print(F("[DEBUG] Gesuchter buttonName: '"));
      Serial.print(buttonName);
      Serial.println(F("'"));

      int a = 0;
      bool buttonFound = false;

      while (configArray[a].btnID[0] != '\0' && strcmp(configArray[a].btnID, "none") != 0) {
          
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
                                      const char* bibusPtr = configArray[a].bibusCmd;
                                      if (strncmp(bibusPtr, "0x", 2) == 0 || strncmp(bibusPtr, "0X", 2) == 0) {
                                        bibusPtr += 2;
                                      }

                                      char bibusFormatiert[6]; 
                                      int len = strlen(bibusPtr);

                                      if (len >= 5) {
                                        snprintf(bibusFormatiert, sizeof(bibusFormatiert), "%s", bibusPtr + (len - 5));
                                      } else {
                                        snprintf(bibusFormatiert, sizeof(bibusFormatiert), "%05X", (unsigned int)strtol(bibusPtr, NULL, 16));
                                      }

                                      snprintf(sendBuffer, sizeof(sendBuffer), "B%s\r", bibusFormatiert);
                                      Serial2.print(sendBuffer);

                                      Serial.print(F("     [BIBUS-SEND-5CHAR] "));
                                      Serial.println(sendBuffer);
                                  }
                              } else {
                                  // IHR ORIGINALER REVOX-NATIVE PFAD
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
              break; 
          }
          ++a;
      }
      
      if (!buttonFound) {
          Serial.print(F("[DEBUG] FEHLER: buttonName '"));
          Serial.print(buttonName);
          Serial.print(F("' wurde im configArray bis Index "));
          Serial.print(a);
          Serial.println(F(" NICHT gefunden (Suche beendet bei 'none')."));
          buttonHold = 0;
      }
      Serial.println(F("--- [DEBUG] Button-Pfad beendet ---\n"));
  }

  // ==========================================
  // INFRAROT-PFAD (IHR ORIGINALER CODE)
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
      lastActivity = millis();
			while (configArray[irid].btnID[0] != '\0' && strcmp(configArray[irid].btnID, "none") != 0) {
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
		}
	}
	// ==========================================
  // KORRIGIERT: 6. DYNAMISCHES SCHLAF-FENSTER (MIT SPERRZEIT)
  // ==========================================
  // Wenn ein WebSocket-Button aktiv ist oder serielle Daten anliegen, halten wir das System wach
  if (buttonHold > 0 || Serial2.available() > 0 || b203data.length() > 0) {
      lastActivity = millis(); // Aktivität registrieren -> Wachbleiben!
  }

  // Erst wenn seit der letzten Aktivität (IR, Web, Serial) mehr als 2000ms vergangen sind,
  // erlauben wir dem ESP32 wieder den Wechsel in den Light Sleep.
  if (millis() - lastActivity >= 2000) {
      delay(10); // Erlaubt 10ms Stromsparen im Hintergrund
  } else {
      delay(1);  // Hält die CPU hellwach und reaktionsschnell für Folgesignale (z.B. Repeats)
  }
}