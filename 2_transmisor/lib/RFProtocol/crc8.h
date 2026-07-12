#pragma once
#include <Arduino.h>
#include "packet.h"

inline uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

inline uint8_t packetChecksum(const Packet &p) {
  uint8_t buf[3 + MAX_PAYLOAD];
  buf[0] = p.seq;
  buf[1] = p.type;
  buf[2] = p.len;
  memcpy(buf + 3, p.payload, p.len);
  return crc8(buf, 3 + p.len);
}
