#include <msp430.h>
#include <string.h>

void uart_init(void);
void uart_send_string(const char *str);

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;  

    // Configure UART pins
    P2SEL1 |= BIT0 | BIT1;        // Set P2.0 = RXD, P2.1 = TXD
    P2SEL0 &= ~(BIT0 | BIT1);

    PM5CTL0 &= ~LOCKLPM5;       

    uart_init();                  // Setup UART

    while (1)
    {
        uart_send_string("Hello World\r\n");
        __delay_cycles(1000000);  
    }
}

void uart_init(void)
{
    UCA0CTLW0 = UCSWRST;             
    UCA0CTLW0 |= UCSSEL__SMCLK;     

    
    UCA0BR0 = 8;                     // 1MHz / 115200 = ~8.68
    UCA0BR1 = 0;
    UCA0MCTLW = 0xD600;             

    UCA0CTLW0 &= ~UCSWRST;         
}

void uart_send_string(const char *str)
{
    while (*str)
    {
        while (!(UCA0IFG & UCTXIFG)); 
        UCA0TXBUF = *str++;           // Send character
    }
}
