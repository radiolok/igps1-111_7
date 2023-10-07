#include <avr/io.h>
#include <avr/interrupt.h>
#include "font5x7.h"
//pin31  - zero - D6



//#define DEBUG_PATTERN
#define COLUMNS (111)
uint8_t data[COLUMNS] = { 0 };
void strobe() {
  PORTD |= (1 << 4);
  delayMicroseconds(20);
  PORTD &= ~(1 << 4);
}

void sendByte(uint8_t data) {
  PORTB &= ~(0x0F);
  PORTC &= ~(0x0F);
  PORTB |= (data << 1) & 0x0F;
  PORTC |= (data >> 3) & 0x0F;
  strobe();
}

volatile uint8_t currentShowPos = 0;
volatile uint8_t startShowPos = 0;
volatile uint8_t currentCathode = 0;

volatile uint8_t currentFillPos = 3;
volatile uint8_t startFillPos = 3;

void setup() {
  // put your setup code here, to run once:
  DDRB |= 0x3F;
  DDRC |= 0x0F;
  DDRD |= (1 << 4);
  DDRD &= ~(1<<PD6);
  PORTD |= (1<<PD6);
  DDRD &= ~(1 << 2);
  //PORTD |= (1<<3);
  PORTD &= ~(1 << 4);
  Serial.begin(38400);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }
  attachInterrupt(digitalPinToInterrupt(3), blink, RISING);
}

uint8_t column = 0;

inline void ClearData() {
  for (uint8_t i = 0; i < COLUMNS; ++i) {
    data[i] = 0;
  }
}

inline void clearDisplay() {
  ClearData();
  startShowPos = 0;
  currentShowPos = 0;
  startFillPos = 3;
  currentFillPos = 3;
}


void loop() {
  if (Serial.available() > 0) {
    // get incoming byte:
    int symbol = Serial.read();
    if ((symbol == 0x0A) || (symbol == 0x0D)) {
      clearDisplay();
    } else if (symbol >= 0x20) {
      for (uint8_t i = 0; i <= SYSTEMRUS5x7_WIDTH; ++i) {
        //Fill 5 columns from array plus one space column.
        data[currentFillPos++] = (i == SYSTEMRUS5x7_WIDTH) ? 0 : System5x7_CP1251[(unsigned int)(symbol - 0x20) * SYSTEMRUS5x7_WIDTH + i];
        if (currentFillPos >= COLUMNS) {
          currentFillPos = 3;
          startShowPos++;
        }
      }
    }
    Serial.write(symbol & 0xff);
  }
}

void blink(){
  PORTB |= (1<<PB5);
  //PD2 - ГТИ
  if (PIND & (1 << PD6)) {
    column = 0;
  } else {
    /*if (column > COLUMNS)
    {
      column = 0;
    }*/
    column++;
  }
#if defined(DEBUG_PATTERN)
  sendByte((column % 2)? 0xAA : 0x55);
#else
  sendByte(data[column]);
#endif
  PORTB &= ~(1<<PB5);

}
