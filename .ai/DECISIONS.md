# code_RF_procc — Decisiones Arquitectónicas

## ADR-001: Elección de MCU — Arduino Nano/Uno (ATmega328P)

**Estado:** Aceptado  
**Fecha:** 2025-07-09  
**Contexto:** Se necesita un sistema TX/RX de bajo costo para prototipado rápido de un timbre inalámbrico RF. El TX debe soportar deep-sleep para operación con batería.

### Decisión

Usar Arduino Nano como transmisor y Arduino Uno como receptor, ambos basados en ATmega328P.

### Alternativas Consideradas

| Opción | Pros | Contras |
|--------|------|---------|
| **Arduino Nano/Uno** ✅ | Bajo costo (~$6/$12), deep-sleep nativo (5µA), ecosistema maduro, 5V nativo (compatible con módulos RF) | Sin WiFi/BT, 2KB RAM, 32KB Flash |
| ESP8266 (D1 Mini) | WiFi integrado, más RAM (80KB), barato (~$4) | Sin deep-sleep real con wake por GPIO externo, 3.3V (necesita level shifter para algunos módulos RF) |
| ESP32 | WiFi+BT, dual-core, más potencia | Overkill para comunicación RF simple, consumo idle mayor, 3.3V |
| ATtiny85 | Ultra bajo consumo, más barato (~$2) | Solo 5 GPIOs, 8KB Flash, sin UART nativo (debug difícil) |

### Consecuencias

- ✅ Deep-sleep PWR_DOWN funcional con ~5µA
- ✅ 5V nativo simplifica conexión con módulos RF
- ✅ PlatformIO tiene soporte maduro para atmelavr
- ❌ Sin conectividad de red (solo RF punto-a-punto)
- ❌ RAM limitada (2KB) restringe buffer y payload

---

## ADR-002: Protocolo de Comunicación — RCSwitch 433MHz con protocolo empaquetado custom

**Estado:** Aceptado  
**Fecha:** 2025-07-09  
**Contexto:** Se requiere comunicación inalámbrica unidireccional simple entre TX y RX con integridad de datos. El medio físico son módulos RF 433MHz ASK/OOK genéricos.

### Decisión

Usar la librería RCSwitch como capa física (envío/recepción de códigos 24-bit) y construir un protocolo empaquetado propio encima: SYNC + SEQ + TYPE + LEN + CRC8 + PAYLOAD.

### Alternativas Consideradas

| Opción | Pros | Contras |
|--------|------|---------|
| **RCSwitch + protocolo custom** ✅ | Librería madura y probada, protocolo extensible, CRC para integridad, seq para dedup | Complejidad de timing entre códigos, debugging no trivial |
| RCSwitch raw (código único) | Ultra simple, funciona "out of the box" | Sin integridad, sin tipado, sin payload variable, solo 3 bytes útiles |
| RadioHead (ASK) | Protocolo con headers y CRC incluido, FIFOs | Más pesado en Flash/RAM, interfaz más compleja, menos documentación comunitaria para AVR |
| VirtualWire | Ligero, diseñado para 433MHz | Librería deprecada, sin mantenimiento, sin CRC nativo |
| NRF24L01 | Bidireccional, ACK automático, 2.4GHz, mayor throughput | Requiere módulo diferente (no 433MHz), SPI, más caro, alcance menor sin PA |

### Consecuencias

- ✅ Control total del formato de paquete (extensible a sensores, batería, config)
- ✅ CRC-8 previene datos corruptos
- ✅ Seq number permite detectar duplicados
- ✅ RCSwitch es estable y ampliamente probado
- ❌ Timing entre códigos requiere calibración (setRepeatTransmit, CODE_GAP_MS, DEDUPE_MS)
- ❌ Sin ACK: no hay garantía de entrega (mitigado con 3 repeticiones de trama)
- ❌ Throughput bajo (~1 paquete/segundo max)

---

## ADR-003: setRepeatTransmit(3) como mínimo de transmisión

**Estado:** Aceptado  
**Fecha:** 2025-07-09  
**Contexto:** RCSwitch necesita ver un código repetido para validarlo (`handleInterrupt` requiere `repeatCount==2`). Se probaron valores de 2, 3, y 10 (default) con hardware real.

### Decisión

Configurar `setRepeatTransmit(3)` en el transmisor. Es el mínimo que produce recepción fiable con los módulos 433MHz utilizados.

### Alternativas Consideradas

| Opción | Pros | Contras |
|--------|------|---------|
| setRepeatTransmit(2) | Mínima latencia (~90ms/código) | **NO FUNCIONA**: RCSwitch necesita 2 gaps entre repeticiones = mínimo 3 transmisiones |
| **setRepeatTransmit(3)** ✅ | Funciona fiablemente, latencia aceptable (~135ms/código), un solo reporte por código | Límite mínimo — puede fallar con módulos de peor calidad |
| setRepeatTransmit(10) (default) | Máxima robustez, funciona siempre | Genera múltiples reportes por código (~cada 90ms), requiere filtro DEDUPE complejo, latencia alta (~448ms/código) |
| setRepeatTransmit(5) | Buen balance | No probado, 3 ya funciona |

### Consecuencias

- ✅ Un solo reporte por código en el receptor (simplifica lógica)
- ✅ Latencia total de trama: ~855ms (aceptable para eventos de botón)
- ✅ DEDUPE_MS=200 es suficiente como safety net
- ❌ Si se cambian los módulos RF por unos de peor calidad, puede necesitar subir a 4-5
- ❌ Probado solo con FS1000A/MX-RM-5V a distancia <5m en interiores

---

## ADR-004: Deduplicación en dos niveles

**Estado:** Aceptado  
**Fecha:** 2025-07-09  
**Contexto:** RCSwitch puede reportar el mismo código múltiples veces, y el TX repite la trama completa 3 veces para robustez. Se necesitan dos filtros independientes.

### Decisión

Implementar deduplicación en dos niveles:
1. **Nivel RCSwitch (DEDUPE_MS=200ms):** Filtra re-reportes del mismo código de 24 bits
2. **Nivel Trama (DEDUPE_WINDOW_MS=800ms):** Filtra paquetes con mismo seq recibidos dentro de 800ms

### Alternativas Consideradas

| Opción | Pros | Contras |
|--------|------|---------|
| **Dos niveles (tiempo + seq)** ✅ | Robusto contra ambos tipos de repetición | Dos parámetros a calibrar |
| Solo DEDUPE por tiempo | Simple | No distingue entre repetición de código y repetición de trama |
| Solo DEDUPE por valor (code == lastCode) | Sin dependencia de timing | Falla si dos códigos consecutivos de DIFERENTES tramas son iguales |
| Sin deduplicación | Más simple | Múltiples eventos falsos por pulsación |

### Consecuencias

- ✅ Una pulsación = exactamente un evento procesado
- ✅ Los 3 PACKET_REPEATS proporcionan redundancia sin generar duplicados
- ❌ Si el usuario presiona >1 vez en <800ms, el segundo evento se pierde
- ❌ Los parámetros (200ms, 800ms) están calibrados para este hardware específico

---

## Plantilla para Nuevas Decisiones

```markdown
## ADR-XXX: [Título]

**Estado:** Propuesto | Aceptado | Deprecado | Sustituido por ADR-XXX
**Fecha:** YYYY-MM-DD
**Contexto:** [Situación que requiere la decisión]

### Decisión
[Qué se decidió]

### Alternativas Consideradas
| Opción | Pros | Contras |
|--------|------|---------|
| ... | ... | ... |

### Consecuencias
- ✅ ...
- ❌ ...
```
