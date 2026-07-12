# Copilot Instructions

## Proyecto
Sistema RF 433MHz punto-a-punto: Arduino Nano (TX, deep-sleep) → Arduino Uno (RX). Framework: Arduino sobre PlatformIO. Plataforma: atmelavr (ATmega328P).

## Reglas de Código
- Framework: Arduino (PlatformIO, NO Arduino IDE)
- MCU: ATmega328P (2KB RAM, 32KB Flash) — optimizar memoria siempre
- Idioma: comentarios en español, identificadores en inglés
- Estilo: camelCase variables, PascalCase clases, SCREAMING_CASE constantes
- Indentación: 2 espacios
- Usar `constexpr` en vez de `#define` para constantes tipadas
- Usar `millis()` para timing, NUNCA `delay()` en receptor (bloquea polling)
- En TX, `delay()` es aceptable (durante transmisión, nada más puede ocurrir)

## Anti-patrones (NO hacer)
- NO usar `enableReceive(2)` — usar `enableReceive(digitalPinToInterrupt(2))`
- NO usar `setRepeatTransmit(2)` — mínimo funcional es 3
- NO usar `delay()` en el loop del receptor — impide detectar códigos RF
- NO confundir número de pin con número de interrupción
- NO asumir que RCSwitch reporta un código una sola vez — puede re-reportar
- NO usar `String` (fragmenta heap en AVR) — usar char arrays o F() macro
- NO olvidar `resetAvailable()` después de leer un código RCSwitch

## Contexto
Lee estos archivos antes de sugerir código:
- `.ai/PROJECT_CONTEXT.md` — convenciones y estructura
- `.ai/PROTOCOL.md` — formato de paquetes y timing RF
- `.ai/HARDWARE.md` — pines y conexiones

## Patrones Preferidos
- Máquinas de estado con `enum` + `switch` (no if/else encadenados)
- Timers no bloqueantes: `if (millis() - lastMs > INTERVAL)`
- Structs para datos empaquetados (no variables sueltas)
- Funciones inline para cálculos pequeños (`crc8`, checksums)
- `Serial.flush()` antes de `sleep_cpu()` (asegurar que TX serial termine)

## Compilación
```bash
cd transmisor && pio run    # Compilar TX
cd receptor && pio run      # Compilar RX
```
