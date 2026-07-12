#pragma once
#include <Arduino.h>
#include <RCSwitch.h>
#include "packet.h"

class RFProtocolTx {
public:
  explicit RFProtocolTx(uint8_t txPin);
  void begin();

  // Envía el paquete como códigos RCSwitch de 24 bits empaquetados.
  // repeats: cuántas veces se repite toda la trama (por defecto 3).
  void sendPacket(const Packet &p, uint8_t repeats = 3);

private:
  uint8_t _txPin;
  RCSwitch _rc;

  // Empaqueta 3 bytes en un código de 24 bits y lo envía por RCSwitch
  void sendCode24(uint8_t b0, uint8_t b1, uint8_t b2);

  // Envía una trama completa (sin repetir)
  void sendFrame(const Packet &p);
};
