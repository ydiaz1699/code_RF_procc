# Notas de Hardware

## Voltajes Críticos
- Módulo RX (MX-RM-5V): alimentar a **5V obligatorio**. Con 3.3V no recibe.
- Módulo TX (FS1000A): funciona a 3.3-12V. A 5V alcance ~30m, a 12V ~100m.
- Arduino Nano: si alimentas con batería, usar VIN (7-12V) o 5V directo.

## Errores Comunes
- **DATA del RX en pin incorrecto** → debe ser D2 (INT0). Cualquier otro pin = silencio total.
- **Sin antena** → alcance se reduce a <1m. Soldar cable rígido de 17.3cm (λ/4 a 433MHz).
- **Breadboard flojo** → causa #1 de "funcionaba ayer, hoy no". Verificar con multímetro.
- **Confundir pin con interrupción** → `enableReceive(0)` ≠ `enableReceive(2)`. Usar `digitalPinToInterrupt(2)`.

## Tips de Debugging
- Si RX no recibe nada: subir sketch simple `enableReceive(0)` + print. Si tampoco recibe → hardware.
- Tocar pin DATA del RX con dedo genera ruido → si Serial imprime valores, la interrupción funciona.
- `setRepeatTransmit(2)` NO funciona. Mínimo `(3)` para que RCSwitch valide.
- El TX con `rc.send()` es **bloqueante**: con 3 reps, cada código tarda ~135ms.

## Quirks
- RCSwitch necesita ver un código **2 veces** (2 gaps) antes de reportarlo → mínimo 3 transmisiones.
- `nReceivedValue` es estática/global: solo almacena UN código a la vez. Si no lees rápido, se pierde.
- El sync pulse de protocolo 1 dura 10.85ms (>4.3ms separationLimit) → funciona como gap detector.
- Tras deep-sleep, el oscilador del Nano necesita ~4ms para estabilizar. Dar 30ms antes de transmitir.
