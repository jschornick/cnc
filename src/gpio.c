// File       : gpio.c
// Author     : Jeff Schornick
//
// MSP432 GPIO helper functions
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include "msp432p401r.h"
#include "gpio.h"

void gpio_set(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits, uint8_t val)
{
  if (val) {
    port->OUT |= pin_bits;
  } else {
    port->OUT &= ~pin_bits;
  }
}

void gpio_set_input(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->DIR &= ~pin_bits;
}

void gpio_set_output(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->DIR |= pin_bits;
}

void gpio_set_pullup(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits, uint8_t pull_dir)
{
  if(pull_dir == GPIO_PULL_UP) {
    port->DIR |= pin_bits;
  } else {
    port->DIR &= ~pin_bits;
  }
  // enable pulling (for both up or down)
  port->REN |= pin_bits;
}

void gpio_disable_interrupt(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits) {
  port->IE &= ~pin_bits;
}

void gpio_set_interrupt(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits, uint8_t edge)
{
  if(edge == GPIO_FALLING) {
    port->IES |= pin_bits;
  } else {
    port->IES &= ~pin_bits;
  }

  port->IFG &= ~pin_bits; // clear flag
  port->IE |= pin_bits;   // interrupt enabled
}
