// TEST 7 - TX (Nano)
// COPIA EXACTA de tu Test 3 que funcionó.
// Sin setRepeatTransmit, sin nada especial.
#include <Arduino.h>
#include <RCSwitch.h>

RCSwitch rc = RCSwitch();
const uint8_t TX_PIN = 10;

void setup() {
  Serial.begin(9600);
  rc.enableTransmit(TX_PIN);
  Serial.println("TEST7 TX - Copia exacta de tu Test3");
}

void loop() {
  Serial.println("Enviando trama...");
  rc.send(0xAA0101, 24);
  delay(100);
  rc.send(0x02D700, 24);
  delay(100);
  rc.send(0x4F4E00, 24);
  delay(100);
  delay(3000);
}
