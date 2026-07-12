# code_RF_procc — Tareas

> Última actualización: 2025-07-09

## TODO

- [ ] 🟡 Agregar tipo `TYPE_BATTERY` (0x03) para monitoreo de voltaje de batería del Nano
- [ ] 🟡 Implementar filtro anti-bounce mejorado en wake (3 lecturas consecutivas LOW)
- [ ] 🟢 Agregar LED indicador en el receptor cuando recibe paquete válido
- [ ] 🟢 Documentar alcance real medido con antena 17cm en diferentes escenarios
- [ ] 🟢 Limpiar carpetas test4-7 del repo (mover a branch `tests/diagnostics` o eliminar)

## FIXME

- [ ] 🐛 En Test 6 (dedupe por valor), el primer código de la segunda trama genera un "noSYNC" espurio porque `lastProcessedCode` retiene el último código de la trama anterior (`0x4F4E00`). Impacto: cosmético, no afecta funcionalidad (la FSM ya está en WAIT_SYNC).
- [ ] 🐛 Si el payload contiene exactamente los mismos bytes que otro código de la trama (ej: payload `[0xAA, 0x01, 0x01]` = mismo valor que SYNC code), el filtro DEDUPE por valor lo descartaría. Impacto: edge case poco probable con payloads reales.

## IN PROGRESS

- [ ] 🔄 Validar protocolo completo con el código final (`transmisor/` + `receptor/`) después de corregir pin de hardware — Asignado: @ydiaz1699

## DONE

- [x] ✅ Identificar causa raíz de "no recibe datos" — Completado: 2025-07-09
- [x] ✅ Implementar filtro DEDUPE_MS en RFProtocolRx — Completado: 2025-07-09
- [x] ✅ Calibrar setRepeatTransmit(3) como mínimo funcional — Completado: 2025-07-09
- [x] ✅ Corregir enableReceive() con digitalPinToInterrupt() — Completado: 2025-07-09
- [x] ✅ Generar documentación .ai/ completa — Completado: 2025-07-09
- [x] ✅ Crear meta-prompt de análisis profundo — Completado: 2025-07-09

---

## Notas

- Las tareas se extraen de comentarios `// TODO:` en el código y del estado del proyecto.
- Mover tareas entre secciones al cambiar su estado.
- Formato de prioridad: 🔴 Alta | 🟡 Media | 🟢 Baja
