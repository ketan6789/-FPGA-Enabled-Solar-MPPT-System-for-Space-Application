#include <msp430.h>
#include <stdint.h>

#define FILTER_SIZE 10
#define VREF 75.0f         // Desired output voltage

// PI Controller
typedef struct {
    float fIn;         // Error 
    float fKp;         // Proportional gain
    float fKi;         // Integral gain
    float fDtSec;     

    float fPout;       // Proportional output
    float fIout;       // Integral output
    float fIprevIn;    // Previous proportional output (for integration)
    float fIprevOut;   // Previous integral output

    float fOut;        // Controller output (duty cycle)
    float fUpOutLim;   // Upper output limit (e.g., 1.0)
    float fLowOutLim;  // Lower output limit (e.g., 0.0)
} tPI;


void uart_init(void);
void uart_send_string(const char *str);
void uart_send_char(char c);
void send_voltage_ascii(float v);
void pwm_init(void);
void tPI_calc(tPI* ptPI);
void tPI_rst(tPI* ptPI);


volatile float voltage = 0;
volatile float Vout = 0;
volatile float duty_cycle = 0;

volatile uint16_t adc_buffer[FILTER_SIZE] = {0};
volatile uint8_t buffer_index = 0;
volatile uint8_t buffer_full = 0;

// PI controller parametervalues
tPI myPI = {
    .fKp = 0.05f,
    .fKi = 0.005f,
    .fDtSec = 0.5f,         // Sampling interval in seconds
    .fUpOutLim = 0.4f,
    .fLowOutLim = 0.0f
};

void pwm_init(void)
{
    P1DIR |= BIT2;               
    P1SEL0 |= BIT2;              
    P1SEL1 &= ~BIT2;

    TA1CCR0 = 19;                
    TA1CCTL1 = OUTMOD_7;      
    TA1CCR1 = 0;                 
    TA1CTL = TASSEL_2 | MC_1 | TACLR;  
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                 

    // GPIO Setup for LED
    P1OUT &= ~BIT0;                           
    P1DIR |= BIT0;                            

    // Configure UART pins
    P2SEL1 |= BIT0 | BIT1;                    
    P2SEL0 &= ~(BIT0 | BIT1);

    // Configure ADC pins
    P4SEL1 |= BIT2;                           
    P4SEL0 |= BIT2;

    
    PM5CTL0 &= ~LOCKLPM5;

    uart_init();                             
    pwm_init();                              
    tPI_rst(&myPI);                          

    
    while(REFCTL0 & REFGENBUSY);
    REFCTL0 |= REFVSEL_2 | REFON;             
    while(!(REFCTL0 & REFGENRDY));            

    // Configure ADC12
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12CTL2 |= ADC12RES_2;                  // 12-bit resolution
    ADC12IER0 |= ADC12IE0;
    ADC12MCTL0 |= ADC12INCH_10 | ADC12VRSEL_1; // A10, Vref=2.5V

    while(1)
    {
        __delay_cycles(500000);               // 0.5 second delay
        ADC12CTL0 |= ADC12ENC | ADC12SC;      

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

// PI Controller

void tPI_calc(tPI* ptPI)
{
    float fPreOut;

    ptPI->fPout = ptPI->fIn * ptPI->fKp;

    
    if ((ptPI->fOut < ptPI->fUpOutLim && ptPI->fOut > ptPI->fLowOutLim) ||
        (ptPI->fIn < 0 && ptPI->fOut >= ptPI->fUpOutLim) ||
        (ptPI->fIn > 0 && ptPI->fOut <= ptPI->fLowOutLim))
    {
        ptPI->fIout = ptPI->fIprevOut + 0.5f * ptPI->fDtSec *
            (ptPI->fPout * ptPI->fKi + ptPI->fIprevIn);
    }
    else
    {
        ptPI->fIout = ptPI->fIprevOut; 
    }

    ptPI->fIprevIn = ptPI->fPout;
    ptPI->fIprevOut = ptPI->fIout;

    fPreOut = ptPI->fPout + ptPI->fIout;


    if (fPreOut > ptPI->fUpOutLim) fPreOut = ptPI->fUpOutLim;
    if (fPreOut < ptPI->fLowOutLim) fPreOut = ptPI->fLowOutLim;

    ptPI->fOut = fPreOut;
}


// PI controller reset
void tPI_rst(tPI* ptPI)
{
    ptPI->fIn = 0.0f;
    ptPI->fIout = 0.0f;
    ptPI->fIprevIn = 0.0f;
    ptPI->fIprevOut = 0.0f;
    ptPI->fOut = 0.0f;
    ptPI->fPout = 0.0f;
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

            
                myPI.fIn = VREF - voltage;
                tPI_calc(&myPI);
                duty_cycle = myPI.fOut;
                
                if (duty_cycle > 0.5f)
                duty_cycle = 0.5f;

                
                TA1CCR1 = (uint16_t)(duty_cycle * TA1CCR0);

            
                if (avg_adc >= 0x666)
                    P1OUT |= BIT0;
                else
                    P1OUT &= ~BIT0;
            }

            __bic_SR_register_on_exit(LPM0_bits);
            break;
        }
        default: break;
    }
}

