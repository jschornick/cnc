// File       : cnc.c
// Author     : Jeff Schornick
//
// MSP432 CNC controller
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include "msp432p401r.h"
#include "uart.h"
#include "spi.h"
#include "tmc.h"
#include "gpio.h"



// MSP-EXP432 board layout
//
// P1.0 : LED1+
// P2.0 : LED2_RED+
// P2.1 : LED2_GREEN+
// P2.2 : LED2_BLUE+
//
// P1.1 : S1 : N-O, closed->GND
// P1.4 : S2 : N-O, closed->GND

// Function: timer_init
//
// Initializes two timers, one fast, one slow.
// The fast timer (A0) is configured with a short period to be used for PWM.
// The slow timer (A1) is configured to interrupt every 1 second.
void timer_init(void)
{
  // ACLK = auxillary clock, set to LFXT = 32768 KHz
  // SMCLK =  DCO freq (3MHz)

  // Timer A0, fast clock
  //----------------------

  // Up counting mode (0 -> CCR0)
  TIMER_A0->CTL = TIMER_A_CTL_MC__UP | TIMER_A_CTL_SSEL__SMCLK | TIMER_A_CTL_CLR;

  // Set a short period (in timer ticks) for PWM
  //   ...then the duty cycle can be set as a fraction of this value in CCR[1..3]
  TIMER_A0->CCR[0] = 200;


  // Timer A1, slow clock
  //---------------------

  // Up counting mode (0 -> CCR0)
  TIMER_A1->CTL = TIMER_A_CTL_MC__UP | TIMER_A_CTL_SSEL__ACLK | TIMER_A_CTL_CLR;

  // capture control interrupt enable (CCIFG set when TA0R counts to CCR0)
  TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;
  //TIMER_A1->CCR[0] = 32767;  // once per second for blink
  TIMER_A1->CCR[0] = 3277;  // ~10 Hz

  // enable interrupt associated with CCR0 match
  __NVIC_EnableIRQ(TA1_0_IRQn);
}

// Function: button_init
//
// Initializes GPIOs attached to the two hardware buttons on the MSP432
// board to trigger an interupt when either button is pressed.
void button_init(void)
{
  P1->DIR &= ~(BIT1 | BIT4); // input
  P1->OUT |= BIT1 | BIT4; // pull up (vs pull down)
  P1->REN |= BIT1 | BIT4; // enable pulling (up)
  P1->IES |= BIT1 | BIT4; // high-to-low edge select

  P1->IFG &= ~(BIT1 | BIT4);
  P1->IE |= BIT1 | BIT4;  // interrupt enabled

  __NVIC_EnableIRQ(PORT1_IRQn);
}


// Function: pwm_init
//
// Initializes the fast timer (A0) to produce PWM signals for the RGB LED.
// Since the LED is not attached to the default outputs, these are remapped
// to pins 2.0-2.2.
void pwm_init(void)
{
  // Remap three of the TA0 outputs to the RGB LED
  PMAP->KEYID = PMAP_KEYID_VAL;
  PMAP->CTL = PMAP_CTL_PRECFG;
  P2MAP->PMAP_REGISTER0 = PMAP_TA0CCR1A;
  P2MAP->PMAP_REGISTER1 = PMAP_TA0CCR2A;
  P2MAP->PMAP_REGISTER2 = PMAP_TA0CCR3A;

  // Select the mapped (timer) module as the GPIO output for each
  P2->SEL0 |= BIT0 | BIT1 | BIT2;
  P2->SEL1 &= ~(BIT0 | BIT1 |BIT2);
  P2->DIR |= BIT0 | BIT1 | BIT2;

  // Use set/reset mode to define a duty cycle for each color
  TIMER_A0->CCTL[1] = TIMER_A_CCTLN_OUTMOD_7;
  TIMER_A0->CCTL[2] = TIMER_A_CCTLN_OUTMOD_7;
  TIMER_A0->CCTL[3] = TIMER_A_CCTLN_OUTMOD_7;
}


#define ADC_MAX 0x3FFF  /* 14-bit mode */
#define ADC_REF 3.3     /* AVCC = VCC = 3.3V */

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


// Button flags that may be set during GPIO interrupt
volatile uint8_t S1_FLAG;
volatile uint8_t S2_FLAG;

