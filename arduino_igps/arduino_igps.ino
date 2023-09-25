#include "font5x7.h"

#define ROWS (7)
#define COLUMNS (111)

static const uint8_t cathodeBit[] = { 1 << PD4, 1 << PD5, 1 << PD6, 1 << PD7 };

#define CATHODE_OUT (PORTD)
#define CATHODE_MASK ((1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7))
#define CATHODE_NUMBER (4)

#define TIMER_1USEC 16
#define TIMER_NORMAL_TICK (0xFFFF - (TIMER_1USEC * 200))  //170us
#define TIMER_FLUSH_TICK (0xFFFF - (TIMER_1USEC * 700))   //800us
#define DELAY_1US (TIMER_1USEC)                           //10us

volatile uint8_t currentShowPos = 0;
volatile uint8_t startShowPos = 0;
volatile uint8_t currentCathode = 0;

volatile uint8_t currentFillPos = 0;
volatile uint8_t startFillPos = 0;

uint8_t data[COLUMNS] = { 0 };

inline void __delay_cycles(uint8_t clocks) {
  do {
    __asm("nop");
  } while (--clocks);
}

void setup(void) {
  //Set anodes to output
  DDRC |= 0x2F;
  DDRB |= (1 << PB3) | (1 << PB5) | (1<<PB4);
  //Set cathodes to output
  DDRD |= 0xFC;
  Serial.begin(38400);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }
  //Start Timer:
  TIMSK1 |= (1 << TOIE1);
  TCNT1 = TIMER_NORMAL_TICK;
  TCCR1A = 0x00;
  TCCR1B = (1 << CS10);
  ClearData();
  sei();
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
          currentFillPos = 0;
          startShowPos++;
        }
      }
    }
    Serial.write(symbol & 0xff);
  }
}

inline void sendAnodes(uint8_t data) {
  PORTC &= ~0x3F;
  PORTB &= ~(1 << PB5);
  PORTC |= data & 0x2F;
  PORTB |= (data >> 1) & 0x20;
}

inline void sendCathode(uint8_t cathode) {
  CATHODE_OUT &= ~CATHODE_MASK;
  if (cathode < CATHODE_NUMBER) {
    CATHODE_OUT |= cathodeBit[cathode];
  }
}

inline void flushDisplay() {
  sendCathode(0);
  sendAnodes(0);
}

inline void nextRow(uint8_t pos) {
  sendAnodes(0);
  __delay_cycles(DELAY_1US * 40);  //need to wait 10us
  sendCathode((pos % 3) + 1);
  __delay_cycles(DELAY_1US * 20);  //need to wait 10us
  sendAnodes(data[pos]);
}

inline void ClearData() {
  for (uint8_t i = 0; i < COLUMNS; ++i) {
    data[i] = 0;
  }
}

inline void clearDisplay() {
  ClearData();
  startShowPos = 0;
  currentShowPos = 0;
  startFillPos = 0;
  currentFillPos = 0;
}

ISR(TIMER1_OVF_vect) {
  PORTB |= (1<< PB4);
  if (currentShowPos == 0) {
    if (PINB & (1<<PB3))
      PORTB &= ~(1<< PB3);
    else
      PORTB |= (1<< PB3);
    TCNT1 = TIMER_FLUSH_TICK;
    flushDisplay();
  } else {
    TCNT1 = TIMER_NORMAL_TICK;
    nextRow(currentShowPos - 1);
  }
  currentShowPos++;
  if (currentShowPos > COLUMNS) {
    currentShowPos = 0;
  }
  PORTB &= ~(1<< PB4);
}
