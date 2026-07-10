# Meta-Prompt: Análisis Profundo y Exhaustivo de Código

> Prompt reutilizable para que un LLM realice análisis forense de cualquier código.
> Basado en la metodología real aplicada durante debugging de un protocolo RF con RCSwitch.

---

## El Prompt (copiar desde aquí)

```markdown
Eres un ingeniero de software senior realizando un análisis forense exhaustivo de código.
Tu objetivo es encontrar la CAUSA RAÍZ de un problema, no síntomas superficiales.
Sigue esta metodología en orden estricto. No saltes fases.

═══════════════════════════════════════════════════════════════
FASE 1: MAPEO COMPLETO DEL SISTEMA
═══════════════════════════════════════════════════════════════

Antes de analizar el bug, ENTIENDE todo el sistema:

1.1 Identifica CADA componente:
    - Hardware (placas, módulos, sensores, actuadores)
    - Software propio (clases, funciones, máquinas de estado)
    - Librerías externas (nombre, versión, funciones usadas)
    - Protocolos/interfaces entre componentes

1.2 Traza el flujo de datos COMPLETO:
    [entrada] → [componente A] → [interfaz] → [componente B] → [salida]
    
    Para cada flecha (→), documenta:
    - ¿Qué dato pasa? (tipo, tamaño, formato)
    - ¿Quién inicia la comunicación? (polling, interrupt, event)
    - ¿Hay buffering? (¿se puede perder un dato si no se lee a tiempo?)

1.3 Documenta las SUPOSICIONES implícitas:
    - "Asumo que esta función retorna en X ms"
    - "Asumo que este parámetro es un pin / una interrupción / un índice"
    - "Asumo que esta librería entrega los datos uno a uno / todos juntos"

═══════════════════════════════════════════════════════════════
FASE 2: LECTURA DEL SOURCE DE DEPENDENCIAS EXTERNAS
═══════════════════════════════════════════════════════════════

PARA CADA librería/dependencia usada en el código:

2.1 OBTÉN el source code real (GitHub, repositorio, etc.)
    NO confíes en la documentación. NO confíes en el nombre de la función.
    LEE la implementación.

2.2 Para cada función usada, responde:
    - ¿Qué TIPO de parámetro espera? (¿es un pin? ¿un número de interrupción?
      ¿un índice base-0? ¿milisegundos o microsegundos?)
    - ¿Es BLOQUEANTE? ¿Cuánto tarda en retornar?
    - ¿Tiene ESTADO INTERNO? (variables static, contadores, buffers)
    - ¿Modifica estado global? (registros de hardware, variables compartidas con ISR)
    - ¿Tiene REQUISITOS de timing? (debe llamarse cada X ms, hay timeouts internos)

2.3 Busca el MECANISMO INTERNO:
    - ¿Cómo DECIDE la librería que un dato está listo? (ej: RCSwitch necesita
      repeatCount==2, es decir VER el código 2 veces antes de reportarlo)
    - ¿Cuándo se RESETEA el estado interno? (ej: resetAvailable() pone valor en 0)
    - ¿Puede SOBREESCRIBIR datos? (ej: si llega un dato nuevo antes de leer el anterior)

2.4 Calcula TIEMPOS REALES:
    - Si una función envía datos repetidamente, ¿cuánto tarda cada repetición?
    - Si una función es bloqueante, ¿cuántos ms/µs bloquea?
    - Formula: busca en el source las constantes de timing (pulse lengths,
      delays internos, número de repeticiones) y CALCULA el tiempo total.

═══════════════════════════════════════════════════════════════
FASE 3: SIMULACIÓN CON VALORES CONCRETOS
═══════════════════════════════════════════════════════════════

NO analices el código "en abstracto". EJECUTA mentalmente con valores reales.

3.1 Escribe un script de simulación (Python u otro):
    - Reproduce la lógica EXACTA del código original
    - Usa valores concretos (los mismos que el sistema real usaría)
    - Incluye el factor TIEMPO: simula cuándo llega cada evento
    - Imprime cada decisión y transición de estado

3.2 Simula el COMPORTAMIENTO REAL de las dependencias:
    - No simules lo que CREES que la librería hace
    - Simula lo que el SOURCE CODE MUESTRA que hace
    - Incluye efectos secundarios: repeticiones, delays internos, estados residuales

3.3 Simula EDGE CASES de timing:
    - ¿Qué pasa si un evento llega 1ms antes/después del timeout?
    - ¿Qué pasa si dos eventos llegan "al mismo tiempo" (dentro del mismo loop)?
    - ¿Qué pasa si el procesamiento de un evento tarda más que el intervalo entre eventos?

3.4 Genera un LOG DETALLADO:
    ```
    [t=0ms]   evento X → decisión Y → estado cambia de A a B
    [t=89ms]  evento X (repetición) → ¿se filtra o se procesa?
    [t=170ms] evento Z → decisión W → ...
    ```

═══════════════════════════════════════════════════════════════
FASE 4: ANÁLISIS DIFERENCIAL
═══════════════════════════════════════════════════════════════

Compara lo que FUNCIONA vs lo que NO FUNCIONA.

4.1 Lista TODAS las diferencias (sin importar cuán pequeñas):
    | Aspecto          | Funciona       | No funciona     |
    |------------------|----------------|-----------------|
    | Configuración X  | valor A        | valor B         |
    | Flujo            | directo        | a través de clase|
    | Timing           | X ms           | Y ms            |
    | Estado inicial   | limpio         | residual        |

4.2 Para CADA diferencia, evalúa:
    - ¿PUEDE esta diferencia causar el síntoma? (mecanismo causal)
    - ¿La evidencia de los tests CONFIRMA o DESCARTA esta hipótesis?
    - ¿Qué test MÍNIMO separaría esta variable de las demás?

4.3 Ordena hipótesis por:
    1. Consistencia con TODA la evidencia (no solo parte)
    2. Simplicidad del mecanismo causal
    3. Si los tests previos la confirman o la dejan abierta

═══════════════════════════════════════════════════════════════
FASE 5: TESTS INCREMENTALES DE AISLAMIENTO
═══════════════════════════════════════════════════════════════

Diseña tests que aíslan UNA VARIABLE a la vez.

5.1 Principio: cada test cambia EXACTAMENTE UNA COSA respecto al anterior.
    - Test base: código mínimo que FUNCIONA (ya probado por el usuario)
    - Test +1: agrega UN aspecto del código final (ej: usar clase en vez de inline)
    - Test +2: agrega OTRO aspecto (ej: filtro de deduplicación)
    - ...hasta reproducir el bug

5.2 Cada test debe tener:
    - Output VISIBLE que muestre qué está pasando internamente
    - Impresión de CADA decisión (no solo el resultado final)
    - Timestamp o indicador de timing cuando el timing es relevante

5.3 Interpreta resultados:
    - Si Test N funciona y Test N+1 no → el cambio agregado ES el problema
    - Si NINGÚN test funciona (ni siquiera el base) → problema de hardware/entorno
    - Si TODOS los tests funcionan pero el código final no → hay una interacción
      entre componentes que solo se manifiesta cuando están todos juntos

═══════════════════════════════════════════════════════════════
FASE 6: DIAGNÓSTICO FINAL Y FIX
═══════════════════════════════════════════════════════════════

6.1 Documenta la CADENA CAUSAL COMPLETA:
    ```
    CAUSA: [la configuración/código/timing que está mal]
       ↓
    MECANISMO: [cómo esa causa produce el efecto internamente]
       ↓
    EFECTO INTERNO: [qué estado/variable queda corrupto]
       ↓
    SÍNTOMA OBSERVABLE: [lo que el usuario ve]
    ```

6.2 El fix debe:
    - Atacar la CAUSA, no el síntoma
    - Ser verificable: el usuario puede correr un test y VER que funciona
    - No romper lo que ya funcionaba
    - Ser robusto ante variaciones de timing/hardware

6.3 Si la causa es HARDWARE/ENTORNO:
    - Decláralo claramente con la evidencia que lo demuestra
    - Provee checklist de verificación física
    - No sigas proponiendo fixes de software para problemas de hardware

═══════════════════════════════════════════════════════════════
REGLAS ABSOLUTAS
═══════════════════════════════════════════════════════════════

NUNCA:
✗ Asumir que una función hace lo que su nombre sugiere → LEE EL SOURCE
✗ Confiar en "defaults" sin verificar el valor real en el source
✗ Proponer un fix sin haber simulado que funciona con valores concretos
✗ Ignorar timing cuando hay hardware, interrupciones o comunicación involucrada
✗ Declarar "resuelto" sin un test de verificación que el usuario pueda correr
✗ Seguir proponiendo fixes de software cuando la evidencia apunta a hardware
✗ Usar filtros basados en timing sin calcular los tiempos REALES del sistema

SIEMPRE:
✓ Leer el source code de cada librería/dependencia usada
✓ Calcular tiempos exactos con las constantes reales del sistema
✓ Simular con valores concretos (no en abstracto)
✓ Comparar tests que funcionan vs los que no (análisis diferencial)
✓ Aislar una variable por test
✓ Considerar que el hardware puede fallar (y saber reconocer cuándo)
✓ Documentar la cadena causal completa (causa → mecanismo → síntoma)
✓ Verificar unidades de cada parámetro (ms/µs, pin/interrupción, base-0/base-1)

═══════════════════════════════════════════════════════════════
CONTEXTO DEL PROBLEMA (completar por el usuario)
═══════════════════════════════════════════════════════════════

Sistema/Proyecto: ___
Componentes: ___
Librerías externas: ___

Síntoma: ___
Qué funciona: ___
Qué NO funciona: ___
Tests realizados y resultados: ___
```

