#include <msp430.h>
#include <stdint.h>
#define FILTER_SIZE 200

volatile float voltage = 0;
volatile float Vout = 0;

void uart_init(void);
void uart_send_string(const char *str);
void uart_send_char(char c);
void send_voltage_ascii(float v);

volatile uint16_t adc_buffer[FILTER_SIZE] = {0}; 
volatile uint8_t buffer_index = 0;
volatile uint8_t buffer_full = 0; 

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                
    
    // GPIO Setup for LED
    P1OUT &= ~BIT0;                          
    P1DIR |= BIT0;                            // P1.0 output

    // Configure UART pins
    P2SEL1 |= BIT0 | BIT1;                    // Set P2.0 = RXD, P2.1 = TXD
    P2SEL0 &= ~(BIT0 | BIT1);

    // Configure ADC pins
    P4SEL1 |= BIT2;                           // Configure P4.2 for ADC
    P4SEL0 |= BIT2;

    
    PM5CTL0 &= ~LOCKLPM5;

    uart_init();                              // Setup UART

    
    while(REFCTL0 & REFGENBUSY);              
    REFCTL0 |= REFVSEL_2 | REFON;             // Internal ref = 2.5V ON
    while(!(REFCTL0 & REFGENRDY));           

    // Configure ADC12
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12CTL2 |= ADC12RES_2;                 
    ADC12IER0 |= ADC12IE0;
    ADC12MCTL0 |= ADC12INCH_10 | ADC12VRSEL_1; // A10, Vref=2.5V

    while(1)
    {
        __delay_cycles(100000);               // 0.1 second delay
        ADC12CTL0 |= ADC12ENC | ADC12SC;      // Start conversion

        __bis_SR_register(LPM0_bits + GIE);   
        __no_operation();                    

        
        if (buffer_full) {
            uart_send_string("Raw ADC = ");
            send_voltage_ascii((float)adc_buffer[buffer_index == 0 ? FILTER_SIZE-1 : buffer_index-1]);
            uart_send_string(", Vout = ");
            send_voltage_ascii(Vout);
            uart_send_string("V, Voltage = ");
            send_voltage_ascii(voltage);
            uart_send_string(" V\r\n");
        }
    }
}

void uart_init(void)
{
    UCA0CTLW0 = UCSWRST;                    
    UCA0CTLW0 |= UCSSEL__SMCLK;               

    UCA0BR0 = 8;                              
    UCA0BR1 = 0;
    UCA0MCTLW = 0xD600;                       // 115200 baud

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
    if (v < 0) {
        uart_send_char('-');
        v = -v;
    }

    unsigned int whole = (unsigned int)v;
    unsigned int frac = (unsigned int)((v - whole) * 1000);

    if (whole >= 1000) {
        uart_send_char('0' + (whole / 1000));
        uart_send_char('0' + ((whole / 100) % 10));
        uart_send_char('0' + ((whole / 10) % 10));
        uart_send_char('0' + (whole % 10));
    }
    else if (whole >= 100) {
        uart_send_char('0' + (whole / 100));
        uart_send_char('0' + ((whole / 10) % 10));
        uart_send_char('0' + (whole % 10));
    }
    else if (whole >= 10) {
        uart_send_char('0' + (whole / 10));
        uart_send_char('0' + (whole % 10));
    }
    else {
        uart_send_char('0' + whole);
    }

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
        {
            
            adc_buffer[buffer_index] = ADC12MEM0;
            buffer_index++;
            
            if (buffer_index >= FILTER_SIZE) {
                buffer_index = 0;
                buffer_full = 1; 
            }

            
            if (buffer_full) {
                
                uint32_t sum = 0;
                uint8_t i;
                for (i = 0; i < FILTER_SIZE; i++) {
                    sum += adc_buffer[i];
                }
                uint16_t avg_adc = sum / FILTER_SIZE;

                Vout = (avg_adc * 2.5f) / 4096.0f;

                
               
                 voltage = 772.0f * (Vout - 1.286f);
                 
                
                if (avg_adc >= 0x666)
                    P1OUT |= BIT0;
                else
                    P1OUT &= ~BIT0;
            }

            __bic_SR_register_on_exit(LPM0_bits);
            break;
        }

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
