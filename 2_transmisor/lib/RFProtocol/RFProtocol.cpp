#include "RFProtocol.h"
#include "crc8.h"

namespace {
  const uint16_t REPEAT_GAP_MS = 300;  // separación entre repeticiones de trama completa
  const uint16_t CODE_GAP_MS   = 150;  // separación entre códigos DENTRO de una trama.
                                       // Con setRepeatTransmit(3): rc.send() tarda ~135ms,
                                       // + 150ms de gap = ~285ms entre códigos.
                                       // Esto coincide con el dt=286-287ms medido en Test 5.
}

RFProtocolTx::RFProtocolTx(uint8_t txPin) : _txPin(txPin) {}

void RFProtocolTx::begin() {
  _rc.enableTransmit(_txPin);
  // 3 repeticiones: mínimo probado para recepción fiable con módulos 433MHz.
  // Con 2 reps el RX no valida (RCSwitch necesita 2 gaps = 3 transmisiones).
  // Con 10 (default) genera múltiples reportes que complican la decodificación.
  _rc.setRepeatTransmit(3);
}

void RFProtocolTx::sendCode24(uint8_t b0, uint8_t b1, uint8_t b2) {
  uint32_t code = ((uint32_t)b0 << 16) | ((uint32_t)b1 << 8) | (uint32_t)b2;
  _rc.send(code, 24);
  delay(CODE_GAP_MS);
}

void RFProtocolTx::sendFrame(const Packet &p) {
  uint8_t crc = packetChecksum(p);

  // Código 0: [SYNC=0xAA][SEQ][TYPE]
  sendCode24(SYNC_BYTE, p.seq, p.type);

  // Código 1: [LEN][CRC8][0x00]
  sendCode24(p.len, crc, 0x00);

  // Códigos 2..N: payload empaquetado 3 bytes por código
  uint8_t i = 0;
  while (i < p.len) {
    uint8_t b0 = p.payload[i++];
    uint8_t b1 = (i < p.len) ? p.payload[i++] : 0x00;
    uint8_t b2 = (i < p.len) ? p.payload[i++] : 0x00;
    sendCode24(b0, b1, b2);
  }
}

void RFProtocolTx::sendPacket(const Packet &p, uint8_t repeats) {
  for (uint8_t r = 0; r < repeats; r++) {
    sendFrame(p);
    if (r < repeats - 1) delay(REPEAT_GAP_MS);
  }
}
