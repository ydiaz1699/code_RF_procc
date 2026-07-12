Porque el contador (`seq`) **no vive en el receptor, vive en el transmisor**. El receptor solo muestra lo que le llega — no genera el número de secuencia.

Mirá el código:

```cpp
// transmisor/src/main.cpp
uint8_t txSeq = 0;   // ← acá nace el contador

case ST_TRANSMIT: {
  Packet p;
  p.seq = txSeq++;   // se incrementa acá, en el transmisor
  ...
```

Esa variable `txSeq` vive en la RAM **del transmisor**. Cuando reiniciás el **receptor**, lo único que se resetea es `lastSeq = -1` de su lado:

```cpp
// receptor/src/main.cpp
int16_t lastSeq = -1;   // esto sí se resetea al reiniciar el receptor
```

pero eso solo controla el filtro de duplicados local, no tiene nada que ver con el valor que trae el paquete. El paquete siempre trae el `seq` que el transmisor tenía guardado en ese momento, y el transmisor sigue funcionando (no se reinició), así que sigue mandando `seq` desde donde iba.

### Por qué el transmisor no pierde el contador tampoco

Ojo que esto es más sutil de lo que parece: el transmisor usa `SLEEP_MODE_PWR_DOWN`, pero eso **no es un reset ni un apagado real** — es un modo de bajo consumo que detiene los clocks pero mantiene la RAM intacta. Por eso `txSeq` sobrevive entre "dormidas". Solo se resetea a 0 si:

1. Le cortás la alimentación al transmisor (batería/cable), o
2. Se presiona el botón físico de reset, o
3. Se reprograma (sube el código de nuevo).

### Si querés que el receptor sepa cuándo el transmisor reinició

No hay forma de detectarlo con el protocolo actual, porque no hay ningún campo que indique "acabo de arrancar". Dos opciones simples:

**Opción A — Detectar salto hacia atrás (wraparound o reinicio)**
En el receptor, si `p.seq < lastSeq` y no es el caso normal de overflow de `uint8_t` (255→0), asumís que el transmisor se reinició:

```cpp
if (p.seq < lastSeq && lastSeq != 255) {
  Serial.println("Transmisor reiniciado (seq volvió atrás)");
}
```

**Opción B — Agregar un "boot ID" al paquete**
Sumar un byte aleatorio (o incremental en EEPROM) generado en el `setup()` del transmisor, que cambia cada vez que arranca. Así el receptor compara ese ID y sabe con certeza que hubo un reinicio, sin depender de heurísticas sobre el `seq`.

¿Te sirve más la A (simple, sin tocar el protocolo) o preferís la B (más robusta, pero hay que agregar un campo)?
---
# Análisis del proyecto `code_rf_procc`

Es un sistema transmisor/receptor RF a 433MHz basado en **RCSwitch**, con un protocolo propio de paquetes (sync + seq + type + len + payload + CRC8) empaquetado en códigos de 24 bits. Arquitectura limpia, bien comentada y con máquinas de estado explícitas en ambos lados. Aquí el desglose:

## Arquitectura general

- **Packet**: `seq`, `type`, `len`, `payload[16]` — estructura simple y correcta.
- **CRC8**: polinomio 0x07 (CRC-8-CCITT), calculado igual en ambos lados sobre `seq+type+len+payload`. Correcto y consistente.
- **Empaquetado**: cada código de 24 bits transporta 3 bytes; una trama completa usa 2 códigos de cabecera + `ceil(len/3)` códigos de payload.
- **TX**: máquina de estados de bajo consumo (`SLEEP_MODE_PWR_DOWN`, wake por interrupción en D2), con antirrebote y reenvío de trama completa 3 veces (`PACKET_REPEATS`).
- **RX**: máquina de estados de reensamblado por código, con filtro de duplicados por temporización y timeout de trama incompleta.

## Puntos fuertes

