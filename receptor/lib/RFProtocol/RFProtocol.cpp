#include "RFProtocol.h"
#include "crc8.h"

RFProtocolRx::RFProtocolRx(uint8_t interruptPin) : _interruptPin(interruptPin) {}

void RFProtocolRx::begin() {
  // enableReceive() espera el NÚMERO DE INTERRUPCIÓN, no el pin.
  // digitalPinToInterrupt() hace la conversión correcta en cualquier placa.
  _rc.enableReceive(digitalPinToInterrupt(_interruptPin));
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

  if (code == 0) return;

  // ─── Filtro de repeticiones de RCSwitch ───
  // Con setRepeatTransmit(2) en el TX, RCSwitch reporta cada código
  // exactamente UNA vez (necesita 2 repeticiones para validar = 1 reporte).
  // Pero por seguridad, si llega el MISMO código dentro de la ventana
  // de deduplicación, lo descartamos.
  //
  // Timing con protocolo 1 (pulseLength=350µs):
  //   Un código completo = 24bits × 1400µs + sync 11200µs = 44.8ms
  //   Con setRepeatTransmit(2): rc.send() tarda 2 × 44.8ms = ~90ms
  //   Entre reportes del MISMO código: ~90ms (si hubiera más de 2 reps)
  //
  // DEDUPE_MS=120ms: filtra cualquier re-reporte del mismo código,
  // pero NO filtra el siguiente código de la trama (que llega ~170ms
  // después = 90ms de rc.send() + 80ms de CODE_GAP_MS).
  unsigned long now = millis();
  if (code == _lastValue && (now - _lastCodeMs) < DEDUPE_MS) {
    _lastCodeMs = now;
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
      break;

    case RX_WAIT_HEADER2:
      if (b0 > MAX_PAYLOAD) {
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
  _state = RX_WAIT_SYNC;
}
