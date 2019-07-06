#include <msp430.h> 
#include <inttypes.h>

#include <font5x7.h>

#define ROWS (7)
#define COLUMNS (111)

#define ANODE_DIR (P2DIR)
#define ANODE_OUT (P2OUT)
#define ANODE_MASK (0x7F)

#define CATHODE_DIR (P1DIR)
#define CATHODE_OUT (P1OUT)

static const uint8_t cathodeBit[] = {BIT0, BIT6, BIT5, BIT4};

#define CATHODE_MASK (BIT0 + BIT4 + BIT5 + BIT6)
#define CATHODE_NUMBER (4)

#define TIMER_1USEC 16
#define TIMER_NORMAL_TICK (0xFFFF - (TIMER_1USEC*200))//170us
#define TIMER_FLUSH_TICK (0xFFFF - (TIMER_1USEC*700))//800us
#define DELAY_1US (TIMER_1USEC)//10us

volatile uint8_t currentShowPos = 0;
volatile uint8_t startShowPos = 0;
volatile uint8_t currentCathode = 0;

volatile uint8_t currentFillPos = 0;
volatile uint8_t startFillPos = 0;

uint8_t data[COLUMNS] = {0};



void TimerStart()
{
    TA0R = TIMER_NORMAL_TICK;
    //TA0CCR0 = TIMER_NORMAL_TICK;
    TA0CTL |= TASSEL1 + MC1 +TAIE + TAIFG;// + ID1 + ID0;
}

void TimerStop()
{
    TA0CTL = 0x00;
}

void initDisplay()
{
    CATHODE_DIR |= CATHODE_MASK + BIT7;
    CATHODE_OUT &= ~(CATHODE_MASK + BIT7);
    ANODE_DIR |= ANODE_MASK;
    //P2SEL2 |= BIT6;
    ANODE_OUT &= ~ANODE_MASK;

    TimerStart();
}

inline void sendAnodes(uint8_t data)
{
    ANODE_OUT = data & ANODE_MASK;
    if (data & BIT6)
    {CATHODE_OUT |= BIT7;}
    else
    {CATHODE_OUT &= ~BIT7;}
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
    sendAnodes(0);
    __delay_cycles(DELAY_1US*40);//need to wait 10us
    /*currentCathode++;
    if (currentCathode >= CATHODE_NUMBER)
    {
        currentCathode = 1;
    }*/
    sendCathode((pos % 3) + 1);
    __delay_cycles(DELAY_1US*20);//need to wait 10us
    sendAnodes(data[pos]);

}

inline void ClearData()
{
    for (uint8_t i = 0; i < COLUMNS; ++i)
    {
        data[i] = 0;
    }
}


inline void clearDisplay()
{
    ClearData();
   startShowPos = 0;
   currentShowPos = 0;
   startFillPos = 0;
   currentFillPos = 0;
}

void main(void)
{
   WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
    BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;
	//BCSCTL2 = DIVS1 + DIVS0;

	P1SEL |= BIT1 + BIT2 ; // P1.1 = RXD, P1.2=TXD
	P1SEL2 |= BIT1 + BIT2 ; // P1.1 = RXD, P1.2=TXD

	P1OUT |= BIT3;
	P1REN |= BIT3;

    /*Baud rate 38400*/
    UCA0CTL0 = 0x00;
    UCA0CTL1 = UCSSEL1|UCSWRST;
    /*See table 15-4*/
    UCA0BR0 = (416-256);
    UCA0BR1 = 416/256;
    UCA0MCTL = UCBRS2|UCBRS1;
    UCA0CTL1 &= ~UCSWRST;

    IE2 |= UCA0TXIE + UCA0RXIE;

    ClearData();

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
        if (!(P1IN & BIT3))
        {
            clearDisplay();
        }
        if (currentShowPos == 0)
        {
            flushDisplay();
            TA0R = TIMER_FLUSH_TICK;
        }
        else
        {
            nextRow(currentShowPos - 1);
            TA0R = TIMER_NORMAL_TICK;
        }
        currentShowPos++;
        if (currentShowPos > COLUMNS)
        {
            currentShowPos = 0;
        }
    }
}




void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) TxEnd(void)
{
    if (IFG2&UCA0TXIFG)
    {

    }
}

void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) RxGet(void)
{
    if (IFG2&UCA0RXIFG)
    {
        uint8_t symbol = UCA0RXBUF;

        if ((symbol == 0x0A) || (symbol == 0x0D))
        {
            clearDisplay();
        }
        else if (symbol >= 0x20)
        {
            for (uint8_t i = 0; i <= SYSTEMRUS5x7_WIDTH; ++i)
            {
                //Fill 5 columns from array plus one space column.
                data[currentFillPos++] = (i == SYSTEMRUS5x7_WIDTH)? 0 : System5x7_CP1251[(unsigned int)(symbol - 0x20)* SYSTEMRUS5x7_WIDTH + i];
                if (currentFillPos >= COLUMNS)
                {
                    currentFillPos = 0;
                    startShowPos++;
                }
            }
        }
        UCA0TXBUF = symbol;
       }

}

