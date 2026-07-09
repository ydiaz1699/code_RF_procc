// TEST 7 - RX (Uno)
// COPIA EXACTA de tu Test 3 que funcionó. Solo para confirmar
// que el hardware sigue OK y que no es un problema de conexiones.
#include <Arduino.h>
#include <RCSwitch.h>

RCSwitch rc = RCSwitch();

void setup() {
  Serial.begin(9600);
  rc.enableReceive(0);  // interrupcion 0 = pin D2, EXACTO como tu test 3
  Serial.println("TEST7 RX - Copia exacta de tu Test3 que funciono");
}

void loop() {
  if (rc.available()) {
    Serial.print("RX: 0x");
    Serial.println(rc.getReceivedValue(), HEX);
    rc.resetAvailable();
  }
}
