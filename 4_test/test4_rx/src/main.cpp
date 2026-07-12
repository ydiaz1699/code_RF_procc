// TEST 4 - RX (Uno)
// Receptor con debug COMPLETO para ver exactamente qué pasa.
// NO usa la clase RFProtocolRx. Implementa la lógica INLINE con prints.
#include <Arduino.h>
#include <RCSwitch.h>

RCSwitch rc = RCSwitch();

// === Constantes del protocolo ===
const uint8_t SYNC_BYTE   = 0xAA;
const uint8_t MAX_PAYLOAD = 16;

// === Estado ===
enum RxState { WAIT_SYNC, WAIT_HEADER2, WAIT_PAYLOAD };
RxState state = WAIT_SYNC;

uint8_t pkt_seq = 0;
uint8_t pkt_type = 0;
uint8_t pkt_len = 0;
uint8_t pkt_crc = 0;
uint8_t pkt_payload[16];
uint8_t payload_idx = 0;

// === Deduplicación ===
uint32_t lastValue = 0;
unsigned long lastTime = 0;
const unsigned long DEDUPE_MS = 120;

// === CRC8 ===
uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

void setup() {
  Serial.begin(9600);
  rc.enableReceive(digitalPinToInterrupt(2));
  Serial.println("TEST4 RX - Debug completo, pin D2");
  Serial.print("digitalPinToInterrupt(2) = ");
  Serial.println(digitalPinToInterrupt(2));
}

void loop() {
  if (!rc.available()) return;

  uint32_t code = rc.getReceivedValue();
  unsigned int bitlen = rc.getReceivedBitlength();
  rc.resetAvailable();

  if (code == 0) {
    Serial.println("[code=0, ignorado]");
    return;
  }

  // Debug: mostrar TODO lo que llega
  unsigned long now = millis();
  Serial.print("RAW: 0x");
  if (code < 0x100000) Serial.print("0");
  if (code < 0x10000) Serial.print("0");
  if (code < 0x1000) Serial.print("0");
  if (code < 0x100) Serial.print("0");
  if (code < 0x10) Serial.print("0");
  Serial.print(code, HEX);
  Serial.print(" bits=");
  Serial.print(bitlen);
  Serial.print(" dt=");
  Serial.print(now - lastTime);
  Serial.print("ms state=");
  Serial.print(state == WAIT_SYNC ? "SYNC" : state == WAIT_HEADER2 ? "HDR2" : "PAYL");

  // Filtro DEDUPE
  if (code == lastValue && (now - lastTime) < DEDUPE_MS) {
    Serial.println(" -> DEDUPE skip");
    lastTime = now;
    return;
  }
  lastValue = code;
  lastTime = now;

  // Decodificar bytes
  uint8_t b0 = (code >> 16) & 0xFF;
  uint8_t b1 = (code >> 8) & 0xFF;
  uint8_t b2 = code & 0xFF;

  Serial.print(" b=[");
  Serial.print(b0, HEX);
  Serial.print(",");
  Serial.print(b1, HEX);
  Serial.print(",");
  Serial.print(b2, HEX);
  Serial.print("]");

  // Máquina de estados
  switch (state) {
    case WAIT_SYNC:
      if (b0 == SYNC_BYTE) {
        pkt_seq = b1;
        pkt_type = b2;
        state = WAIT_HEADER2;
        Serial.print(" -> SYNC OK seq=");
        Serial.print(pkt_seq);
        Serial.print(" type=0x");
        Serial.print(pkt_type, HEX);
      } else {
        Serial.print(" -> NO SYNC");
      }
      break;

    case WAIT_HEADER2:
      if (b0 > MAX_PAYLOAD) {
        Serial.print(" -> LEN INVALIDO(");
        Serial.print(b0);
        Serial.print(">16) RESET");
        state = WAIT_SYNC;
      } else {
        pkt_len = b0;
        pkt_crc = b1;
        payload_idx = 0;
        if (pkt_len == 0) {
          // verificar CRC
          uint8_t buf[3] = {pkt_seq, pkt_type, pkt_len};
          uint8_t calc = crc8(buf, 3);
          Serial.print(" -> len=0 crc:");
          Serial.print(calc == pkt_crc ? "OK" : "FAIL");
          state = WAIT_SYNC;
        } else {
          state = WAIT_PAYLOAD;
          Serial.print(" -> len=");
          Serial.print(pkt_len);
          Serial.print(" crc=0x");
          Serial.print(pkt_crc, HEX);
        }
      }
      break;

    case WAIT_PAYLOAD:
      pkt_payload[payload_idx++] = b0;
      if (payload_idx < pkt_len) pkt_payload[payload_idx++] = b1;
      if (payload_idx < pkt_len) pkt_payload[payload_idx++] = b2;

      Serial.print(" -> payload idx=");
      Serial.print(payload_idx);
      Serial.print("/");
      Serial.print(pkt_len);

      if (payload_idx >= pkt_len) {
        // Verificar CRC
        uint8_t buf[3 + MAX_PAYLOAD];
        buf[0] = pkt_seq;
        buf[1] = pkt_type;
        buf[2] = pkt_len;
        for (uint8_t i = 0; i < pkt_len; i++) buf[3+i] = pkt_payload[i];
        uint8_t calc = crc8(buf, 3 + pkt_len);

        if (calc == pkt_crc) {
          Serial.print(" CRC OK! PKT: seq=");
          Serial.print(pkt_seq);
          Serial.print(" type=0x");
          Serial.print(pkt_type, HEX);
          Serial.print(" payload=");
          for (uint8_t i = 0; i < pkt_len; i++) Serial.print((char)pkt_payload[i]);
        } else {
          Serial.print(" CRC FAIL calc=0x");
          Serial.print(calc, HEX);
          Serial.print(" recv=0x");
          Serial.print(pkt_crc, HEX);
        }
        state = WAIT_SYNC;
      }
      break;
  }

  Serial.println();
}
