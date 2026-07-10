# code_RF_procc — Arquitectura

## Diagrama General

```
┌─────────────────────────────────────────────────────────┐
│                    TRANSMISOR (Nano)                      │
│                                                          │
│  ┌──────────┐    ┌──────────────┐    ┌───────────────┐  │
│  │  Botón   │───▶│  FSM App TX  │───▶│ RFProtocolTx  │  │
│  │  (D2)    │    │  (5 estados) │    │  sendPacket() │  │
│  └──────────┘    └──────────────┘    └───────┬───────┘  │
│                                              │           │
│                                    ┌─────────▼────────┐  │
│                                    │  RCSwitch.send() │  │
│                                    │  (D10, 24 bits)  │  │
│                                    └─────────┬────────┘  │
└──────────────────────────────────────────────┼───────────┘
                                               │ 433MHz
                                               ▼
┌──────────────────────────────────────────────┼───────────┐
│                    RECEPTOR (Uno)             │           │
│                                    ┌─────────▼────────┐  │
│                                    │ RCSwitch ISR     │  │
│                                    │ handleInterrupt()│  │
│                                    └─────────┬────────┘  │
│                                              │           │
│  ┌──────────┐    ┌──────────────┐    ┌───────▼───────┐  │
│  │  Serial  │◀───│  Dedup Seq   │◀───│ RFProtocolRx  │  │
│  │  Output  │    │  (800ms win) │    │  update()     │  │
│  └──────────┘    └──────────────┘    │  (3 estados)  │  │
│                                      └───────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## Componentes

| Componente | Tipo | Responsabilidad | Archivos | Dependencias |
|------------|------|-----------------|----------|--------------|
| FSM App TX | Máquina de estados | Gestión del ciclo sleep→wake→tx→release | `transmisor/src/main.cpp` | RFProtocolTx, avr/sleep |
| RFProtocolTx | Clase | Fragmentar Packet en códigos RCSwitch 24-bit | `transmisor/lib/RFProtocol/RFProtocol.cpp` | RCSwitch, crc8, packet |
| RFProtocolRx | Clase | Re-ensamblar códigos 24-bit en Packet válido | `receptor/lib/RFProtocol/RFProtocol.cpp` | RCSwitch, crc8, packet |
| Dedup Seq | Lógica inline | Filtrar paquetes repetidos por seq number | `receptor/src/main.cpp` | — |
| CRC8 | Utilidad inline | Validación de integridad de paquetes | `*/lib/RFProtocol/crc8.h` | — |
| Packet | Estructura | Formato de datos compartido TX/RX | `*/lib/RFProtocol/packet.h` | — |

## Máquina de Estados — Transmisor (FSM App TX)

```
                    ┌───────────┐
          ┌────────│  ST_SLEEP  │◀──────────────┐
          │        └───────────┘                │
          │ ISR: wokeByButton=true              │
          ▼                                     │
    ┌───────────┐                               │
    │  ST_WAKE  │──(botón no presionado)────────┘
    └─────┬─────┘
          │ (botón presionado, 30ms estab.)
          ▼
┌─────────────────┐
│ ST_BUILD_PACKET │  (extensión: leer sensores)
└────────┬────────┘
         │
         ▼
  ┌─────────────┐
  │ ST_TRANSMIT │  rf.sendPacket(p, 3)
  └──────┬──────┘
         │
         ▼
┌────────────────┐
│ST_WAIT_RELEASE │──(soltar botón + 250ms debounce)──▶ ST_SLEEP
└────────────────┘
```

| Estado | Descripción | Transición a | Condición | Acción |
|--------|-------------|--------------|-----------|--------|
| ST_SLEEP | CPU en PWR_DOWN, INT0 habilitada | ST_WAKE | Flanco FALLING en D2 | Rehabilita periféricos, detach ISR |
| ST_WAKE | Estabilización post-despertar | ST_BUILD_PACKET | Botón LOW tras 30ms | — |
| ST_WAKE | Despertar espurio | ST_SLEEP | Botón HIGH tras 30ms | — |
| ST_BUILD_PACKET | Preparar datos (extensible) | ST_TRANSMIT | Inmediato | — |
| ST_TRANSMIT | Enviar paquete RF | ST_WAIT_RELEASE | sendPacket() retorna | Incrementa txSeq |
| ST_WAIT_RELEASE | Esperar que suelten botón | ST_SLEEP | Botón HIGH + 250ms | Debounce |

## Máquina de Estados — Receptor (RFProtocolRx)

```
    ┌───────────────┐
    │ RX_WAIT_SYNC  │◀──────────────────────────┐
    └───────┬───────┘                            │
            │ b0 == 0xAA                         │
            ▼                                    │
   ┌────────────────┐                            │
   │ RX_WAIT_HEADER2│──(b0 > MAX_PAYLOAD)───────▶│
   └───────┬────────┘                            │
           │ len válido                          │
           ▼                                     │
  ┌─────────────────┐                            │
  │ RX_WAIT_PAYLOAD │──(payload completo)───▶ finalizePacket()
  └─────────────────┘                            │
                                                 │
        finalizePacket() ────────────────────────┘
        (CRC ok → packetReady=true)
        (CRC fail → descarta)
```

| Estado | Descripción | Transición a | Condición | Acción |
|--------|-------------|--------------|-----------|--------|
| RX_WAIT_SYNC | Espera código con 0xAA en byte alto | RX_WAIT_HEADER2 | b0 == SYNC_BYTE | Guarda seq, type |
| RX_WAIT_HEADER2 | Espera len + CRC | RX_WAIT_PAYLOAD | len ≤ MAX_PAYLOAD, len > 0 | Guarda len, crc |
| RX_WAIT_HEADER2 | Longitud inválida | RX_WAIT_SYNC | b0 > MAX_PAYLOAD | Reset |
| RX_WAIT_PAYLOAD | Acumula bytes de payload | RX_WAIT_SYNC | payloadIdx ≥ len | finalizePacket() |
| (cualquier estado) | Timeout de trama | RX_WAIT_SYNC | millis() - lastCode > 1000ms | Reset |

## Flujo de Datos

```
[Botón D2]
    │ FALLING edge (interrupt)
    ▼
[Wake from sleep]
    │
    ▼
[Build Packet]
    │ seq=N, type=0x01, payload="ON"
    ▼
[RFProtocolTx::sendPacket()]
    │ Fragmenta en 3 códigos de 24-bit:
    │   Code0: [0xAA][seq][type]
    │   Code1: [len][CRC8][0x00]
    │   Code2: [payload_b0][payload_b1][0x00]
    │
    │ Cada código: rc.send(code, 24) con setRepeatTransmit(3)
    │ delay(150ms) entre códigos
    ▼
[433MHz RF] ─── aire ───▶ [Módulo RX]
    │
    ▼
[RCSwitch ISR: handleInterrupt()]
    │ repeatCount==2 → nReceivedValue = code
    ▼
[RFProtocolRx::update()]
    │ Lee rc.available()
    │ Filtra DEDUPE (mismo valor, <200ms)
    │ Pasa a handleCode()
    ▼
[FSM RX: SYNC → HEADER → PAYLOAD → CRC]
    │
    ▼
[Packet completo, CRC válido]
    │ Filtro dedup por seq (800ms window)
    ▼
[Serial.print("Paquete recibido seq=N payload=ON")]
```
