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
#define RXD2 16
#define TXD2 17
#define REVOX_BAUD 1200

#define USE_LittleFS
#include <FS.h>
#ifdef USE_LittleFS
  #define SPIFFS LITTLEFS
  #include <LITTLEFS.h> 
#else
  #include <SPIFFS.h>
#endif 
#include <IRRemote.hpp>

#include "secrets.h"
#include "IRsend.h"
#include "config.h"

bool State = 0;
bool buttonHold = 0;
bool wsopen =0;
unsigned long previousMillis = 0;
const long interval = 130;
char buttonName[18];
int irid =0;
String b203data;
const int PIN_RECV = 25;

//byte xonxoffstate = 0; // 1 means don't send data

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strncmp((char*)data, "button", 6) == 0) {
      data +=6;
      if (strncmp((char*)data, "Push", 4) == 0){
      strncpy(buttonName, (char*)data + 4, sizeof(buttonName));
      buttonHold = 1;
      } else if (strncmp((char*)data, "Release", 7) == 0){
      buttonHold = 0;
      //cmdRelease = 1;
      }
    }

    else if (strncmp((char*)data, "setup", 5) == 0) {
      char setupBytes[7];
      strncpy( setupBytes, (char*)data + 5, sizeof(setupBytes));
      Serial2.print(setupBytes);
      Serial2.print("\r");
      Serial.println(setupBytes);
    }

    else if (strncmp((char*)data, "getsettings", 11) == 0) {
      char settingsBytes[3];
      strncpy( settingsBytes, (char*)data + 11, sizeof(settingsBytes));
      Serial2.print(settingsBytes);
      Serial2.print("\r");
      Serial.println(settingsBytes);
    }

    else if (strncmp((char*)data, "tape2", 5) == 0) {
              char b215settingsBytes[2];
              strncpy( b215settingsBytes, (char*)data + 5, sizeof(b215settingsBytes));
              int i = 0;
              while( outputTable[i].descr != NULL) {
                if ( strcmp ( "tape2", outputTable[i].descr ) == 0) {
                  Serial2.print( outputTable[i].out );
                  Serial2.print( b215settingsBytes );
                  Serial2.print("\r");
                  Serial.print( outputTable[i].out );
                  Serial.print( b215settingsBytes );
                break;
                }
              ++i;
              }
    }

    else if (strncmp((char*)data, "cdplayer", 8) == 0) {
              char b226settingsBytes[2];
              strncpy( b226settingsBytes, (char*)data + 8, sizeof(b226settingsBytes)); 
              int i = 0;
              while( outputTable[i].descr != NULL) {
                if ( strcmp ("cdplayer", outputTable[i].descr ) == 0) {
                  Serial2.print( outputTable[i].out );
                  Serial2.print( b226settingsBytes );
                  Serial2.print("\r");
                  Serial.print( outputTable[i].out );
                  Serial.print( b226settingsBytes );
                break;
                }
              ++i;
              }
    }


    else if (strncmp((char*)data, "testEvent", 9) == 0) {
      char testEventBytes[5];
      strncpy( testEventBytes, (char*)data + 9, sizeof(testEventBytes));
      Serial2.print(testEventBytes);
      Serial2.print("\r");
      Serial.println(testEventBytes);
    }

    else if (strncmp((char*)data, "setDate", 7) == 0) {
      char setDateBytes[9];
      strncpy( setDateBytes, (char*)data + 7, sizeof(setDateBytes));
      Serial2.print(setDateBytes);
      Serial2.print("\r");
      Serial.println(setDateBytes);
    }

    else if (strncmp((char*)data, "setTime", 7) == 0) {
      char setTimeBytes[9];
      strncpy( setTimeBytes, (char*)data + 7, sizeof(setTimeBytes));
      Serial2.print(setTimeBytes);
      Serial2.print("\r");
      Serial.println(setTimeBytes);
    }

    else if (strncmp((char*)data, "setEvent", 8) == 0) {
      char setEventBytes[32];
      strncpy( setEventBytes, (char*)data + 8, sizeof(setEventBytes));
      Serial2.print(setEventBytes);
      Serial2.print("\r");
      Serial.println(setEventBytes);
    }

    else if (strncmp((char*)data, "callEvent", 9) == 0) {
      char callEventBytes[5];
      strncpy( callEventBytes, (char*)data + 9, sizeof(callEventBytes));
      Serial2.print(callEventBytes);
      Serial2.print("\r");
      Serial.println(callEventBytes);
    }

    else if (strncmp((char*)data, "delEvent", 8) == 0) {
      char delEventBytes[5];
      strncpy( delEventBytes, (char*)data + 8, sizeof(delEventBytes));
      Serial2.print(delEventBytes);
      Serial2.print("\r");
      Serial.println(delEventBytes);
    }

    else if (strncmp((char*)data, "toggle", 6) == 0) {
      data +=6;
      if (strncmp((char*)data, "true", 4) == 0) {
      Serial2.print("0R0");
      Serial2.print("\r");
      Serial.println("0R0");
      } else if (strncmp((char*)data, "false", 5) == 0) {
      Serial2.print("0R1");
      Serial2.print("\r");
      Serial.println("0R1");
      }
    }

    else if (strncmp((char*)data, "page", 4) == 0) {
      data +=4;
      if (strncmp((char*)data, "Release", 7) == 0){
      strncpy(buttonName, (char*)data + 7, sizeof(buttonName));
        int i = 0;
        while( outputTable[i].descr != NULL) {
          if (( strcmp ( buttonName, outputTable[i].descr ) == 0) && ( outputTable[i].feedback == 1)){
          Serial2.print( outputTable[i].out );
          Serial2.print("X");
          Serial2.print("\r");
          Serial.print(outputTable[i].out);
          Serial.println("X");
          break;
          }
          ++i;
        }
      }
    }
  }
}
      
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, 
      void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      wsopen = 1;
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      wsopen = 0;
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
    
    // Auf dem Pfad `/` wird die Datei `index.html` aus dem `data` Ordner ausgeliefert
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
      request->send(LittleFS, "/index.html");
    });
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/b203.html", "text/html");
    });
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/cd.html", "text/html");
    });
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/phono.html", "text/html");
    });
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/tape1.html", "text/html");
    });
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/tape2.html", "text/html");
    });
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/tuner.html", "text/html");
    });
    // Wenn die angeforderte Seite nicht vorhanden ist, `notFound()` aufrufen
    server.onNotFound(notFound);
    server.serveStatic("/", LittleFS, "/");
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

  setupIRoutPin();
  
 Serial2.begin(REVOX_BAUD, SERIAL_8N1, RXD2, TXD2);

 if (Serial2) {
  Serial.println("Revox Serial startet");
 }
  Serial2.setTimeout(5000);
  //Serial2.write(0x13);
  //Serial2.write(0x11);
  
  if (!LittleFS.begin()) // Start LittleFS
  {
    Serial.println("LittleFS mount failed");
    return;
  }
 
  IrReceiver.begin(PIN_RECV, ENABLE_LED_FEEDBACK);
  Serial.println("IR Empfaenger aktiviert");
  // Start the receiver

 // Connect to WiFi network
  WiFi.begin(ssid, passPhrase);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  WiFi.setSleep(false);
  
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  setupServerRoutes();

  initWebSocket();

  // Start server
  Serial.print("HTTP server started");
  server.begin();
  Serial.println();
  Serial.print("Revox-Remote ist bereit");
  Serial.println();
    
}

