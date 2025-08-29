#include <msp430.h>
#include <stdint.h>

#define FILTER_SIZE 200
#define PWM_PERIOD 1000         // PWM period for 1kHz frequency
#define DUTY_STEP 10            // Duty cycle step size (1%)
#define MIN_DUTY 100            // Minimum duty cycle (10%)
#define MAX_DUTY 500            // Maximum duty cycle (50%)
#define MPPT_DELAY 15            // Number of cycles to wait between MPPT adjustments

volatile float voltage = 0;
volatile float Vout = 0;
volatile float sensor2 = 0;
volatile float current = 0;
volatile float power = 0;
volatile float prev_power = 0;
volatile float prev_voltage = 0;

volatile uint16_t adc_buffer1[FILTER_SIZE] = {0};
volatile uint16_t adc_buffer2[FILTER_SIZE] = {0};
volatile uint16_t buffer_index1 = 0;
volatile uint16_t buffer_index2 = 0;
volatile uint8_t buffer_full1 = 0;
volatile uint8_t buffer_full2 = 0;

// MPPT variables
volatile uint16_t duty_cycle = 100;     // Initial duty cycle (10%)
volatile uint8_t mppt_counter = 0;
volatile uint8_t mppt_direction = 1;    // 1 for increase, 0 for decrease
volatile uint8_t mppt_enabled = 0;

uint8_t current_channel = 0; // 0 for A10, 1 for A7

void uart_init(void);
void uart_send_string(const char *str);
void uart_send_char(char c);
void send_voltage_ascii(float v);
void pwm_init(void);
void set_duty_cycle(uint16_t duty);
void mppt_algorithm(void);

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
    pwm_init();

    
    while(REFCTL0 & REFGENBUSY);
    REFCTL0 |= REFVSEL_2 | REFON;
    while(!(REFCTL0 & REFGENRDY));

    // Configure ADC12
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12CTL2 |= ADC12RES_2;
    ADC12IER0 |= ADC12IE0;


    ADC12MCTL0 = ADC12INCH_10 | ADC12VRSEL_1;

    uart_send_string("MPPT System Initialized\r\n");
    uart_send_string("Constant Irradiance, 25°C Operation\r\n");

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
            // Calculate power
            power = voltage * current;
            
            // Run MPPT algorithm
            mppt_algorithm();
            
            // Print readings every few cycles
            if (mppt_counter == 0)
            {
                uart_send_string("V=");
                send_voltage_ascii(voltage);
                uart_send_string("V, I=");
                send_voltage_ascii(current);
                uart_send_string("A, P=");
                send_voltage_ascii(power);
                uart_send_string("W, Duty=");
                send_voltage_ascii((float)duty_cycle / 10.0f);
                uart_send_string("%\r\n");
            }
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

void pwm_init(void)
{
    P1DIR |= BIT2;
    P1SEL0 |= BIT2;
    P1SEL1 &= ~BIT2;
    
    TA1CTL = TASSEL__SMCLK | MC__UP | TACLR;
    TA1CCR0 = PWM_PERIOD - 1;                    // Set PWM period
    TA1CCR1 = duty_cycle;                        // Set initial duty cycle
    TA1CCTL1 = OUTMOD_7;                         // Reset/Set mode
}

void set_duty_cycle(uint16_t duty)
{
    if (duty >= MIN_DUTY && duty <= MAX_DUTY)
    {
        duty_cycle = duty;
        TA1CCR1 = duty_cycle;
    }
}

void mppt_algorithm(void)
{
    mppt_counter++;
    

    if (mppt_counter >= MPPT_DELAY)
    {
        mppt_counter = 0;
        
        if (!mppt_enabled)
        {
        
            prev_power = power;
            prev_voltage = voltage;
            mppt_enabled = 1;
            return;
        }
        
        // P&O Algorithm Implementation
        float delta_power = power - prev_power;
        float delta_voltage = voltage - prev_voltage;
        
        // Avoid division by zero
        if (delta_voltage == 0)
        {
            // If voltage hasn't changed, continue in same direction
            if (mppt_direction == 1)
            {
                set_duty_cycle(duty_cycle + DUTY_STEP);
            }
            else
            {
                set_duty_cycle(duty_cycle - DUTY_STEP);
            }
        }
        else
        {
            // Calculate dP/dV
            float dp_dv = delta_power / delta_voltage;
            
            if (dp_dv > 0)
            {
                // We're on the left side of MPP, increase voltage (decrease duty)
                if (delta_voltage > 0)
                {
                    // Voltage increased, decrease duty cycle
                    set_duty_cycle(duty_cycle - DUTY_STEP);
                    mppt_direction = 0;
                }
                else
                {
                    // Voltage decreased, increase duty cycle
                    set_duty_cycle(duty_cycle + DUTY_STEP);
                    mppt_direction = 1;
                }
            }
            else if (dp_dv < 0)
            {
                // We're on the right side of MPP, decrease voltage (increase duty)
                if (delta_voltage > 0)
                {
                    // Voltage increased, increase duty cycle
                    set_duty_cycle(duty_cycle + DUTY_STEP);
                    mppt_direction = 1;
                }
                else
                {
                    // Voltage decreased, decrease duty cycle
                    set_duty_cycle(duty_cycle - DUTY_STEP);
                    mppt_direction = 0;
                }
            }
            // If dP/dV = 0, we're at MPP, don't change duty cycle
        }
        
    
        prev_power = power;
        prev_voltage = voltage;
        
    
        if (power > 0.1f)  
        {
            P1OUT ^= BIT0;  
        }
    }
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

            if (current_channel == 0) // A10 - Voltage sensor
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
                    
                    // Ensure voltage is not negative
                    if (voltage < 0) voltage = 0;
                }

                current_channel = 1; // Switch to next channel (A7)
            }
            else if (current_channel == 1) // A7 - Current sensor
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
                    current = (sensor2 - 1.653f) / 0.05f;  // Convert to current in amperes
                    
                    // Ensure current is not negative
                    if (current < 0) current = 0;
                }

                current_channel = 0; // Switch back to A10
            }

            __bic_SR_register_on_exit(LPM0_bits);
            break;
        }
        default: break;
    }
}

