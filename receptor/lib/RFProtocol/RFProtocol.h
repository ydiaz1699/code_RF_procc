#pragma once
#include <Arduino.h>
#include <RCSwitch.h>
#include "packet.h"

// Reconstruye Packets a partir de los códigos RCSwitch de 24 bits
// empaquetados por RFProtocolTx (ver transmisor/lib/RFProtocol).
class RFProtocolRx {
public:
  explicit RFProtocolRx(uint8_t interruptPin);
  void begin();

  // Debe llamarse en cada loop(). Procesa un código pendiente de RCSwitch
  // (si lo hay) y avanza la máquina de estados de reensamblado.
  void update();

  // true si hay un paquete completo y con CRC válido esperando ser leído.
  bool available() const;

  // Consume el paquete listo (limpia el flag de available()).
  Packet read();

private:
  uint8_t _interruptPin;
  RCSwitch _rc;

  enum RxState { RX_WAIT_SYNC, RX_WAIT_HEADER2, RX_WAIT_PAYLOAD };
  RxState _state = RX_WAIT_SYNC;

  Packet _building{};
  uint8_t _recvCrc = 0;
  uint8_t _payloadIdx = 0;

  unsigned long _lastCodeMs = 0;
  static const unsigned long RX_TIMEOUT_MS = 500;  // si se corta una trama a medias, se descarta

  // ─── Filtro de repeticiones RCSwitch ───
  uint32_t _lastValue = 0;
  // Ventana de deduplicación para repeticiones del mismo código.
  // RCSwitch con nRepeatTransmit=10 y protocolo 1 envía cada código en ~30ms total,
  // pero entre repeticiones pasan solo ~2-3ms. Usamos 50ms como margen seguro.
  static const unsigned long DEDUPE_MS = 50;

  bool _packetReady = false;
  Packet _readyPacket{};

  void handleCode(uint32_t code);
  void finalizePacket();
};
