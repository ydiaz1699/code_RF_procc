# code_RF_procc — Roadmap

> Última actualización: 2025-07-09

## 🟢 Corto Plazo (1–2 semanas)

Tareas inmediatas para tener un sistema confiable:

| # | Tarea | Prioridad | Dependencia | Estado |
|---|-------|-----------|-------------|--------|
| 1 | Validar protocolo completo con hardware corregido (pin D2) | 🔴 Alta | — | En progreso |
| 2 | Probar alcance con antena 17cm a 5m, 10m, 30m | 🔴 Alta | #1 | Pendiente |
| 3 | Agregar TYPE_BATTERY para monitoreo de voltaje (ADC A0) | 🟡 Media | #1 | Pendiente |
| 4 | Implementar filtro anti-bounce mejorado en wake del Nano | 🟡 Media | — | Pendiente |
| 5 | Limpiar carpetas test4-7 del repo | 🟢 Baja | — | Pendiente |

## 🟡 Medio Plazo (1–2 meses)

Mejoras y funcionalidades adicionales:

| # | Tarea | Prioridad | Dependencia | Notas |
|---|-------|-----------|-------------|-------|
| 1 | Agregar múltiples sensores al TX (temperatura, humedad) | 🟡 Media | Corto #3 | Requiere más tipos de paquete |
| 2 | Implementar LED/buzzer de notificación en RX | 🟡 Media | Corto #1 | Para uso como timbre inalámbrico |
| 3 | Migrar a ESP32 (RX) para agregar WiFi/notificaciones push | 🟢 Baja | — | Cambiaría la placa RX |
| 4 | Agregar modo de configuración por serial (cambiar ID, canal) | 🟢 Baja | — | TYPE_CONFIG ya reservado |

## 🔴 Largo Plazo (3+ meses)

Visión futura y escalabilidad:

| # | Tarea | Categoría | Notas |
|---|-------|-----------|-------|
| 1 | Soporte multi-transmisor (varios TX con IDs únicos → 1 RX) | Escalabilidad | Requiere campo device_id en header |
| 2 | Bidireccionalidad (ACK del RX al TX) | Fiabilidad | Requiere módulo TX+RX en ambos nodos |
| 3 | Cifrado de payload (AES-128 o XOR con key compartida) | Seguridad | Para evitar replay attacks |

## 🚧 Bloqueantes

Impedimentos actuales que bloquean el progreso:

| # | Bloqueante | Impacto | Acción Requerida | Responsable |
|---|-----------|---------|------------------|-------------|
| 1 | Pin de hardware del módulo RX estaba mal conectado | Bloquea validación del protocolo final | Reconectar DATA del módulo RX a pin D2 del Uno | @ydiaz1699 |

---

## Leyenda

- 🔴 **Alta** — Bloquea otras tareas o es crítica para el MVP
- 🟡 **Media** — Importante pero no bloquea el progreso
- 🟢 **Baja** — Nice-to-have, se hace si hay tiempo
- Estados: `Pendiente` | `En progreso` | `Completado` | `Bloqueado`
