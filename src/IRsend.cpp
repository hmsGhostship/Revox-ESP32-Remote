#include "IRsend.h"

void setupIRoutPin() {
    pinMode(IRoutPin1, OUTPUT);
    pinMode(IRoutPin2, OUTPUT);
    //pinMode(BUTTON_PIN, INPUT_PULLUP); // Interner Pull-up
}

void IRoutPin(bool status) {
  digitalWrite(IRoutPin1, status);
  digitalWrite(IRoutPin2, status);
}

void sendIR(String address, String ITTcode){

// FunktionsendIR("1000", "111111"); //Adressierung 1000 oder 0000=sekundäre Adresse bei Revox

IRoutPin(LOW);  // Pin auf LOW setzen
IRoutPin(HIGH); // Pin auf HIGH setzen
delayMicroseconds(15);      // 15 Mikrosekunden Impuls
IRoutPin(LOW);  // Pin auf LOW setzen
delayMicroseconds(435);      // Vorbereitungspuls

IRoutPin(HIGH); // Pin auf HIGH setzen
delayMicroseconds(15);      // 15 Mikrosekunden warten
IRoutPin(LOW);  // Pin auf LOW setzen
delayMicroseconds(135);      // Startpuls


char serCode[10]; //Arrayvariable vorbereiten zum einzeln ablesen
address.toCharArray(serCode, address.length() + 1);   


for (byte i = 0; i < 4; i = i + 1) {
  
  IRoutPin(HIGH); // Pin auf HIGH setzen
  delayMicroseconds(15);      // 15 Mikrosekunden warten
  IRoutPin(LOW);  // Pin auf LOW setzen

switch (serCode[i]){
  case '0':
  delayMicroseconds(285);      // 285 Mikrosekunden warten
  break;

  case '1':
  delayMicroseconds(135);      // 135 Mikrosekunden warten
  break;
}

}  // Ende Adressierung 1000=Bank1, oder 0000=Bank2 möglich

ITTcode.toCharArray(serCode, ITTcode.length() + 1);

for (byte i = 0; i < 6; i = i + 1) {

  IRoutPin(HIGH); // Pin auf HIGH setzen
  delayMicroseconds(15);      // 215Mikrosekunden warten
  IRoutPin(LOW);  // Pin auf LOW setzen

switch (serCode[i]){
  case '0':
  delayMicroseconds(285);      // 285 Mikrosekunden warten
  break;
  case '1':
  delayMicroseconds(135);      // 135 Mikrosekunden warten
  break;
}
} //ende ITTcode-Ausführung

//Abschlusscode
  IRoutPin(HIGH); // Pin auf HIGH setzen
  delayMicroseconds(15);      // 15 Mikrosekunden warten
  IRoutPin(LOW);  // Pin auf LOW setzen
  delayMicroseconds(435);      // 435 Mikrosekunden Stoppuls
  IRoutPin(HIGH); // Pin auf HIGH setzen
  delayMicroseconds(15);      // 15 Mikrosekunden warten
  IRoutPin(LOW);  // Pin auf LOW setzen
}