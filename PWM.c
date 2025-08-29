#include <msp430.h>

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;        

    
    PM5CTL0 &= ~LOCKLPM5;

    
    FRCTL0 = FRCTLPW | NWAITS_1;     
    CSCTL0_H = CSKEY_H;              
    CSCTL1 = DCOFSEL_3 | DCORSEL;    // DCO = 8 MHz
    CSCTL2 = SELS__DCOCLK;                
    CSCTL3 = DIVS__1;               
    CSCTL0_H = 0;                   

    
    P1DIR |= BIT2;                   // Set P1.2 as output
    P1SEL0 |= BIT2;                  
    P1SEL1 &= ~BIT2;

    
    TA1CCR0 = 159;                   // (8MHz / 50kHz) - 1 = 159
    TA1CCR1 = 16;                    // 10% Duty Cycle
    TA1CCTL1 = OUTMOD_7;            
    TA1CTL = TASSEL_2 | MC_1 | TACLR; 

    while (1);                       
}