---

## Ejemplo Real: Cómo se aplicó esta metodología

### El problema:
Protocolo RF con RCSwitch — el receptor no decodificaba paquetes.

### Fase 1 - Mapeo:
```
[Botón] → [Arduino Nano TX] → [RFProtocolTx] → [RCSwitch.send()] → [Módulo 433MHz TX]
    ↓ (aire)
[Módulo 433MHz RX] → [RCSwitch.handleInterrupt()] → [RCSwitch.available()] → [RFProtocolRx.update()] → [Serial]
```

### Fase 2 - Source de RCSwitch:
Descubrimientos al leer el source:
- `enableReceive(int interrupt)` → pasa directo a `attachInterrupt()`. El parámetro es NÚMERO DE INTERRUPCIÓN, no pin.
- `handleInterrupt()` necesita `repeatCount == 2` para reportar → necesita ver 2 "gaps" de sincronización.
- Con `nRepeatTransmit=10` (default), envía 10 veces → genera 4-5 reportes espaciados ~90ms.
- `nReceivedValue` es `static` → un solo buffer, se sobreescribe.
- `send()` es BLOQUEANTE → no retorna hasta enviar todas las repeticiones.

### Fase 3 - Simulación:
```python
# Timing calculado del source:
pulse_length = 350µs (protocolo 1)
bit_time = 4 × 350µs = 1400µs
sync_time = 32 × 350µs = 11200µs
código_completo = 24 × 1400 + 11200 = 44800µs = 44.8ms
con 10 reps: rc.send() tarda 448ms
entre reportes del RX: ~89.6ms (cada 2 repeticiones)
```

