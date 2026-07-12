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