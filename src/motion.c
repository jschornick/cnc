// File       : motion.c
// Author     : Jeff Schornick
//
// Motion functions
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include "uart.h"
#include "timer.h"
#include "motion.h"

volatile int32_t pos[] = {0, 0, 0} ;
int32_t target[] = {0, 0, 0};

void rapid_x(uint32_t steps)
{
  uart_queue_str("Rapid X : ");
  uart_queue_dec(steps);
  uart_queue_str(" steps\r\n");

  target[X_AXIS] = pos[X_AXIS] + steps;
  step_timer_on();
}

void rapid_y(uint32_t steps)
{
  uart_queue_str("Rapid Y : ");
  uart_queue_dec(steps);
  uart_queue_str(" steps\r\n");

  target[Y_AXIS] = pos[Y_AXIS] + steps;
  step_timer_on();
}

void rapid_z(uint32_t steps)
{
  uart_queue_str("Rapid Z : ");
  uart_queue_dec(steps);
  uart_queue_str(" steps\r\n");

  target[Z_AXIS] = pos[Z_AXIS] + steps;
  step_timer_on();
}

void goto_pos(int32_t x, int32_t y, int32_t z)
{
  target[X_AXIS] = x;
  target[Y_AXIS] = y;
  target[Z_AXIS] = z;
  step_timer_on();
}

void home(void)
{
  target[X_AXIS] = 0;
  target[Y_AXIS] = 0;
  target[Z_AXIS] = 0;
  step_timer_on();
}

void rapid_speed(uint32_t speed)
{

  if (speed <= 1000) {
    // speed == 1 --> 32.77 step/s
    // speed == 500 --> 65.5 steps/s
    step_timer_period(1001 - speed);
    uart_queue_str("Rapid speed set to : ");
    uart_queue_dec(speed);
    uart_queue_str("\r\n");
  } else {
    uart_queue_str("Out of range!\r\n");
  }
}
