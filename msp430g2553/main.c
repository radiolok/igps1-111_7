#include <msp430.h> 
#include <inttypes.h>


#define ROWS (7)
#define COLUMNS (111)

#define ANODE_DIR (P2DIR)
#define ANODE_OUT (P2OUT)
#define ANODE_MASK (0x7F)

#define CATHODE_DIR (P1DIR)
#define CATHODE_OUT (P1OUT)

static const uint8_t cathodeBit[] = {BIT3, BIT4, BIT5, BIT6};

#define CATHODE_MASK (BIT3 + BIT4 + BIT5 + BIT6)
#define CATHODE_NUMBER (4)

#define TIMER_1USEC 16
#define TIMER_NORMAL_TICK (0xFFFF - (TIMER_1USEC*158))//170us
#define TIMER_FLUSH_TICK (0xFFFF - (TIMER_1USEC*700))//800us
#define DELAY_10US (TIMER_1USEC * 10)//10us

volatile uint8_t currentPos = 0;
uint8_t currentCathode = 0;

uint8_t data[COLUMNS] = {0};



void TimerStart()
{
    TA0R = TIMER_NORMAL_TICK;
    //TA0CCR0 = TIMER_NORMAL_TICK;
    TA0CTL |= TASSEL1 + MC1 +TAIE + TAIFG;
}

void TimerStop()
{
    TA0CTL = 0x00;
}

void initDisplay()
{
    CATHODE_DIR |= CATHODE_MASK;
    CATHODE_OUT &= ~CATHODE_MASK;
    ANODE_DIR |= ANODE_MASK;
    ANODE_OUT &= ~ANODE_MASK;

    TimerStart();
}

inline void sendAnodes(uint8_t data)
{
    ANODE_OUT = data & ANODE_MASK;
}

inline void sendCathode(uint8_t cathode)
{
    CATHODE_OUT &= ~CATHODE_MASK;
    if (cathode < CATHODE_NUMBER)
    {
        CATHODE_OUT |= cathodeBit[cathode];
    }
}


inline void flushDisplay()
{
    currentCathode = 0;
    sendCathode(currentCathode);
    sendAnodes(0);
}

inline void nextRow(uint8_t pos)
{
    currentCathode++;
    if (currentCathode >= CATHODE_NUMBER)
    {
        currentCathode = 1;
    }
    sendCathode(currentCathode);
    __delay_cycles(DELAY_10US);//need to wait 10us
    sendAnodes(data[pos]);

}




void main(void)
{
   WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
    BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;




    initDisplay();

    while(1)
    {
        __bis_SR_register(LPM1_bits + GIE);
    }
}



void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) TAIFTick(void)
{
    if (TA0IV & TA0IV_TAIFG)
    {
        if (currentPos == 0)
        {
            flushDisplay();
            TA0R = TIMER_FLUSH_TICK;
        }
        else
        {
            nextRow(currentPos - 1);
            TA0R = TIMER_NORMAL_TICK;
        }
        currentPos++;
        if (currentPos > COLUMNS)
        {
            currentPos = 0;
        }
    }
}

void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) CC0Tick(void)
{


}

