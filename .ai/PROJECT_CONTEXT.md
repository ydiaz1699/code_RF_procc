# code_RF_procc — Contexto del Proyecto

## Propósito

Sistema de comunicación RF punto-a-punto a 433MHz entre un Arduino Nano (transmisor) y un Arduino Uno (receptor). Transmite eventos de botón con protocolo empaquetado sobre RCSwitch, incluyendo secuencia, tipado, CRC8 y deep-sleep para bajo consumo.

## Referencias al Catálogo

| Tipo | Referencia | Notas |
|------|-----------|-------|
| Placa TX | `arduino-nano` (ATmega328P) | Transmisor con deep-sleep PWR_DOWN |
| Placa RX | `arduino-uno` (ATmega328P) | Receptor siempre encendido |
| Periférico TX | Módulo 433MHz TX (FS1000A) | Pin DATA en D10 |
| Periférico RX | Módulo 433MHz RX (MX-RM-5V) | Pin DATA en D2 (INT0) |
| Periférico | Pulsador NO | Pin D2 del Nano (wake interrupt) |

## Archivos Clave

| Archivo | Propósito | Notas |
|---------|-----------|-------|
| `transmisor/src/main.cpp` | App TX: FSM sleep→wake→build→transmit→release | Usa avr/sleep.h |
| `receptor/src/main.cpp` | App RX: loop con rf.update() y dedup por seq | Imprime paquetes por Serial |
| `transmisor/lib/RFProtocol/RFProtocol.cpp` | Clase RFProtocolTx: fragmenta Packet en códigos 24-bit | setRepeatTransmit(3) |
| `receptor/lib/RFProtocol/RFProtocol.cpp` | Clase RFProtocolRx: re-ensambla códigos en Packet | DEDUPE_MS=200, FSM de 3 estados |
| `*/lib/RFProtocol/packet.h` | Estructura Packet + PacketType enum | Compartido TX/RX |
| `*/lib/RFProtocol/crc8.h` | CRC-8 polinomio 0x07 | Inline, compartido TX/RX |

## Convenciones

- **Idioma del código:** Español en comentarios, inglés en identificadores
- **Nomenclatura:** camelCase para variables locales, PascalCase para clases/enums, SCREAMING_CASE para constantes
- **Indentación:** 2 espacios
- **Prefijos de pines:** `TX_PIN`, `BUTTON_PIN`, `RX_INTERRUPT_PIN`
- **Estructura de lib:** Librería compartida `RFProtocol/` duplicada en ambos proyectos (TX y RX)

## Cómo Compilar

```bash
# Transmisor (Nano)
cd transmisor
pio run --environment nanoatmega328
pio run --target upload --upload-port COM12
pio device monitor --port COM12 --baud 9600

# Receptor (Uno)
cd receptor
pio run --environment uno
pio run --target upload --upload-port COM10
pio device monitor --port COM10 --baud 9600
```

## Notas Adicionales

- El proyecto requiere ambas placas conectadas simultáneamente para probar
- Los módulos RF 433MHz necesitan antena de 17cm para alcance óptimo
- El TX usa deep-sleep PWR_DOWN y despierta por interrupción externa (INT0 = D2)
- Verificado con tests incrementales (test4-test7) que `setRepeatTransmit(3)` es el mínimo funcional
