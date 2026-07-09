// TEST 4 - TX (Nano)
// Envía los mismos 3 códigos que en Test 3, pero usando la clase RFProtocolTx
// con setRepeatTransmit(2) para verificar que el TX sale correctamente.
#include <Arduino.h>
#include <RCSwitch.h>

RCSwitch rc = RCSwitch();
const uint8_t TX_PIN = 10;

void setup() {
  Serial.begin(9600);
  rc.enableTransmit(TX_PIN);
  rc.setRepeatTransmit(2);  // mínimo para que RX valide
  Serial.println("TEST4 TX - setRepeatTransmit(2), 3 codigos, loop cada 5s");
}

void loop() {
  Serial.println("--- Enviando trama ---");

  Serial.print("  Codigo 0: 0xAA0101 ... ");
  rc.send(0xAA0101, 24);
  Serial.println("enviado");
  delay(80);

  Serial.print("  Codigo 1: 0x02D700 ... ");
  rc.send(0x02D700, 24);
  Serial.println("enviado");
  delay(80);

  Serial.print("  Codigo 2: 0x4F4E00 ... ");
  rc.send(0x4F4E00, 24);
  Serial.println("enviado");

  Serial.println("--- Trama completa ---\n");
  delay(5000);
}
