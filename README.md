# Protocolo RF 433MHz — Arduino Nano → Uno

Sistema de comunicación inalámbrica punto-a-punto usando módulos RF 433MHz con protocolo empaquetado custom sobre RCSwitch.

## Qué Hace

Cuando presionas un botón en el Arduino Nano (transmisor), este despierta de deep-sleep, envía un paquete RF con tipo de evento + payload, y vuelve a dormir. El Arduino Uno (receptor) recibe, valida CRC, filtra duplicados, e imprime el evento por Serial.

## Hardware Necesario

| Componente | Cantidad | Precio aprox. |
|------------|----------|---------------|
| Arduino Nano (ATmega328P) | 1 | ~$6 |
| Arduino Uno R3 | 1 | ~$12 |
| Módulo TX 433MHz (FS1000A) | 1 | ~$1 |
| Módulo RX 433MHz (MX-RM-5V) | 1 | ~$1 |
| Pulsador NO | 1 | ~$0.10 |
| Cable antena 17cm | 2 | ~$0 |
| Cables dupont / Breadboard | — | — |

## Conexiones

### Transmisor (Nano)

| Periférico | → Pin Nano |
|------------|-----------|
| TX 433MHz DATA | D10 |
| TX 433MHz VCC | 5V |
| TX 433MHz GND | GND |
| Botón terminal 1 | D2 |
| Botón terminal 2 | GND |

### Receptor (Uno)

| Periférico | → Pin Uno |
|------------|----------|
| RX 433MHz DATA | D2 |
| RX 433MHz VCC | 5V |
| RX 433MHz GND | GND |

> **Importante:** Soldar o conectar una antena de 17cm al pad ANT de ambos módulos RF. Sin antena el alcance es <1 metro.

## Instalación

```bash
# 1. Clonar repositorio
git clone https://github.com/ydiaz1699/code_RF_procc.git
cd code_RF_procc

# 2. Instalar PlatformIO (si no lo tienes)
pip install platformio

# 3. Compilar y subir el transmisor
cd transmisor
pio run --target upload

# 4. Compilar y subir el receptor (en otra terminal)
cd ../receptor
pio run --target upload

# 5. Monitorear el receptor
pio device monitor --baud 9600
```

## Uso

1. Conecta ambos Arduinos por USB
2. El receptor mostrará: `Receptor listo (protocolo empaquetado)`
3. Presiona el botón en el Nano
4. El receptor imprime: `Paquete recibido seq=0 type=0x1 len=2 payload=ON`

## Estructura del Proyecto

```
code_RF_procc/
├── transmisor/          # Arduino Nano (TX) - PlatformIO project
│   ├── src/main.cpp     # App: sleep → wake → transmit → sleep
│   └── lib/RFProtocol/  # Librería: fragmenta packets en códigos 24-bit
├── receptor/            # Arduino Uno (RX) - PlatformIO project
│   ├── src/main.cpp     # App: loop → recibe → dedup → imprime
│   └── lib/RFProtocol/  # Librería: re-ensambla códigos en packets
├── .ai/                 # Documentación técnica para IA
│   ├── PROJECT_CONTEXT.md
│   ├── ARCHITECTURE.md
│   ├── HARDWARE.md
│   ├── PROTOCOL.md
│   ├── SOFTWARE.md
│   └── DECISIONS.md
├── test4-7_*/           # Tests de diagnóstico incrementales
└── meta-prompt-analisis-profundo.md
```

## Protocolo RF (resumen)

Cada paquete se fragmenta en códigos RCSwitch de 24 bits:

```
Código 0: [SYNC=0xAA][seq][type]     ← Sincronización
Código 1: [len][CRC8][0x00]          ← Header con integridad
Código 2: [payload bytes, 3/código]  ← Datos
```

- CRC-8 (polinomio 0x07) valida integridad
- `setRepeatTransmit(3)` = mínimo para recepción fiable
- Deduplicación en dos niveles: por código (200ms) y por seq (800ms)

Documentación completa del protocolo en `.ai/PROTOCOL.md`.

## Troubleshooting

| Problema | Causa probable | Solución |
|----------|---------------|----------|
| RX no recibe nada | Pin DATA del módulo RX no está en D2 | Verificar conexión física al pin D2 del Uno |
| RX no recibe nada | Sin antena en módulo RX/TX | Soldar cable de 17cm al pad ANT |
| RX no recibe nada | Módulo RX alimentado a 3.3V | Conectar a 5V (obligatorio para MX-RM-5V) |
| TX imprime "Transmitiendo" pero RX silencioso | Contacto flojo en breadboard | Verificar continuidad con multímetro |
| Paquetes recibidos con CRC FAIL | Interferencia RF o distancia excesiva | Acercar módulos, verificar antenas |
| Múltiples eventos por una pulsación | DEDUPE_WINDOW_MS muy corto | Verificar que sea 800ms en receptor |
| TX no despierta con botón | Botón no conectado a D2 del Nano | D2 es obligatorio (INT0 para wake) |
| Serial muestra basura | Baud rate incorrecto | Verificar `monitor_speed = 9600` en platformio.ini |

## Bajo Consumo (TX)

El Nano transmisor usa deep-sleep PWR_DOWN (~5µA). Con 3×AAA (1000mAh) y uso típico de 10 pulsaciones/día, la autonomía estimada es **~200 días**.

## Licencia

MIT

## Documentación Técnica

La carpeta `.ai/` contiene documentación detallada siguiendo el framework [vsCode-AI](https://github.com/ydiaz1699/vsCode-AI):

- `ARCHITECTURE.md` — Máquinas de estado, diagramas de flujo
- `HARDWARE.md` — Wiring completo, consumo energético
- `PROTOCOL.md` — Formato de paquetes, timing, CRC, ejemplos
- `SOFTWARE.md` — Librerías, comandos, configuración
- `DECISIONS.md` — Decisiones arquitectónicas (ADRs)
