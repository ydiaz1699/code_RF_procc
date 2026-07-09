# code_RF_procc

Protocolo RF empaquetado sobre RCSwitch (433MHz) para Arduino Nano (TX) y Arduino Uno (RX).

## Estructura

```
├── transmisor/     # Arduino Nano - TX con sleep + botón
│   └── lib/RFProtocol/
└── receptor/       # Arduino Uno  - RX con decodificación
    └── lib/RFProtocol/
```

## Hardware
- **TX:** Arduino Nano + módulo 433MHz TX en pin D10, botón en D2
- **RX:** Arduino Uno + módulo 433MHz RX en pin D2 (interrupción 0)

## Protocolo
Cada paquete se fragmenta en códigos RCSwitch de 24 bits:
1. `[SYNC=0xAA][SEQ][TYPE]`
2. `[LEN][CRC8][0x00]`
3. `[payload bytes empaquetados 3 por código]`

## Fix aplicado: Deduplicación de repeticiones RCSwitch

**Problema:** RCSwitch retransmite cada código ~10 veces (nRepeatTransmit).
El receptor entregaba cada repetición como un código nuevo, desincronizando
la máquina de estados del protocolo.

**Solución:**
- RX: Filtro de deduplicación (DEDUPE_MS = 50ms) - ignora repeticiones del mismo valor
- TX: CODE_GAP_MS incrementado a 150ms (> DEDUPE_MS) para que el receptor
  distinga entre repeticiones y el siguiente código real de la trama
