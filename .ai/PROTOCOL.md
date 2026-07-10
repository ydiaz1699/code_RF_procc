# code_RF_procc — Protocolo de Comunicación

## Información General

| Campo | Valor |
|-------|-------|
| Tipo | RF 433MHz ASK/OOK unidireccional |
| Capa física | RCSwitch protocolo 1 (pulseLength=350µs) |
| Capa de enlace | Protocolo empaquetado custom sobre códigos 24-bit |
| Dirección | TX → RX (simplex, sin ACK) |
| Alcance | 30-100m con antena 17cm |
| Integridad | CRC-8 polinomio 0x07 |
| Repeticiones | 3× por trama completa (a nivel aplicación) |

## Estructura del Paquete (Packet)

```c
struct Packet {
  uint8_t seq;                  // Número de secuencia (0-255, wraps)
  uint8_t type;                 // Tipo de paquete (PacketType enum)
  uint8_t len;                  // Longitud del payload (0-16)
  uint8_t payload[MAX_PAYLOAD]; // Datos (máx 16 bytes)
};
```

## Tipos de Paquete

| Código | Nombre | Descripción | Payload |
|--------|--------|-------------|---------|
| 0x01 | TYPE_BUTTON_EVENT | Evento de pulsador | ASCII: "ON" (2 bytes) |
| 0x02 | TYPE_TEMPERATURE | Lectura de temperatura | Reservado |
| 0x03 | TYPE_BATTERY | Nivel de batería | Reservado |
| 0x04 | TYPE_CONFIG | Configuración remota | Reservado |
| 0x05 | TYPE_ACK | Acknowledgment | Reservado |

## Formato de Transmisión RF

Cada Packet se fragmenta en códigos RCSwitch de 24 bits (3 bytes por código):

### Estructura de Trama

```
┌──────────────────────────────────────────────────────────────┐
│                     TRAMA COMPLETA                             │
├────────────────┬─────────────────┬───────────────────────────┤
│   Código 0     │    Código 1     │   Código 2..N (payload)   │
│  SYNC+SEQ+TYPE │  LEN+CRC+PAD   │   3 bytes por código      │
├────────────────┼─────────────────┼───────────────────────────┤
│ [0xAA][S][T]   │ [L][CRC][0x00]  │ [P0][P1][P2]...          │
└────────────────┴─────────────────┴───────────────────────────┘
```

### Detalle de Cada Código

| Código | Byte 0 (MSB) | Byte 1 | Byte 2 (LSB) | Descripción |
|--------|:---:|:---:|:---:|-------------|
| 0 | `0xAA` (SYNC) | `seq` | `type` | Sincronización + identificación |
| 1 | `len` | `CRC8` | `0x00` | Longitud + integridad + padding |
| 2..N | `payload[i]` | `payload[i+1]` | `payload[i+2]` | Datos empaquetados 3 por código |

### CRC-8

- **Polinomio:** 0x07 (x⁸ + x² + x + 1)
- **Valor inicial:** 0x00
- **Datos cubiertos:** `[seq, type, len, payload[0..len-1]]`
- **NO cubre:** SYNC byte ni padding

```c
uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
    }
  }
  return crc;
}
```

## Timing de Transmisión

| Parámetro | Valor | Fuente |
|-----------|-------|--------|
| `setRepeatTransmit` | 3 | Mínimo para que RCSwitch valide en RX |
| Tiempo por código (rc.send) | ~135ms | 3 reps × 44.8ms/rep |
| `CODE_GAP_MS` (delay entre códigos) | 150ms | Separación entre códigos de la trama |
| dt medido entre códigos en RX | ~286ms | 135ms send + 150ms gap |
| `REPEAT_GAP_MS` (delay entre tramas) | 300ms | Separación entre repeticiones completas |
| Tiempo total por trama (3 códigos) | ~855ms | 3×135ms + 2×150ms |
| Tiempo total con 3 repeticiones | ~3.2s | 3×855ms + 2×300ms |

## Secuencia de Comunicación

```
TRANSMISOR (Nano)                                    RECEPTOR (Uno)
      │                                                    │
      │  rc.send(0xAA0101, 24) ─── 3 reps ──────────────▶ │ handleInterrupt()
      │  [SYNC=0xAA, seq=1, type=0x01]                     │ repeatCount==2 → available
      │                                                    │ update(): SYNC OK, state=HEADER2
      │  ◄── delay(150ms) ──▶                              │
      │                                                    │
      │  rc.send(0x02D700, 24) ─── 3 reps ──────────────▶ │ update(): len=2, crc=0xD7
      │  [LEN=2, CRC=0xD7, PAD=0x00]                      │ state=PAYLOAD
      │                                                    │
      │  ◄── delay(150ms) ──▶                              │
      │                                                    │
      │  rc.send(0x4F4E00, 24) ─── 3 reps ──────────────▶ │ update(): payload="ON"
      │  [P0='O', P1='N', PAD=0x00]                        │ CRC OK → packetReady!
      │                                                    │
      │  ◄── delay(300ms) ──▶                              │ Dedup: seq=1, print
      │                                                    │
      │  [REPEAT: misma trama]                             │ Dedup: seq==lastSeq → ignore
      │                                                    │
```

## Deduplicación (Receptor)

### Nivel 1: Filtro DEDUPE en RFProtocolRx (repeticiones de RCSwitch)

| Parámetro | Valor | Función |
|-----------|-------|---------|
| `DEDUPE_MS` | 200ms | Si mismo código llega en <200ms, es repetición de RCSwitch |
| Mecanismo | Comparar `code == _lastValue` AND `dt < 200ms` | Descarta re-reportes |

### Nivel 2: Filtro por Seq en main.cpp (repeticiones de trama)

| Parámetro | Valor | Función |
|-----------|-------|---------|
| `DEDUPE_WINDOW_MS` | 800ms | Si mismo seq llega en <800ms, es repeat de trama |
| Mecanismo | Comparar `p.seq == lastSeq` AND `dt < 800ms` | Descarta tramas repetidas |

## Ejemplo Completo

### Paquete: Evento de botón (seq=5)

```
Packet {
  seq  = 5       (0x05)
  type = 0x01    (BUTTON_EVENT)
  len  = 2
  payload = ['O', 'N'] = [0x4F, 0x4E]
}

CRC input = [0x05, 0x01, 0x02, 0x4F, 0x4E]
CRC result = 0x96

Códigos generados:
  Code 0: (0xAA << 16) | (0x05 << 8) | 0x01 = 0xAA0501
  Code 1: (0x02 << 16) | (0x96 << 8) | 0x00 = 0x029600
  Code 2: (0x4F << 16) | (0x4E << 8) | 0x00 = 0x4F4E00
```

## Limitaciones Conocidas

| Limitación | Impacto | Mitigación |
|------------|---------|------------|
| Sin ACK | TX no sabe si RX recibió | 3 repeticiones de trama |
| Unidireccional | RX no puede pedir retransmisión | Redundancia (3 repeats) |
| Payload máx 16 bytes | No apto para datos grandes | Suficiente para eventos y sensores |
| Sin cifrado | Cualquiera con RX 433MHz puede leer | Aceptable para uso doméstico |
| Colisión RF | Si dos TX transmiten a la vez, se pierden ambos | Un solo TX en este diseño |
| Seq wrap (0-255) | Posible false-dedup tras 256 eventos en <800ms | Improbable con botón humano |
