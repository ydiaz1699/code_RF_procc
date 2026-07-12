#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include "RFProtocol.h"
#include "packet.h"

// ---------- Configuración de hardware ----------
const uint8_t TX_PIN     = 10;
const uint8_t BUTTON_PIN = 2;   // D2 = interrupción externa (obligatorio para wake)
const unsigned long DEBOUNCE_MS = 250;
const uint8_t PACKET_REPEATS = 3;

RFProtocolTx rf(TX_PIN);
uint8_t txSeq = 0;

// ---------- Máquina de estados de la aplicación ----------
enum AppState { ST_SLEEP, ST_WAKE, ST_BUILD_PACKET, ST_TRANSMIT, ST_WAIT_RELEASE };
AppState state = ST_SLEEP;

volatile bool wokeByButton = false;
void onButtonInterrupt() { wokeByButton = true; }

void enterSleep() {
  Serial.println("Durmiendo...");
  Serial.flush();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonInterrupt, FALLING);

  power_adc_disable();
  power_spi_disable();
  power_timer1_disable();
  power_timer2_disable();

  sleep_cpu();

  // --- Aquí se reanuda tras la interrupción ---
  sleep_disable();
  power_all_enable();
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  rf.begin();
  Serial.println("Transmisor listo (RCSwitch + protocolo empaquetado)");
}

void loop() {
  switch (state) {

    case ST_SLEEP:
      enterSleep();
      if (wokeByButton) {
        wokeByButton = false;
        state = ST_WAKE;
      }
      break;

    case ST_WAKE: {
      // Estabilización tras despertar (oscilador necesita ~4ms, damos 30ms)
      unsigned long t0 = millis();
      while (millis() - t0 < 30) { /* espera */ }
      state = (digitalRead(BUTTON_PIN) == LOW) ? ST_BUILD_PACKET : ST_SLEEP;
      break;
    }

    case ST_BUILD_PACKET:
      // Punto de extensión: aquí se pueden leer sensores, batería, etc.
      state = ST_TRANSMIT;
      break;

    case ST_TRANSMIT: {
      Packet p;
      p.seq  = txSeq++;
      p.type = TYPE_BUTTON_EVENT;
      p.len  = 2;
      p.payload[0] = 'O';
      p.payload[1] = 'N';

      Serial.print("Transmitiendo seq=");
      Serial.print(p.seq);
      Serial.print(" type=0x");
      Serial.print(p.type, HEX);
      Serial.print(" payload=");
      for (uint8_t i = 0; i < p.len; i++) Serial.print((char)p.payload[i]);
      Serial.println();

      rf.sendPacket(p, PACKET_REPEATS);

      state = ST_WAIT_RELEASE;
      break;
    }

    case ST_WAIT_RELEASE:
      while (digitalRead(BUTTON_PIN) == LOW) { /* espera a soltar */ }
      {
        unsigned long t0 = millis();
        while (millis() - t0 < DEBOUNCE_MS) { /* debounce tras soltar */ }
      }
      state = ST_SLEEP;
      break;
  }
}
