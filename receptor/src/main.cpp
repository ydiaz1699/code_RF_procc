#include <Arduino.h>
#include "RFProtocol.h"

const uint8_t RX_INTERRUPT_PIN = 2;  // D2 = interrupción 0 en el Uno

RFProtocolRx rf(RX_INTERRUPT_PIN);

int16_t lastSeq = -1;
unsigned long lastSeqMs = 0;
const unsigned long DEDUPE_WINDOW_MS = 800;  // cubre las 3 repeticiones (gap de 200ms cada una)

void setup() {
  Serial.begin(9600);
  rf.begin();
  Serial.println("Receptor listo (protocolo empaquetado)");
}

void loop() {
  rf.update();

  if (rf.available()) {
    Packet p = rf.read();

    bool isDuplicate = (p.seq == lastSeq) && (millis() - lastSeqMs < DEDUPE_WINDOW_MS);
    lastSeq = p.seq;
    lastSeqMs = millis();

    if (isDuplicate) return;  // repetición de la misma trama (PACKET_REPEATS), se ignora

    Serial.print("Paquete recibido seq=");
    Serial.print(p.seq);
    Serial.print(" type=0x");
    Serial.print(p.type, HEX);
    Serial.print(" len=");
    Serial.print(p.len);
    Serial.print(" payload=");
    for (uint8_t i = 0; i < p.len; i++) Serial.print((char)p.payload[i]);
    Serial.println();
  }
}
