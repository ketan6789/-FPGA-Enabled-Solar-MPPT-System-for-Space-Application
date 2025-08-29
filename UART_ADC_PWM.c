#include <msp430.h>

volatile float voltage = 0;

void uart_init(void);
void uart_send_string(const char *str);
void uart_send_char(char c);
void send_voltage_ascii(float v);
void pwm_init(void);  

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                 
    
   
    P1OUT &= ~BIT0;                           
    P1DIR |= BIT0;                            // P1.0 output

   
    P2SEL1 |= BIT0 | BIT1;                    // Set P2.0 = RXD, P2.1 = TXD
    P2SEL0 &= ~(BIT0 | BIT1);

   
    P4SEL1 |= BIT2;                           // Configure P4.2 for ADC
    P4SEL0 |= BIT2;


    PM5CTL0 &= ~LOCKLPM5;

    uart_init();                              
    pwm_init();                               


    while(REFCTL0 & REFGENBUSY);              
    REFCTL0 |= REFVSEL_2 | REFON;             // Select internal ref = 2.5V
                                              

    // Configure ADC12
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;                     
    ADC12CTL2 |= ADC12RES_2;                  // 12-bit conversion results
    ADC12IER0 |= ADC12IE0;                   
    ADC12MCTL0 |= ADC12INCH_10 | ADC12VRSEL_1; // A10 ADC input select; Vref=2.5V

    while(!(REFCTL0 & REFGENRDY));            
                                              

    while(1)
    {
        __delay_cycles(500000);               // Delay between conversions (~0.5s)
        ADC12CTL0 |= ADC12ENC | ADC12SC;      

        __bis_SR_register(LPM0_bits + GIE);   
        __no_operation();                     // For debug only

        
        uart_send_string("Voltage = ");
        send_voltage_ascii(voltage);
        uart_send_string(" V\r\n");
    }
}

void pwm_init(void)
{
    
    P1DIR |= BIT2;                
    P1SEL0 |= BIT2;                
    P1SEL1 &= ~BIT2;

    
    TA1CCR0 = 20 - 1;              
    TA1CCR1 = 2;                   
    TA1CCTL1 = OUTMOD_7;           
    TA1CTL = TASSEL_2 | MC_1 | TACLR; 
}


void uart_init(void)
{
    UCA0CTLW0 = UCSWRST;             
    UCA0CTLW0 |= UCSSEL__SMCLK;      

    // Baud Rate = 115200 @ 1MHz SMCLK
    UCA0BR0 = 8;                   
    UCA0BR1 = 0;
    UCA0MCTLW = 0xD600;             

    UCA0CTLW0 &= ~UCSWRST;          
}

void uart_send_char(char c)
{
    while (!(UCA0IFG & UCTXIFG));   
    UCA0TXBUF = c;                  
}

void uart_send_string(const char *str)
{
    while (*str)
    {
        uart_send_char(*str++);      
    }
}

void send_voltage_ascii(float v)
{
    unsigned int whole = (unsigned int)v;
    unsigned int frac = (unsigned int)((v - whole) * 1000); 

    uart_send_char('0' + whole);
    uart_send_char('.');
    uart_send_char('0' + (frac / 100));
    uart_send_char('0' + ((frac / 10) % 10));
    uart_send_char('0' + (frac % 10));
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch (__even_in_range(ADC12IV, ADC12IV_ADC12RDYIFG))
    {
        case ADC12IV_NONE:        break;        
        case ADC12IV_ADC12OVIFG:  break;        
        case ADC12IV_ADC12TOVIFG: break;        
        case ADC12IV_ADC12HIIFG:  break;        
        case ADC12IV_ADC12LOIFG:  break;        
        case ADC12IV_ADC12INIFG:  break;        
        case ADC12IV_ADC12IFG0:                 
            voltage = (ADC12MEM0 * 2.5f) / 4096.0f;
            if (ADC12MEM0 >= 0x666)            
                P1OUT |= BIT0;                 
            else
                P1OUT &= ~BIT0;                 
            __bic_SR_register_on_exit(LPM0_bits); 
            break;                              

        case ADC12IV_ADC12IFG1:   break;        
        case ADC12IV_ADC12IFG2:   break;        
        case ADC12IV_ADC12IFG3:   break;        
        case ADC12IV_ADC12IFG4:   break;        
        case ADC12IV_ADC12IFG5:   break;        
        case ADC12IV_ADC12IFG6:   break;        
        case ADC12IV_ADC12IFG7:   break;        
        case ADC12IV_ADC12IFG8:   break;        
        case ADC12IV_ADC12IFG9:   break;        
        case ADC12IV_ADC12IFG10:  break;        
        case ADC12IV_ADC12IFG11:  break;        
        case ADC12IV_ADC12IFG12:  break;        
        case ADC12IV_ADC12IFG13:  break;        
        case ADC12IV_ADC12IFG14:  break;        
        case ADC12IV_ADC12IFG15:  break;        
        case ADC12IV_ADC12IFG16:  break;        
        case ADC12IV_ADC12IFG17:  break;        
        case ADC12IV_ADC12IFG18:  break;        
        case ADC12IV_ADC12IFG19:  break;        
        case ADC12IV_ADC12IFG20:  break;        
        case ADC12IV_ADC12IFG21:  break;        
        case ADC12IV_ADC12IFG22:  break;        
        case ADC12IV_ADC12IFG23:  break;        
        case ADC12IV_ADC12IFG24:  break;        
        case ADC12IV_ADC12IFG25:  break;        
        case ADC12IV_ADC12IFG26:  break;        
        case ADC12IV_ADC12IFG27:  break;        
        case ADC12IV_ADC12IFG28:  break;        
        case ADC12IV_ADC12IFG29:  break;        
        case ADC12IV_ADC12IFG30:  break;        
        case ADC12IV_ADC12IFG31:  break;        
        case ADC12IV_ADC12RDYIFG: break;        
        default: break;
    }
}
