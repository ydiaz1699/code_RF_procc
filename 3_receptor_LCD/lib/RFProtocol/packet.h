#pragma once
#include <Arduino.h>

constexpr uint8_t SYNC_BYTE   = 0xAA;
constexpr uint8_t MAX_PAYLOAD = 16;

enum PacketType : uint8_t {
  TYPE_BUTTON_EVENT = 0x01,
  TYPE_TEMPERATURE  = 0x02,
  TYPE_BATTERY      = 0x03,
  TYPE_CONFIG       = 0x04,
  TYPE_ACK          = 0x05,
};

struct Packet {
  uint8_t seq;
  uint8_t type;
  uint8_t len;
  uint8_t payload[MAX_PAYLOAD];
};
