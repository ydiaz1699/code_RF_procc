# code_RF_procc — Software

## platformio.ini

### Transmisor (Nano)

```ini
[env:nanoatmega328]
platform = atmelavr
board = nanoatmega328
framework = arduino
monitor_speed = 9600
lib_deps =
    sui77/rc-switch
```

### Receptor (Uno)

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
monitor_speed = 9600
lib_deps =
    sui77/rc-switch
```

## Librerías

| Librería | Versión | Propósito | Fuente | Ubicación |
|----------|---------|-----------|--------|-----------|
| sui77/rc-switch | latest (2.6.4+) | Envío/recepción de códigos 433MHz con modulación ASK/OOK | PlatformIO Registry | `lib_deps` |
| RFProtocol | local | Protocolo empaquetado: fragmentación TX y re-ensamblado RX sobre RCSwitch | Propia | `lib/RFProtocol/` |
| avr/sleep.h | AVR core | Control de modos de bajo consumo del ATmega328P | AVR-libc (incluido en framework) | Sistema |
| avr/power.h | AVR core | Habilitación/deshabilitación selectiva de periféricos MCU | AVR-libc (incluido en framework) | Sistema |
| avr/interrupt.h | AVR core | Manejo de interrupciones globales (sei/cli) | AVR-libc (incluido en framework) | Sistema |

## Librería Local: RFProtocol

| Archivo | Función | Usado en |
|---------|---------|----------|
| `packet.h` | Estructura `Packet`, constantes `SYNC_BYTE`, `MAX_PAYLOAD`, enum `PacketType` | TX y RX |
| `crc8.h` | Función inline `crc8()` y `packetChecksum()` | TX y RX |
| `RFProtocol.h` | Declaración de `RFProtocolTx` o `RFProtocolRx` | TX o RX |
| `RFProtocol.cpp` | Implementación de la clase (TX: fragmentación, RX: re-ensamblado) | TX o RX |

> **Nota:** La librería está duplicada en `transmisor/lib/RFProtocol/` y `receptor/lib/RFProtocol/`. Los archivos `packet.h` y `crc8.h` son idénticos. Los archivos `RFProtocol.h/.cpp` difieren (TX tiene `RFProtocolTx`, RX tiene `RFProtocolRx`).

## Dependencias del Sistema

| Herramienta | Versión Mínima | Propósito |
|-------------|----------------|-----------|
| PlatformIO Core | 6.0+ | Compilación, upload, gestión de librerías |
| Python | 3.8+ | Requerido por PlatformIO |
| Driver CH340/CH341 | — | Comunicación USB con Arduino Nano clones |
| Driver ATmega16U2 | — | Comunicación USB con Arduino Uno original |

## Comandos

### Compilación y Upload

```bash
# Compilar transmisor
cd transmisor && pio run

# Compilar receptor
cd receptor && pio run

# Upload transmisor (ajustar COM port)
cd transmisor && pio run --target upload --upload-port COM12

# Upload receptor (ajustar COM port)
cd receptor && pio run --target upload --upload-port COM10
```

### Monitor Serial

```bash
# Monitor transmisor
cd transmisor && pio device monitor --port COM12 --baud 9600

# Monitor receptor
cd receptor && pio device monitor --port COM10 --baud 9600
```

### Limpieza

```bash
cd transmisor && pio run --target clean
cd receptor && pio run --target clean
```

### Listar puertos disponibles

```bash
pio device list
```

## Configuración Específica de RCSwitch

| Parámetro | Valor en TX | Valor en RX | Notas |
|-----------|-------------|-------------|-------|
| `setRepeatTransmit()` | 3 | — | Mínimo funcional para hardware 433MHz |
| `enableTransmit()` | pin 10 | — | GPIO output |
| `enableReceive()` | — | `digitalPinToInterrupt(2)` = 0 | INT0 en Uno |
| Protocolo | 1 (default) | Auto-detecta | pulseLength=350µs |
| Bit length | 24 | Auto-detecta | 3 bytes por código |

## Notas de Compilación

- La plataforma `atmelavr` se descarga automáticamente la primera vez
- `sui77/rc-switch` se resuelve desde el PlatformIO Registry
- No se requieren build flags adicionales
- La librería local `RFProtocol/` se detecta automáticamente por PlatformIO (carpeta `lib/`)
- Flash utilizado: ~8KB de 32KB (Nano/Uno) — margen amplio para extensiones
