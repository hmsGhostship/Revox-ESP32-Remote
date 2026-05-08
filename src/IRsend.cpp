#include "IRsend.h"

void setupIRoutPin() {
    pinMode(IRoutPin, OUTPUT);
    //pinMode(BUTTON_PIN, INPUT_PULLUP); // Interner Pull-up
}


void sendIR(String address, String ITTcode){
// FunktionsendIR("1000", "111111"); //Adressierung 1000 oder 0000=sekundäre Adresse bei Revox

digitalWrite(IRoutPin, LOW);  // Pin auf LOW setzen
digitalWrite(IRoutPin, HIGH); // Pin auf HIGH setzen
delayMicroseconds(15);      // 15 Mikrosekunden Impuls
digitalWrite(IRoutPin, LOW);  // Pin auf LOW setzen
delayMicroseconds(435);      // Vorbereitungspuls

digitalWrite(IRoutPin, HIGH); // Pin auf HIGH setzen
delayMicroseconds(15);      // 15 Mikrosekunden warten
digitalWrite(IRoutPin, LOW);  // Pin auf LOW setzen
delayMicroseconds(135);      // Startpuls


char serCode[10]; //Arrayvariable vorbereiten zum einzeln ablesen
address.toCharArray(serCode, address.length() + 1);   


for (byte i = 0; i < 4; i = i + 1) {
  
  digitalWrite(IRoutPin, HIGH); // Pin auf HIGH setzen
  delayMicroseconds(15);      // 15 Mikrosekunden warten
  digitalWrite(IRoutPin, LOW);  // Pin auf LOW setzen

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

  digitalWrite(IRoutPin, HIGH); // Pin auf HIGH setzen
  delayMicroseconds(15);      // 215Mikrosekunden warten
  digitalWrite(IRoutPin, LOW);  // Pin auf LOW setzen

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
  digitalWrite(IRoutPin, HIGH); // Pin auf HIGH setzen
  delayMicroseconds(15);      // 15 Mikrosekunden warten
  digitalWrite(IRoutPin, LOW);  // Pin auf LOW setzen
  delayMicroseconds(435);      // 435 Mikrosekunden Stoppuls
  digitalWrite(IRoutPin, HIGH); // Pin auf HIGH setzen
  delayMicroseconds(15);      // 15 Mikrosekunden warten
  digitalWrite(IRoutPin, LOW);  // Pin auf LOW setzen
}