// File       : gpio.c
// Author     : Jeff Schornick
//
// MSP432 GPIO helper functions
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include "msp432p401r.h"

void gpio_set_high(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->OUT |= pin_bits;
}

void gpio_set_low(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->OUT &= ~pin_bits;
}

void gpio_toggle(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->OUT ^= pin_bits;
}


void gpio_set_input(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->DIR &= ~pin_bits;
}

void gpio_set_output(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->DIR |= pin_bits;
}

