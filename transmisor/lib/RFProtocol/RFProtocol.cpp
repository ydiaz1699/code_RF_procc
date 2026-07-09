#include "RFProtocol.h"
#include "crc8.h"

namespace {
  const uint16_t REPEAT_GAP_MS = 300;  // separación entre repeticiones de trama completa
  const uint16_t CODE_GAP_MS   = 80;   // separación entre códigos DENTRO de una trama
                                       // Solo necesita ser lo suficiente para que el RX
                                       // procese el código anterior antes del siguiente.
                                       // Con setRepeatTransmit(2), cada rc.send() tarda ~90ms.
                                       // 80ms adicionales dan margen suficiente.
}

RFProtocolTx::RFProtocolTx(uint8_t txPin) : _txPin(txPin) {}

void RFProtocolTx::begin() {
  _rc.enableTransmit(_txPin);
  // CRÍTICO: Reducir repeticiones de RCSwitch al mínimo (2).
  // RCSwitch necesita al menos 2 repeticiones para que el receptor valide
  // (handleInterrupt requiere repeatCount==2). Con el default de 10,
  // el receptor reporta el MISMO código cada ~90ms, lo que confunde
  // nuestra máquina de estados del protocolo.
  _rc.setRepeatTransmit(2);
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
