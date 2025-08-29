#include <msp430.h>
#include <stdint.h>

#define FILTER_SIZE 200

volatile float voltage = 0;
volatile float Vout = 0;
volatile float sensor2 = 0;
volatile float current = 0;        

volatile uint16_t adc_buffer1[FILTER_SIZE] = {0};
volatile uint16_t adc_buffer2[FILTER_SIZE] = {0};
volatile uint16_t buffer_index1 = 0;
volatile uint16_t buffer_index2 = 0;
volatile uint8_t buffer_full1 = 0;
volatile uint8_t buffer_full2 = 0;

uint8_t current_channel = 0; // 0 for A10, 1 for A7

void uart_init(void);
void uart_send_string(const char *str);
void uart_send_char(char c);
void send_voltage_ascii(float v);

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                

    P1OUT &= ~BIT0;
    P1DIR |= BIT0;

    // Configure UART pins
    P2SEL1 |= BIT0 | BIT1;
    P2SEL0 &= ~(BIT0 | BIT1);

    // Configure ADC pins
    P4SEL1 |= BIT2;                           // A10 on P4.2
    P4SEL0 |= BIT2;

    P2SEL1 |= BIT4;                           // A7 on P2.4
    P2SEL0 |= BIT4;

    PM5CTL0 &= ~LOCKLPM5;

    uart_init();

    
    while(REFCTL0 & REFGENBUSY);
    REFCTL0 |= REFVSEL_2 | REFON;
    while(!(REFCTL0 & REFGENRDY));

    
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12CTL2 |= ADC12RES_2;
    ADC12IER0 |= ADC12IE0;

    
    ADC12MCTL0 = ADC12INCH_10 | ADC12VRSEL_1;

    while(1)
    {
        __delay_cycles(100000);

        ADC12CTL0 &= ~ADC12ENC; 
        
        if(current_channel == 0)
            ADC12MCTL0 = ADC12INCH_10 | ADC12VRSEL_1;
        else
            ADC12MCTL0 = ADC12INCH_7 | ADC12VRSEL_1;
        
        ADC12CTL0 |= ADC12ENC | ADC12SC;  

        __bis_SR_register(LPM0_bits + GIE);
        __no_operation();

        if (buffer_full1 && buffer_full2)
        {
            uart_send_string("Vout = ");
            send_voltage_ascii(Vout);
            uart_send_string(" V, Voltage = ");
            send_voltage_ascii(voltage);
            uart_send_string(" V, Sensor2 = ");
            send_voltage_ascii(sensor2);
            uart_send_string(" V, Current = ");
            send_voltage_ascii(current);
            uart_send_string(" A\r\n");
        }
    }
}

void uart_init(void)
{
    UCA0CTLW0 = UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK;

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
        uart_send_char(*str++);
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
        case ADC12IV_ADC12IFG0:
        {
            uint16_t value = ADC12MEM0;

            if (current_channel == 0) // A10
            {
                adc_buffer1[buffer_index1] = value;
                buffer_index1++;
                if (buffer_index1 >= FILTER_SIZE) {
                    buffer_index1 = 0;
                    buffer_full1 = 1;
                }

                if (buffer_full1)
                {
                    uint32_t sum = 0;
                    uint16_t i;
                    for (i = 0; i < FILTER_SIZE; i++)
                        sum += adc_buffer1[i];
                    uint16_t avg_adc = sum / FILTER_SIZE;

                    Vout = (avg_adc * 2.5f) / 4096.0f;
                    voltage = 772.0f * (Vout - 1.286f);

                    if (avg_adc >= 0x666)
                        P1OUT |= BIT0;
                    else
                        P1OUT &= ~BIT0;
                }

                current_channel = 1;
            }
            else if (current_channel == 1) // A7
            {
                adc_buffer2[buffer_index2] = value;
                buffer_index2++;
                if (buffer_index2 >= FILTER_SIZE) {
                    buffer_index2 = 0;
                    buffer_full2 = 1;
                }

                if (buffer_full2)
                {
                    uint32_t sum = 0;
                    uint16_t i;
                    for (i = 0; i < FILTER_SIZE; i++)
                        sum += adc_buffer2[i];
                    uint16_t avg_adc = sum / FILTER_SIZE;

                    sensor2 = (avg_adc * 2.5f) / 4096.0f;
                    current = (sensor2 - 1.65f) / 0.05f;  
                }

                current_channel = 0; 
            }

            __bic_SR_register_on_exit(LPM0_bits);
            break;
        }
        default: break;
    }
}

