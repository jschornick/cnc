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

#define LED1_PORT    P1
#define LED1_PIN     PIN0
#define LED1 LED1_PORT, LED1_PIN

#define BUTTON1_PORT P1
#define BUTTON1_PIN  PIN1
#define BUTTON1 BUTTON1_PORT, BUTTON1_PIN

#define BUTTON2_PORT P1
#define BUTTON2_PIN  PIN4
#define BUTTON2 BUTTON2_PORT, BUTTON2_PIN

// Function: button_init
//
// Initializes GPIOs attached to the two hardware buttons on the MSP432
// board to trigger an interupt when either button is pressed.
void button_init(void)
{
  gpio_set_input(BUTTON1);
  gpio_set_input(BUTTON2);

  gpio_set_pullup(BUTTON1, GPIO_PULL_UP);
  gpio_set_pullup(BUTTON2, GPIO_PULL_UP);

  gpio_set_interrupt(BUTTON1, GPIO_FALLING);
  gpio_set_interrupt(BUTTON2, GPIO_FALLING);

  __NVIC_EnableIRQ(PORT1_IRQn);
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
  case 'S':
    uart_queue_str("Step All\r\n");
    gpio_toggle(tmc_pins[0].step_port, tmc_pins[0].step_pin);
    gpio_toggle(tmc_pins[1].step_port, tmc_pins[1].step_pin);
    gpio_toggle(tmc_pins[2].step_port, tmc_pins[2].step_pin);
    break;
  case '<':
    uart_queue_str("Direction <<--\r\n");
    gpio_low(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
    break;
  case '>':
    uart_queue_str("Direction -->>\r\n");
    gpio_high(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
    break;
  case 'e':
    for(i = 0; i < 3; i++) {
      gpio_low(tmc_pins[i].en_port, tmc_pins[i].en_pin);
    }
    uart_queue_str("All MOSFETs enabled\r\n");
    break;
  case 'd':
    for(i = 0; i < 3; i++) {
      gpio_high(tmc_pins[i].en_port, tmc_pins[i].en_pin);
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

  gpio_set_output(LED1);
  gpio_high(LED1);

  button_init();
  uart_init();
  spi_init();
  //timer_init();
  //pwm_init();
  //adc_init();

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
      S1_FLAG=0;
    }
    if(S2_FLAG) {
      uart_queue_str("\r\nB2\r\n");
      S2_FLAG=0;
    }

    // Pop received characters off the FIFO and echo back
    while(rx_fifo.count) {
      fifo_pop(&rx_fifo, &c);
      process_char(c);
    }

    gpio_low(LED1);
    __sleep();
    gpio_high(LED1);

  };
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


