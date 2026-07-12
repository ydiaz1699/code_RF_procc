// TEST 6 - TX (Nano)
// Usa default nRepeatTransmit=10 (que sabemos que funciona con este hardware).
// delay(200) entre códigos para dar separación clara.
#include <Arduino.h>
#include <RCSwitch.h>

RCSwitch rc = RCSwitch();
const uint8_t TX_PIN = 10;

void setup() {
  Serial.begin(9600);
  rc.enableTransmit(TX_PIN);
  // NO setRepeatTransmit - usa default (10)
  Serial.println("TEST6 TX - default reps(10), delay(200) entre codigos");
}

void loop() {
  Serial.println("--- Enviando trama ---");

  Serial.print("  0xAA0101...");
  rc.send(0xAA0101, 24);
  Serial.println("ok");
  delay(200);

  Serial.print("  0x02D700...");
  rc.send(0x02D700, 24);
  Serial.println("ok");
  delay(200);

  Serial.print("  0x4F4E00...");
  rc.send(0x4F4E00, 24);
  Serial.println("ok");

  Serial.println("--- FIN ---\n");
  delay(5000);
}
