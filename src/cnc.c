// File       : cnc.c
// Author     : Jeff Schornick
//
// MSP432 CNC controller
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include <stdlib.h>
#include "msp432p401r.h"
#include "uart.h"
#include "spi.h"
#include "timer.h"
#include "tmc.h"
#include "gpio.h"
#include "buttons.h"
#include "menu.h"

#include "motion.h"

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

/* motion_t sample_motion1; */
/* motion_t sample_motion2; */

int main(void) {

  // clocks/watchdog configured via SystemInit() on reset (see: system_msp432p401r.c)

  gpio_set_output(LED1);
  gpio_high(LED1);

  button_init();
  uart_init();
  spi_init();

  timer_init();

  tmc = 0;

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


  motion = new_motion(50, 50, 100, 100);
  next_motion = new_motion(-50, 50, 50, 101);

  char new_char;
  input_state = INPUT_MENU;
  display_main_menu(2);

  while(1) {
    if(B1_flag) {
      uart_queue_str("\r\nB1 : E-stop!\r\n");
      // disable all mosfets
      for(uint8_t i = 0; i < 3; i++) {
        gpio_high(tmc_pins[i].en_port, tmc_pins[i].en_pin);
      }
      B1_flag=0;
    }
    if(B2_flag) {
      uart_queue_str("\r\nB2 : E-stop\r\n");
      for(uint8_t i = 0; i < 3; i++) {
        gpio_high(tmc_pins[i].en_port, tmc_pins[i].en_pin);
      }
      B2_flag=0;
    }

    // Pop received characters off the FIFO and process them
    while(rx_fifo.count) {
      fifo_pop(&rx_fifo, &new_char);
      process_input(new_char);
    }

    gpio_low(LED1);
    __sleep();
    gpio_high(LED1);

  };
}

