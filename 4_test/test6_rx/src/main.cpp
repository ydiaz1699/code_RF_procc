// TEST 6 - RX (Uno)
// SIN filtro de deduplicación por tiempo.
// Estrategia: la máquina de estados SOLO avanza si el código recibido
// es DIFERENTE al anterior. Si es el mismo → es una repetición → ignorar.
// Esto funciona porque dentro de una trama, los códigos son siempre diferentes
// (SYNC tiene 0xAA, HEADER tiene LEN<16, PAYLOAD tiene datos).
#include <Arduino.h>
#include <RCSwitch.h>

RCSwitch rc = RCSwitch();

// === Constantes ===
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

// === Deduplicación por VALOR (no por tiempo) ===
uint32_t lastProcessedCode = 0;
unsigned long lastNewCodeTime = 0;

// Timeout: si no llega un nuevo código diferente en 1 segundo, reset
const unsigned long FRAME_TIMEOUT_MS = 1000;

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
  Serial.println("TEST6 RX - Dedupe por VALOR (no por tiempo)");
}

void loop() {
  // Timeout de trama
  if (state != WAIT_SYNC && (millis() - lastNewCodeTime > FRAME_TIMEOUT_MS)) {
    Serial.println("[TIMEOUT - reset]");
    state = WAIT_SYNC;
    lastProcessedCode = 0;
  }

  if (!rc.available()) return;

  uint32_t code = rc.getReceivedValue();
  rc.resetAvailable();

  if (code == 0) return;

  // === FILTRO: si el código es IGUAL al último procesado, es repetición ===
  if (code == lastProcessedCode) {
    // Es una repetición de RCSwitch del mismo código. Ignorar.
    return;
  }

  // Código NUEVO (diferente al anterior) → procesar
  lastProcessedCode = code;
  lastNewCodeTime = millis();

  uint8_t b0 = (code >> 16) & 0xFF;
  uint8_t b1 = (code >> 8) & 0xFF;
  uint8_t b2 = code & 0xFF;

  // Debug
  Serial.print("NEW: 0x");
  Serial.print(code, HEX);
  Serial.print(" st=");
  Serial.print(state == WAIT_SYNC ? "SYN" : state == WAIT_HEADER2 ? "HD2" : "PAY");

  switch (state) {
    case WAIT_SYNC:
      if (b0 == SYNC_BYTE) {
        pkt_seq = b1;
        pkt_type = b2;
        state = WAIT_HEADER2;
        Serial.print(" -> SYNC s=");
        Serial.print(pkt_seq);
      } else {
        Serial.print(" -> noSYNC");
      }
      break;

    case WAIT_HEADER2:
      if (b0 > MAX_PAYLOAD) {
        state = WAIT_SYNC;
        lastProcessedCode = 0;
        Serial.print(" -> BAD LEN=");
        Serial.print(b0);
      } else {
        pkt_len = b0;
        pkt_crc = b1;
        payload_idx = 0;
        if (pkt_len == 0) {
          uint8_t buf[3] = {pkt_seq, pkt_type, pkt_len};
          uint8_t calc = crc8(buf, 3);
          Serial.print(calc == pkt_crc ? " -> PKT(empty)OK" : " -> CRC-FAIL");
          state = WAIT_SYNC;
          lastProcessedCode = 0;
        } else {
          state = WAIT_PAYLOAD;
          Serial.print(" -> HDR l=");
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
          Serial.print(" -> *** OK: ");
          for (uint8_t i = 0; i < pkt_len; i++) Serial.print((char)pkt_payload[i]);
          Serial.print(" ***");
        } else {
          Serial.print(" -> CRC-FAIL c=0x");
          Serial.print(calc, HEX);
          Serial.print("/r=0x");
          Serial.print(pkt_crc, HEX);
        }
        state = WAIT_SYNC;
        lastProcessedCode = 0;  // reset para permitir nueva trama
      } else {
        Serial.print(" -> pay ");
        Serial.print(payload_idx);
        Serial.print("/");
        Serial.print(pkt_len);
      }
      break;
  }
  Serial.println();
}
