// File       : adc.c
// Author     : Jeff Schornick
//
// ADC setup
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#define ADC_MAX 0x3FFF  /* 14-bit mode */
#define ADC_REF 3.3     /* AVCC = VCC = 3.3V */

volatile uint8_t adc_flag = 0;   // adc complete flag, set in ADC ISR
volatile uint16_t adc_val = 0;   // adc reading, set in ADC ISR

// Function: adc_initializes
//
// Initializes the ADC in 14-bit single shot mode, reading from external pin A1.
//
// NOTE: Initialization based on TI Resource explorer example mps432p401x_adc14_01
void adc_init(void)
{
  // CTL0_SHP : sample-and-hold: pulse source = sampling timer
  // CTL0_SHT0: sample-and-hold: SHT0_3 = 32 ADC clock cycles
  // CTL0_ON  : ADC will be powered during conversion
  ADC14->CTL0 = ADC14_CTL0_SHT0_3 | ADC14_CTL0_SHP | ADC14_CTL0_ON;

  ADC14->CTL1 = ADC14_CTL1_RES_3;        // 14-bit conversion results

  ADC14->MCTL[0] |= ADC14_MCTLN_INCH_1;  // MEM[0] input = A1, AVCC/AVSS refs
  ADC14->IER0 |= ADC14_IER0_IE0;         // Interrupt on conversion complete

  __NVIC_EnableIRQ(ADC14_IRQn);
}

// ADC14 interrupt service routine
//
// Read the ADC value, set a flag for the main loop to print the result
void ADC14_IRQHandler(void)
{
  adc_val = ADC14->MEM[0];  // read and clear interrupt
  adc_flag = 1;
}
