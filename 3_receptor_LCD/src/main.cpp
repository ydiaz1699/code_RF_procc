#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "RFProtocol.h"

const uint8_t RX_INTERRUPT_PIN = 2;

RFProtocolRx rf(RX_INTERRUPT_PIN);

hd44780_I2Cexp lcd;  // autodetecta dirección y pinout
const uint8_t LCD_COLS = 16;
const uint8_t LCD_ROWS = 2;

int16_t lastSeq = -1;
unsigned long lastSeqMs = 0;
const unsigned long DEDUPE_WINDOW_MS = 800;

void setup() {
  Serial.begin(9600);
  rf.begin();

  int status = lcd.begin(LCD_COLS, LCD_ROWS);
  if (status) {
    // fallo de inicialización: parpadea el LED integrado para avisar
    hd44780::fatalError(status);
  }

  lcd.print("Receptor listo");

  Serial.println("Receptor listo (protocolo empaquetado)");
}

void loop() {
  rf.update();

  if (rf.available()) {
    Packet p = rf.read();

    bool isDuplicate = (p.seq == lastSeq) && (millis() - lastSeqMs < DEDUPE_WINDOW_MS);
    lastSeq = p.seq;
    lastSeqMs = millis();

    if (isDuplicate) return;

    Serial.print("Paquete recibido seq=");
    Serial.print(p.seq);
    Serial.print(" type=0x");
    Serial.print(p.type, HEX);
    Serial.print(" len=");
    Serial.print(p.len);
    Serial.print(" payload=");
    for (uint8_t i = 0; i < p.len; i++) Serial.print((char)p.payload[i]);
    Serial.println();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Seq:");
    lcd.print(p.seq);
    lcd.print(" T:0x");
    lcd.print(p.type, HEX);

    lcd.setCursor(0, 1);
    for (uint8_t i = 0; i < p.len && i < LCD_COLS; i++) {
      lcd.print((char)p.payload[i]);
    }
  }
}