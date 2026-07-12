# code_RF_procc — Hardware

## Referencias

### Placas

| Componente | MCU | Función | Notas |
|------------|-----|---------|-------|
| Arduino Nano | ATmega328P | Transmisor (TX) | Deep-sleep PWR_DOWN, wake por INT0 |
| Arduino Uno R3 | ATmega328P | Receptor (RX) | Siempre encendido, monitoreo serial |

### Periféricos

| Componente | Referencia | Cantidad | Protocolo |
|------------|-----------|----------|-----------|
| Módulo TX 433MHz (FS1000A) | No catalogado | 1 | ASK/OOK unidireccional |
| Módulo RX 433MHz (MX-RM-5V) | No catalogado | 1 | ASK/OOK unidireccional |
| Pulsador NO (Normalmente Abierto) | Genérico | 1 | GPIO con pull-up interno |
| Antena helicoidal 17cm | — | 2 | — |

## Tabla de Conexiones (Wiring)

### Transmisor (Arduino Nano)

| Periférico | Pin Periférico | Pin Nano | Función | Notas |
|------------|---------------|----------|---------|-------|
| Módulo TX 433MHz | DATA | D10 | Salida de datos RF | GPIO digital output |
| Módulo TX 433MHz | VCC | 5V | Alimentación | 5V para máximo alcance |
| Módulo TX 433MHz | GND | GND | Tierra | — |
| Pulsador | Terminal 1 | D2 | Wake interrupt (INT0) | INPUT_PULLUP, FALLING edge |
| Pulsador | Terminal 2 | GND | Tierra | — |
| Antena 17cm | — | ANT (módulo TX) | Antena | Mejora alcance de ~1m a ~100m |

### Receptor (Arduino Uno)

| Periférico | Pin Periférico | Pin Uno | Función | Notas |
|------------|---------------|---------|---------|-------|
| Módulo RX 433MHz | DATA | D2 | Entrada de datos RF | Interrupción INT0 para RCSwitch |
| Módulo RX 433MHz | VCC | 5V | Alimentación | 5V obligatorio |
| Módulo RX 433MHz | GND | GND | Tierra | — |
| Antena 17cm | — | ANT (módulo RX) | Antena | Crítica para recepción fiable |

## Consumo Energético

### Transmisor (Nano)

| Estado | Corriente | Duración típica | Notas |
|--------|-----------|-----------------|-------|
| PWR_DOWN sleep | ~5µA | 99% del tiempo | MCU + módulo TX en idle |
| Despertando | ~15mA | 30ms | Estabilización de oscilador |
| Transmitiendo (RF activo) | ~34mA | ~850ms/paquete | 3 códigos × 135ms send + 150ms gap × 3 repeats |
| Esperando release | ~15mA | Variable | Mientras botón presionado |

### Receptor (Uno)

| Estado | Corriente | Duración | Notas |
|--------|-----------|----------|-------|
| Idle (escuchando) | ~45mA | Continuo | MCU + módulo RX siempre activos |
| Procesando paquete | ~46mA | <1ms | Diferencia negligible |

## Alimentación

### Transmisor
- **Fuente:** USB (desarrollo) / Batería 3×AAA o LiPo 3.7V (producción)
- **Voltaje:** 5V (USB) o 3.3-5V (batería con regulador del Nano)
- **Autonomía estimada (3×AAA 1000mAh):**
  - Con 10 pulsaciones/día × 850ms activo = 8.5s activo/día
  - Consumo promedio: ~5µA (sleep domina)
  - Autonomía: **~200 días** (limitado por autodescarga)

### Receptor
- **Fuente:** USB permanente o adaptador 5V
- **Voltaje:** 5V
- **Autonomía:** N/A (alimentación continua requerida)

## Advertencias

> ⚠️ **Pin D2 compartido:** En el Nano, D2 es tanto la interrupción de wake (botón) como INT0. NO conectar otro periférico a D2 del Nano.

> ⚠️ **Pin D2 del Uno:** El módulo RX DEBE ir en D2 (INT0). RCSwitch usa `attachInterrupt()` con el número de interrupción, no el pin. Usar `digitalPinToInterrupt(2)` para portabilidad.

> ⚠️ **Antena obligatoria:** Sin antena de 17cm, el alcance se reduce a <1 metro. Con antena: 30-100m en línea de vista.

> ⚠️ **Alimentación del módulo RX:** El MX-RM-5V requiere 5V. Con 3.3V la sensibilidad cae drásticamente y puede no recibir nada.

> ⚠️ **Breadboard:** Los contactos flojos en breadboard son la causa #1 de "no recibe nada". Verificar con multímetro la continuidad del pin DATA.