volatile uint16_t heartbeat = 0; // pulsing heartbeat, updated by timer ISR

volatile uint8_t adc_flag = 0;   // adc complete flag, set in ADC ISR
volatile uint16_t adc_val = 0;   // adc reading, set in ADC ISR

#define RD_USTEP 00
#define RD_SG    01
#define RD_SG_CS 10
#define RD_NONE  11

uint8_t RDSEL = RD_NONE;

//void decode_response(uint32_t data)
void display_response(uint32_t ustep_data, uint32_t sg_data, uint32_t sgcs_data)
{

  ustep_resp_t * ustep_resp = (ustep_resp_t *) &ustep_data;
  sg_resp_t * sg_resp = (sg_resp_t *) &sg_data;
  sgcs_resp_t * sgcs_resp = (sgcs_resp_t *) &sgcs_data;

  uart_queue_str("\r\nTMC response:\r\n");
  uart_queue_str("-------------\r\n");

  uart_queue_str("Raw uStep : ");
  uart_queue_hex(ustep_data, 20);
  uart_queue_str("\r\n");
  uart_queue_str("Raw SG    : ");
  uart_queue_hex(sg_data, 20);
  uart_queue_str("\r\n");
  uart_queue_str("Raw SGCS  : ");
  uart_queue_hex(sgcs_data, 20);
  uart_queue_str("\r\n");

  uart_queue_str("(00) StallGuard status   : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->SG) );
  uart_queue_str("\r\n");
  uart_queue_str("(01) Over temp shutdown  : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->OT) );
  uart_queue_str("\r\n");
  uart_queue_str("(02) Over temp warning   : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->OTPW) );
  uart_queue_str("\r\n");
  uart_queue_str("(03) Short to ground (A) : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->S2GA) );
  uart_queue_str("\r\n");
  uart_queue_str("(04) Short to ground (B) : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->S2GB) );
  uart_queue_str("\r\n");
  uart_queue_str("(05) Open load (A)       : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->OLA) );
  uart_queue_str("\r\n");
  uart_queue_str("(06) Open load (B)       : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->OLB) );
  uart_queue_str("\r\n");
  uart_queue_str("(07) Standstill          : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->STST) );
  uart_queue_str("\r\n");

  uart_queue_str("     MSTEP               : ");
  if(ustep_resp->POL) {
    uart_queue('+');
  } else {
    uart_queue('-');
  }
  uart_queue_hex(ustep_resp->MSTEP, 9);
  uart_queue_str("\r\n");

  uart_queue_str("     SG2                 : ");
  uart_queue_hex(sg_resp->SG10, 10);
  uart_queue_str("\r\n");

  uart_queue_str("     CS                  : ");
  uart_queue_hex(sgcs_resp->SE5, 5);
  uart_queue_str("\r\n");
}

typedef enum {
  INPUT_MENU = 0,
  INPUT_VALUE
} Input_State_t;

Input_State_t input_state;

uint8_t tmc = 0;

void do_menu(char c)
{
  uint32_t resp_ustep;
  uint32_t resp_sg;
  uint32_t resp_sgcs;
  uint32_t drvconf;
  uint8_t i = 0;
  switch(c) {
  case '0':
    tmc = 0;
    uart_queue_str("Stepper 0 selected.\r\n");
    break;
  case '1':
    tmc = 1;
    uart_queue_str("Stepper 1 selected.\r\n");
    break;
  case '2':
    tmc = 2;
    uart_queue_str("Stepper 2 selected.\r\n");
    break;

  case 'r':
    uart_queue_str("Read\r\n");
    /* spi_send(tmc_drvconf|DRVCONF_RDSEL_USTEP); */
    /* resp_ustep = spi_send(tmc_drvconf|DRVCONF_RDSEL_SG); */
    /* resp_sg = spi_send(tmc_drvconf|DRVCONF_RDSEL_SGCS); */
    /* resp_sgcs = spi_send(tmc_drvconf|DRVCONF_RDSEL_USTEP); */

    drvconf = tmc_config[tmc].drvconf & ~DRVCONF_RDSEL_MASK;
    tmc_send(tmc, drvconf | DRVCONF_RDSEL_USTEP);
    resp_ustep = tmc_send(tmc, drvconf | DRVCONF_RDSEL_SG);
    resp_sg = tmc_send(tmc, drvconf | DRVCONF_RDSEL_SGCS);
    resp_sgcs = tmc_send(tmc, drvconf | DRVCONF_RDSEL_USTEP);
    display_response(resp_ustep, resp_sg, resp_sgcs);
    break;
  case 's':
    uart_queue_str("Step\r\n");
    gpio_toggle(tmc_pins[tmc].step_port, tmc_pins[tmc].step_pin);
    break;
  case '<':
    uart_queue_str("Direction <\r\n");
    gpio_set_low(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
    break;
  case '>':
    uart_queue_str("Direction >\r\n");
    gpio_set_high(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
    break;
  case 'e':
    for(i = 0; i < 3; i++) {
      gpio_set_low(tmc_pins[i].en_port, tmc_pins[i].en_pin);
    }
    uart_queue_str("All MOSFETs enabled\r\n");
    break;
  case 'd':
    for(i = 0; i < 3; i++) {
      gpio_set_high(tmc_pins[i].en_port, tmc_pins[i].en_pin);
    }
    uart_queue_str("All MOSFETs disabled\r\n");
    break;
  case 'i':
    uart_queue_str("Re-initalizing TMCs\r\n");
    tmc_init();
    break;
  default:
    uart_queue(c);
  }
}

void process_char(char c)
{
  switch(input_state) {
    case INPUT_MENU:
      do_menu(c);
      break;
    case INPUT_VALUE:
      break;
  }
}


int main(void) {

  // clocks/watchdog configured via SystemInit() on reset (see: system_msp432p401r.c)

  char c;

  P1->DIR |= BIT0; // set LED1 pin as output
  P1->OUT |= BIT0; // turn on immediately

  P4->DIR |= BIT3; // step pin
  P4->DIR |= BIT4; // dir pin

  P4->DIR |= BIT2; // MOSFET enable pin (active low)
  P4->OUT |= BIT2; // set high, disable MOSFETS at start


  //timer_init();
  button_init();
  //pwm_init();
  uart_init();
  //adc_init();
  //vref_init();

  spi_init();

  __enable_irq();

  // Wake up on exit from ISR
  SCB->SCR &= ~SCB_SCR_SLEEPONEXIT_Msk;

  uart_queue_str("\r\n");
  uart_queue_str("------------------\r\n");
  uart_queue_str("| CNC Controller |\r\n");
  uart_queue_str("------------------\r\n\r\n");

  uart_queue_str("Initializing steppers... ");
  tmc_init();
  uart_queue_str("done!\r\n");


  S1_FLAG = 0;
  S2_FLAG = 0;

  input_state = INPUT_MENU;

  while(1) {
    if(S1_FLAG) {
      uart_queue_str("\r\nB1\r\n");
      RDSEL = RD_USTEP;
      S1_FLAG=0;
    }
    if(S2_FLAG) {
      uart_queue_str("\r\nB2: Step\r\n");
      P4->OUT ^= BIT3; // toggle to step
      S2_FLAG=0;
    }

    // Pop received characters off the FIFO and echo back
    while(rx_fifo.count) {
      fifo_pop(&rx_fifo, &c);
      process_char(c);
    }

    P1->OUT &= ~BIT0; // turn off
    __sleep();
    P1->OUT |= BIT0; // turn on

  };
}


// Interrupt handler for timer compare TA1CCR0 (blinker)
void TA1_0_IRQHandler(void)
{
  // reset timer interrupt flag
  TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

  heartbeat += 0x1000;  // intensity change
  // Rescale the linear heartbeat value to rise and fall
  TIMER_A0->CCR[2] = (heartbeat < 1<<15) ? (heartbeat >> 11) : (0xffff-heartbeat) >> 11;
}

// Interrupt handler for Port 1 (buttons)
void PORT1_IRQHandler(void)
{
  // check if it was S1 or S2
  if (P1->IFG & BIT1) {
    P1->IFG &= ~BIT1; // clear interrupt flag
    S1_FLAG = 1;
  }
  if (P1->IFG & BIT4) {
    P1->IFG &= ~BIT4;
    S2_FLAG = 1;
  }
}


// ADC14 interrupt service routine
//
// Read the ADC value, set a flag for the main loop to print the result
void ADC14_IRQHandler(void)
{
  adc_val = ADC14->MEM[0];  // read and clear interrupt
  adc_flag = 1;
}
