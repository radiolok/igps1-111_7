#include <msp430.h> 


/**
 * main.c
 */
int initTimer()
{
    TA0R = 0x00;
    TA0CCR1 = 0x4FF;
    TA0CCR0 = 0xFFFF;
    TA0CTL =   MC0 + MC1 + ID0 + ID1 + TASSEL1;
    TA0CCTL1 = OUTMOD1 + OUTMOD0;
    return 0;
}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	DCOCTL = CALDCO_12MHZ;
    BCSCTL1 = CALBC1_12MHZ;
    BCSCTL2 = DIVS0 + DIVS1;

    P1DIR = BIT0 + BIT6;
    P1SEL = BIT6;
    P1OUT = BIT3;
    P1REN |= BIT3;

    initTimer();


    while(1)
    {
        if (!(P1IN & BIT3))
        {
            P1OUT |= BIT0;
        }
        else
        {
            P1OUT &= ~(BIT0);
        }

    }

    return 0;
}

