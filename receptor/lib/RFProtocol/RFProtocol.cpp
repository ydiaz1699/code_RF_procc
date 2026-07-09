#include "RFProtocol.h"
#include "crc8.h"

RFProtocolRx::RFProtocolRx(uint8_t interruptPin) : _interruptPin(interruptPin) {}

void RFProtocolRx::begin() {
  _rc.enableReceive(_interruptPin);
}

bool RFProtocolRx::available() const {
  return _packetReady;
}

Packet RFProtocolRx::read() {
  _packetReady = false;
  return _readyPacket;
}

void RFProtocolRx::update() {
  // Si quedamos a medias de una trama y pasó demasiado tiempo sin el
  // siguiente código, se descarta y se vuelve a esperar SYNC.
  if (_state != RX_WAIT_SYNC && (millis() - _lastCodeMs > RX_TIMEOUT_MS)) {
    _state = RX_WAIT_SYNC;
    _lastValue = 0;
  }

  if (!_rc.available()) return;

  uint32_t code = _rc.getReceivedValue();
  _rc.resetAvailable();

  if (code == 0) return;  // RCSwitch a veces reporta 0 con ruido/timeout, se ignora

  // ─── Filtro de repeticiones de RCSwitch ───
  // RCSwitch retransmite cada código varias veces (nRepeatTransmit, default=10).
  // El receptor las entrega todas como si fueran códigos nuevos.
  // Aquí descartamos repeticiones del MISMO código que llegan en ráfaga.
  unsigned long now = millis();
  if (code == _lastValue && (now - _lastCodeMs) < DEDUPE_MS) {
    // Es una repetición del mismo código dentro de la ventana → ignorar
    _lastCodeMs = now;  // actualizar timestamp para extender la ventana
    return;
  }
  _lastValue = code;
  _lastCodeMs = now;

  handleCode(code);
}

void RFProtocolRx::handleCode(uint32_t code) {
  uint8_t b0 = (code >> 16) & 0xFF;
  uint8_t b1 = (code >> 8) & 0xFF;
  uint8_t b2 = code & 0xFF;

  switch (_state) {

    case RX_WAIT_SYNC:
      if (b0 == SYNC_BYTE) {
        _building.seq  = b1;
        _building.type = b2;
        _state = RX_WAIT_HEADER2;
      }
      // si no es SYNC, se ignora el código (ruido o quedamos desincronizados)
      break;

    case RX_WAIT_HEADER2:
      if (b0 > MAX_PAYLOAD) {
        // longitud inválida: probablemente ruido, se descarta la trama
        _state = RX_WAIT_SYNC;
        break;
      }
      _building.len = b0;
      _recvCrc = b1;
      _payloadIdx = 0;

      if (_building.len == 0) {
        finalizePacket();
      } else {
        _state = RX_WAIT_PAYLOAD;
      }
      break;

    case RX_WAIT_PAYLOAD:
      _building.payload[_payloadIdx++] = b0;
      if (_payloadIdx < _building.len) _building.payload[_payloadIdx++] = b1;
      if (_payloadIdx < _building.len) _building.payload[_payloadIdx++] = b2;

      if (_payloadIdx >= _building.len) {
        finalizePacket();
      }
      break;
  }
}

void RFProtocolRx::finalizePacket() {
  uint8_t crc = packetChecksum(_building);
  if (crc == _recvCrc) {
    _readyPacket = _building;
    _packetReady = true;
  }
  // si el CRC no coincide, se descarta silenciosamente y se vuelve a esperar SYNC
  _state = RX_WAIT_SYNC;
}
