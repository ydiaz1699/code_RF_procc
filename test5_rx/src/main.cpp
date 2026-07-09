// TEST 5 - RX (Uno)
// Mismo receptor debug pero con DEDUPE_MS=200 para cubrir
// el intervalo entre reportes con setRepeatTransmit(3).
// Con 3 reps: solo 1 reporte por código (repeatCount llega a 2 una sola vez).
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
// Con setRepeatTransmit(3): un código tarda 3×44.8ms=134ms en enviarse.
// El RX reporta una sola vez (al 2do gap, ~90ms desde inicio).
// No debería haber re-reportes, pero por seguridad:
const unsigned long DEDUPE_MS = 200;

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
  Serial.println("TEST5 RX - Debug completo, pin D2, DEDUPE=200ms");
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

  unsigned long now = millis();
  Serial.print("RAW: 0x");
  Serial.print(code, HEX);
  Serial.print(" bits=");
  Serial.print(bitlen);
  Serial.print(" dt=");
  Serial.print(now - lastTime);
  Serial.print("ms st=");
  Serial.print(state == WAIT_SYNC ? "SYN" : state == WAIT_HEADER2 ? "HD2" : "PAY");

  // Filtro DEDUPE
  if (code == lastValue && (now - lastTime) < DEDUPE_MS) {
    Serial.println(" DEDUPE");
    lastTime = now;
    return;
  }
  lastValue = code;
  lastTime = now;

  uint8_t b0 = (code >> 16) & 0xFF;
  uint8_t b1 = (code >> 8) & 0xFF;
  uint8_t b2 = code & 0xFF;

  switch (state) {
    case WAIT_SYNC:
      if (b0 == SYNC_BYTE) {
        pkt_seq = b1;
        pkt_type = b2;
        state = WAIT_HEADER2;
        Serial.print(" SYNC-OK s=");
        Serial.print(pkt_seq);
      } else {
        Serial.print(" noSYNC");
      }
      break;

    case WAIT_HEADER2:
      if (b0 > MAX_PAYLOAD) {
        state = WAIT_SYNC;
        Serial.print(" BAD-LEN=");
        Serial.print(b0);
      } else {
        pkt_len = b0;
        pkt_crc = b1;
        payload_idx = 0;
        if (pkt_len == 0) {
          uint8_t buf[3] = {pkt_seq, pkt_type, pkt_len};
          uint8_t calc = crc8(buf, 3);
          Serial.print(calc == pkt_crc ? " PKT-OK(empty)" : " CRC-FAIL");
          state = WAIT_SYNC;
        } else {
          state = WAIT_PAYLOAD;
          Serial.print(" HDR len=");
          Serial.print(pkt_len);
        }
      }
      break;

    case WAIT_PAYLOAD:
      pkt_payload[payload_idx++] = b0;
      if (payload_idx < pkt_len) pkt_payload[payload_idx++] = b1;
      if (payload_idx < pkt_len) pkt_payload[payload_idx++] = b2;

      if (payload_idx >= pkt_len) {
        uint8_t buf[3 + MAX_PAYLOAD];
        buf[0] = pkt_seq;
        buf[1] = pkt_type;
        buf[2] = pkt_len;
        for (uint8_t i = 0; i < pkt_len; i++) buf[3+i] = pkt_payload[i];
        uint8_t calc = crc8(buf, 3 + pkt_len);

        if (calc == pkt_crc) {
          Serial.print(" *** PKT OK: ");
          for (uint8_t i = 0; i < pkt_len; i++) Serial.print((char)pkt_payload[i]);
          Serial.print(" ***");
        } else {
          Serial.print(" CRC-FAIL c=0x");
          Serial.print(calc, HEX);
          Serial.print("/r=0x");
          Serial.print(pkt_crc, HEX);
        }
        state = WAIT_SYNC;
      } else {
        Serial.print(" pay ");
        Serial.print(payload_idx);
        Serial.print("/");
        Serial.print(pkt_len);
      }
      break;
  }
  Serial.println();
}
