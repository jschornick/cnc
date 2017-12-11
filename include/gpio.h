// File       : gpio.h
// Author     : Jeff Schornick
//
// MSP432 GPIO helper function declarations and macros
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#ifndef __GPIO_H
#define __GPIO_H

#include "msp432p401r.h"

#define PIN0 BIT0
#define PIN1 BIT1
#define PIN2 BIT2
#define PIN3 BIT3
#define PIN4 BIT4
#define PIN5 BIT5
#define PIN6 BIT6
#define PIN7 BIT7

#define GPIO_PULL_DOWN  0
#define GPIO_PULL_UP    1
#define GPIO_PULL_OFF  0
#define GPIO_PULL_ON   1

// interrupt edge select
#define GPIO_RISING    0
#define GPIO_FALLING   1

static inline uint8_t gpio_intr_flag(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  return (port->IFG & pin_bits);
}

static inline void gpio_intr_clear(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->IFG &= ~pin_bits;
}


static inline void gpio_high(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->OUT |= pin_bits;
}


static inline void gpio_low(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->OUT &= ~pin_bits;
}

static inline void gpio_toggle(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  port->OUT ^= pin_bits;
}

static inline uint8_t gpio_get_output(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits)
{
  return (port->OUT & pin_bits);
}


void gpio_set(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits, uint8_t val);

void gpio_toggle(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits);

void gpio_set_input(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits);

void gpio_set_output(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits);

void gpio_set_pullup(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits, uint8_t pull_dir);

void gpio_set_interrupt(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits, uint8_t edge);

void gpio_disable_interrupt(DIO_PORT_Odd_Interruptable_Type *port, uint8_t pin_bits);

#endif /* __GPIO_H */