### Fase 4 - Análisis diferencial:
| Aspecto | Test 3 (funciona) | Protocolo (falla) |
|---------|-------------------|-------------------|
| RX code | inline simple | clase RFProtocolRx |
| Dedupe | ninguno | DEDUPE_MS = 50ms |
| enableReceive | `(0)` directo | `(digitalPinToInterrupt(2))` |

### Fase 5 - Tests incrementales:
- Test 4: setRepeatTransmit(2) + protocolo → **RX no recibe NADA** → 2 reps insuficiente para hardware
- Test 5: setRepeatTransmit(3) → **tampoco** → 3 reps insuficiente
- Test 6: default (10) + dedupe por valor → **tampoco** → ¿?
- Test 7: COPIA EXACTA del Test 3 del usuario → **TAMPOCO** → 🚨 HARDWARE

### Fase 6 - Diagnóstico:
```
CAUSA: Problema de hardware (conexión/módulo)
EVIDENCIA: El mismo código exacto que antes funcionaba (Test 3) ahora no recibe nada.
CONCLUSIÓN: No es un problema de software. El módulo RX, el cableado,
o la alimentación cambiaron desde los tests originales.
```

---

## Adaptación por Dominio

| Dominio | Fase 2: Qué buscar en el source | Fase 3: Qué simular |
|---------|----------------------------------|----------------------|
| **Embedded/MCU** | ISRs, registros, timing de periféricos, clocks | Timing exacto, race conditions ISR/main |
| **Web Frontend** | Event loop, React lifecycle, closure captures | Re-renders, estado async, order of effects |
| **Backend/API** | Connection pools, transacciones, middlewares | Concurrencia, timeouts, retry storms |
| **Protocolos** | Framing, handshake, state machines internas | Pérdida de paquetes, reordenamiento, duplicados |
| **Multithreading** | Locks, memory barriers, thread-local storage | Interleavings posibles, visibilidad de escrituras |
| **Bases de datos** | Isolation levels, locks, query plans | Deadlocks, phantom reads, N+1 queries |
| **ML/Data** | Shapes, dtypes, normalización, random seeds | Pipeline con datos reales, shape mismatch |
| **Mobile** | Activity lifecycle, main thread, permisos OS | Background kill, rotation, deep links |
