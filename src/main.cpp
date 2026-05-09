/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-websocket-server-arduino/
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

// Import required libraries
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
// HardwareSerial Serial2(16, 17);
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

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "secrets.h"
#include "IRsend.h"
#include "config.h"

bool State = 0;
bool buttonHold = 0;
bool cmdRelease = 0;
bool checkbox =0;
bool wsopen =0;
unsigned long previousMillis = 0;
const long interval = 130;
char buttonName[18];
String b203data;

//byte xonxoffstate = 0; // 1 means don't send data

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const uint16_t kRecvPin = 25;
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;

IRrecv irrecv(kRecvPin);
decode_results results;

/*void notifyClients() {
  Serial.println("notifyData: " + b203data);
  ws.textAll("notifyData: " + b203data);
}*/

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    Serial.println("Message: " + message);

    if (strncmp((char*)data, "button", 6) == 0) {
      data +=6;
      if (strncmp((char*)data, "Push", 4) == 0){
      strncpy(buttonName, (char*)data + 4, sizeof(buttonName));
      buttonHold = 1;
      cmdRelease = 1;
      } else if (strncmp((char*)data, "Release", 7) == 0){
      buttonHold = 0;
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
              char device[5];
              strncpy( device, (char*)data, sizeof(device));
              int i2 = 0;
              while( outputTable[i2].descr != NULL) {
                if ( strcmp ( device, outputTable[i2].descr ) == 0) {
                  Serial2.print( outputTable[i2].out );
                  Serial2.print( b215settingsBytes );
                  Serial2.print("\r");
                  Serial.print( outputTable[i2].out );
                  Serial.print( b215settingsBytes );
                break;
                }
              ++i2;
              }
    }

    else if (strncmp((char*)data, "cdplayer", 8) == 0) {
              char b226settingsBytes[2];
              strncpy( b226settingsBytes, (char*)data + 8, sizeof(b226settingsBytes));
              char device[8];
              strncpy( device, (char*)data, sizeof(device));
              int i2 = 0;
              while( outputTable[i2].descr != NULL) {
                if ( strcmp ( device, outputTable[i2].descr ) == 0) {
                  Serial2.print( outputTable[i2].out );
                  Serial2.print( b226settingsBytes );
                  Serial2.print("\r");
                  Serial.print( outputTable[i2].out );
                  Serial.print( b226settingsBytes );
                break;
                }
              ++i2;
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
      if ((strncmp((char*)data, "true", 4) == 0) || (strncmp((char*)data, "false", 5) == 0)) {
      strncpy(buttonName, (char*)data, sizeof(buttonName));
      cmdRelease = 1;
      Serial.println(buttonName);
      }
    }

    else if (strncmp((char*)data, "page", 4) == 0) {
      data +=4;
      if (strncmp((char*)data, "Release", 7) == 0){
      strncpy(buttonName, (char*)data + 7, sizeof(buttonName));
      cmdRelease = 1;
      }
    }
    /*else if (strncmp((char*)data, "ConnectionOpen", 14) == 0) {
    wsopen = 1;
    }*/
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
  // Serial port for debugging purposes
  Serial.begin(115200); // Start serial
  /*while (!Serial)  // Wait for the serial connection to be establised.
   delay(50);*/

  pinMode(kRecvPin, INPUT);

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
 
  irrecv.enableIRIn();
  Serial.println("IR Empfaenger aktiviert");
  // Start the receiver


 // Connect to WiFi network
 // Connect to Wi-Fi
  WiFi.begin(ssid, passPhrase);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  WiFi.setSleep(false);
  
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  setupServerRoutes();

  // Route for root / web page
	/*server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { 
	   Serial.println("ESP32 Web Server: New request received:");  // for debugging 
	   Serial.println("GET /");        // for debugging 
	   request->send(LittleFS, "/index.html", ""); 
	 });*/

  /*server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false, processor);
  });*/

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
    //Serial.println(b203data);
    //notifyClients();
  }

if ((wsopen == 1) && (b203data != "")) {
      ws.textAll(b203data);
      b203data = '\0';
}

  if ((buttonHold == 1) || (irrecv.decode(&results)) || (cmdRelease == 1)) {
  irrecv.resume();  // Receive the next value

      unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval){
     previousMillis = currentMillis; // Zeit merken
      
      int i = 0;
      while( cmdTable[i].btnName != NULL ) {
        if (( strcmp( buttonName, cmdTable[i].btnName ) == 0)  || ( results.value != 0 && results.value == cmdTable[i].irRecCode ) ) {
            if ( cmdTable[i].address != NULL ) {
            sendIR( cmdTable[i].address, cmdTable[i].ITTcode );
              if ( cmdTable[i].once == 1 ) {
              buttonHold = 0;
              }
            }
            if (( cmdTable[i].serCmd != NULL ) && ( cmdTable[i].cmdFlag > 0 ) && (cmdRelease == 1)) {
              int i2 = 0;
              while( outputTable[i2].descr != NULL) {
                if ( strcmp ( cmdTable[i].device, outputTable[i2].descr ) == 0) {
                  if (cmdTable[i].cmdFlag == 1) {
                  Serial2.print( outputTable[i2].out );
                  Serial.print( outputTable[i2].out );
                  }
                  Serial2.print( cmdTable[i].serCmd );
                  Serial2.print("\r");
                  Serial.println( cmdTable[i].serCmd );
                    if ( cmdTable[i].once == 1 ) {
                    cmdRelease = 0;
                    }
                break;
                }
              ++i2;
              }
            }
            break;
          }
          ++i;  // nächster Eintrag
      }
    }
  }
};