1. El **CRC8 al final valida cualquier "sync falso"** causado por ruido — aunque un 0xAA aparezca por casualidad en el aire, la trama solo se acepta si el checksum coincide. Buen diseño defensivo.
2. La comprobación `b0 > MAX_PAYLOAD` antes de entrar a `RX_WAIT_PAYLOAD` evita overflow del buffer `payload[16]`.
3. Comentarios explican bien *por qué* de los valores mágicos (`DEDUPE_MS=200`, `CODE_GAP_MS=150`, etc.), lo cual es raro de ver y muy valioso para mantenimiento.
4. Gestión de energía en el TX (deshabilita ADC/SPI/timers) es correcta para un dispositivo a batería.

## Riesgos y posibles bugs

1. **Timing "mágico" frágil**: el filtro anti-repetición (`DEDUPE_MS=200`) y el timeout de trama (`RX_TIMEOUT_MS=1000`) dependen de mediciones empíricas ("medido en Test 5") con un módulo/latencia específicos. Si cambian los módulos RF, la longitud de la antena, o el ruido ambiente, estos valores pueden desalinearse y provocar pérdida de códigos o falsos duplicados. Sería más robusto derivarlo de `_rc.getReceivedProtocol()`/duración real medida en runtime en vez de constantes fijas.

2. **Sin protección contra replay**: el protocolo no tiene rolling code ni timestamp — cualquier receptor SDR capturando la trama RF puede reproducirla más tarde y será aceptada como válida (el `seq` solo sirve para deduplicar, no autentica). Para un simple ON/OFF de botón puede ser aceptable, pero si esto controla algo sensible (puerta, relé de potencia) es una vulnerabilidad real.

3. **`ST_WAIT_RELEASE` con espera bloqueante (`while (digitalRead(BUTTON_PIN) == LOW) {}`)**: si el botón queda mecánicamente pegado o hay rebote raro, el dispositivo se queda despierto indefinidamente (drenando batería) fuera de la máquina de estados normal. Podría beneficiarse de un timeout máximo.

4. **`_state` en `RX_WAIT_HEADER2`**: si `b0 == MAX_PAYLOAD` (16) es válido, pero no hay chequeo de que `len` sea internamente consistente con nada más — es simplemente confiado y luego validado solo por CRC al final. Correcto, pero significa que un paquete con `len` corrupto pasará por `RX_WAIT_PAYLOAD` completo antes de fallar el CRC (no es bug, solo nota de diseño).

5. **Código duplicado entre `1_receptor/lib/RFProtocol` y `2_transmisor/lib/RFProtocol`**: `packet.h` y `crc8.h` están literalmente duplicados en ambos proyectos PlatformIO. Cualquier cambio futuro en el protocolo (p. ej. `MAX_PAYLOAD`) requiere editar en dos lugares — riesgo real de que ambos lados diverjan silenciosamente y el CRC deje de coincidir. Recomendaría moverlo a una librería compartida (submódulo git o `lib_extra_dirs` apuntando a una carpeta común).

6. **Secuencia `seq` de 8 bits sin gestión de overflow explícita**: no es un bug (el wraparound de 255→0 es intencional y el dedupe por ventana de tiempo lo cubre), pero si en el futuro se agregan más tipos de eventos con tasas más altas, vale la pena revisar si 800ms de ventana de dedupe sigue siendo suficiente.

7. **RCSwitch protocolo/pulso por defecto**: no se ve `setProtocol()` ni `setPulseLength()` explícito en ninguno de los dos lados — se usa el default de la librería. Funciona porque ambos coinciden, pero es un acoplamiento implícito no documentado; si alguien cambia el receptor sin tocar el transmisor (o viceversa), fallará silenciosamente.

## Sugerencias concretas

- Extraer `RFProtocol` (packet.h, crc8.h, RFProtocol.h/cpp) a una única carpeta compartida para evitar drift entre TX/RX.
- Agregar un contador de "paquete de config" (`TYPE_CONFIG` ya está definido pero no usado) para negociar dinámicamente timing/protocolo RCSwitch en vez de hardcodearlo.
- Si el uso real requiere seguridad (control de acceso, no solo notificación), añadir un nonce/contador monotónico verificado, no solo deduplicado.
- Poner un timeout de seguridad en `ST_WAIT_RELEASE`.

¿Quieres que profundice en alguno de estos puntos (por ejemplo, rediseñar el filtro de deduplicación de forma más robusta, o mostrar cómo compartir la librería entre ambos proyectos PlatformIO)?