void loop() {
  
  ws.cleanupClients();
  
  if (Serial2.available() > 0) {
    b203data = Serial2.readStringUntil('\n'); // Reads until LF or timeout
    Serial.println(b203data);
  }
  
  if ((wsopen == 1 ) && (b203data.length() > 0 )) {
      ws.textAll(b203data);
      b203data = '\0';
      }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval){
  previousMillis = currentMillis; // Zeit merken

    if (IrReceiver.decode()) {
      uint32_t combined = ((uint32_t)IrReceiver.decodedIRData.address << 16) | IrReceiver.decodedIRData.command;
      //Serial.println(combined, HEX );

      if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
        if  ((cmdTable[irid].repeat == 1) && (irid > 0) ) {
        sendIR( cmdTable[irid].address, cmdTable[irid].ITTcode );
        Serial.println("Repeated");
        Serial.println(cmdTable[irid].address);
        Serial.println(cmdTable[irid].ITTcode);
        }
      } else {
        irid = 0;
        Serial.println(combined, HEX );
        while( cmdTable[irid].btnID != NULL ) {
          //Serial.println(irid);
          if ((combined == cmdTable[irid].irRecvCode ) && (combined != 0)) {
            if (( cmdTable[irid].address != NULL ) && ( cmdTable[irid].cmdFlag == 0 )) {
            sendIR( cmdTable[irid].address, cmdTable[irid].ITTcode );
            Serial.println("First press");
            Serial.println(cmdTable[irid].address);
            Serial.println(cmdTable[irid].ITTcode);
            }
          break;
          }
        ++irid;
        }
      }
      IrReceiver.resume(); // Receive the next value 
      } else if (buttonHold == 1) {
      int a = 0;
        while( cmdTable[a].btnID != NULL ) {
          if (strcmp(buttonName, cmdTable[a].btnID) == 0) { 
          int i = 0;
            while( outputTable[i].descr != NULL) {
              if (( strcmp ( cmdTable[a].device, outputTable[i].descr ) == 0)) {
                if (cmdTable[a].cmdFlag > 0 ){
                Serial2.print( outputTable[i].out );
                Serial2.print( cmdTable[a].serCmd );
                Serial2.print("\r");
                Serial.print( outputTable[i].out );
                Serial.println( cmdTable[a].serCmd );
                  if ( cmdTable[a].repeat == 0 ) {
                  buttonHold = 0;
                  }
                } else if ((cmdTable[a].cmdFlag == 0 )) {
                  sendIR( cmdTable[a].address, cmdTable[a].ITTcode );
                    if ( cmdTable[a].repeat == 0 ) {
                    buttonHold = 0;
                    }
                  }
              break;
              }
              ++i;
            }
            break;
          }
          ++a;
        }
      }
  }
};