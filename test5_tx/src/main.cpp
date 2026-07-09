// TEST 5 - TX (Nano)
// setRepeatTransmit(3) = MÍNIMO REAL para que RCSwitch RX valide.
// RCSwitch necesita ver 2 gaps de sync → requiere 3 repeticiones mínimo.
#include <Arduino.h>
#include <RCSwitch.h>

RCSwitch rc = RCSwitch();
const uint8_t TX_PIN = 10;

void setup() {
  Serial.begin(9600);
  rc.enableTransmit(TX_PIN);
  rc.setRepeatTransmit(3);  // MÍNIMO para que el RX valide (necesita 2 gaps = 3 reps)
  Serial.println("TEST5 TX - setRepeatTransmit(3), 3 codigos, loop cada 5s");
}

void loop() {
  Serial.println("--- Enviando trama ---");

  Serial.print("  Codigo 0: 0xAA0101 ... ");
  rc.send(0xAA0101, 24);
  Serial.println("enviado");
  delay(150);

  Serial.print("  Codigo 1: 0x02D700 ... ");
  rc.send(0x02D700, 24);
  Serial.println("enviado");
  delay(150);

  Serial.print("  Codigo 2: 0x4F4E00 ... ");
  rc.send(0x4F4E00, 24);
  Serial.println("enviado");

  Serial.println("--- Trama completa ---\n");
  delay(5000);
}
