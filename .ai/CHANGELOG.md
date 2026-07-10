# Changelog

Todos los cambios notables de este proyecto se documentan en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es/1.1.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/lang/es/).

## [Unreleased]

### Added
- Tipo `TYPE_BATTERY` para monitoreo de voltaje (pendiente implementación)

## [0.2.0] — 2025-07-09

### Fixed
- Corregido `enableReceive()` para usar `digitalPinToInterrupt(pin)` en vez de número directo
- Corregido timing de deduplicación: `DEDUPE_MS` de 50ms a 200ms (calibrado por hardware)
- Corregido `setRepeatTransmit()` de default(10) a 3 (mínimo funcional verificado)
- Ajustado `CODE_GAP_MS` a 150ms para separación fiable entre códigos de trama

### Added
- Filtro de deduplicación por tiempo en `RFProtocolRx::update()` (DEDUPE_MS=200ms)
- Timeout de trama (RX_TIMEOUT_MS=1000ms) para reset automático de FSM
- Documentación `.ai/` completa (6 archivos) siguiendo framework vsCode-AI
- Documentación `docs/` (conexiones SVG, notas HW, copilot-instructions)
- `archivo-mapa.yml` para contexto rápido de IA
- `meta-prompt-analisis-profundo.md` con metodología de debugging real
- Tests de diagnóstico incrementales (test4-test7) para aislamiento de variables

### Changed
- `CODE_GAP_MS` de 100ms a 150ms (TX) para compatibilidad con DEDUPE_MS del RX
- `REPEAT_GAP_MS` de 200ms a 300ms (TX) para separación clara entre repeticiones de trama
- `RX_TIMEOUT_MS` de 500ms a 1000ms para mayor tolerancia

### Hardware
- Verificado que `setRepeatTransmit(2)` NO funciona con módulos FS1000A/MX-RM-5V
- Verificado que `setRepeatTransmit(3)` es el mínimo funcional para este hardware
- Documentado que pin DATA del RX DEBE ser D2 (INT0) en Arduino Uno

## [0.1.0] — 2025-07-09

### Added
- Estructura inicial del proyecto (transmisor + receptor)
- Protocolo empaquetado: SYNC + SEQ + TYPE + LEN + CRC8 + PAYLOAD sobre RCSwitch 24-bit
- Clase `RFProtocolTx` con fragmentación de Packet en códigos 24-bit
- Clase `RFProtocolRx` con re-ensamblado y validación CRC
- FSM de aplicación TX: ST_SLEEP → ST_WAKE → ST_BUILD_PACKET → ST_TRANSMIT → ST_WAIT_RELEASE
- Deep-sleep PWR_DOWN con wake por interrupción externa INT0 (botón en D2)
- Deduplicación de paquetes por seq number (ventana 800ms)
- CRC-8 (polinomio 0x07) para integridad de datos

### Hardware
- Arduino Nano como transmisor con módulo TX 433MHz en D10
- Arduino Uno como receptor con módulo RX 433MHz en D2
- Pulsador NO con pull-up interno en D2 del Nano (wake interrupt)

---

## Convenciones

### Tipos de cambio

- **Added** — Funcionalidad nueva
- **Changed** — Cambios en funcionalidad existente
- **Deprecated** — Funcionalidad que será eliminada próximamente
- **Removed** — Funcionalidad eliminada
- **Fixed** — Corrección de errores
- **Security** — Correcciones de vulnerabilidades
- **Hardware** — Cambios en conexiones o componentes físicos

### Reglas

1. Cada versión tiene su propia sección con fecha en formato ISO (YYYY-MM-DD)
2. Los cambios se agrupan por tipo
3. La sección `[Unreleased]` siempre va primero
4. Los mensajes son concisos pero descriptivos
5. Se referencian issues/PRs cuando aplica: `(#123)`
