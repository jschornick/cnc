// File       : timer.c
// Author     : Jeff Schornick
//
// Timer setup
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include "msp432p401r.h"

#include "motion.h"
#include "tmc.h"

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
  // TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;

  //TIMER_A1->CCR[0] = 32767;  // once per second
  //TIMER_A1->CCR[0] = 3277;  // ~10 Hz
  TIMER_A1->CCR[0] = 1000;  // full steps ok

  // enable interrupt associated with CCR0 match
  __NVIC_EnableIRQ(TA1_0_IRQn);
}


void step_timer_on(void) {
  TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;
}

void step_timer_period(uint16_t period) {
  TIMER_A1->CCR[0] = period;
}

// Interrupt handler for timer compare TA1CCR0 (blinker)
void TA1_0_IRQHandler(void)
{
  // reset timer interrupt flag
  TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

  uint8_t nochanges = 0;
  for (uint8_t axis=0; axis<3; axis++) {
    if (pos[axis] < target[axis]) {
      tmc_set_dir(axis, TMC_FWD);
      gpio_toggle(tmc_pins[axis].step_port, tmc_pins[axis].step_pin);
      pos[axis]++;
    } else if (pos[axis] > target[axis]) {
      tmc_set_dir(axis, TMC_REV);
      gpio_toggle(tmc_pins[axis].step_port, tmc_pins[axis].step_pin);
      pos[axis]--;
    } else {
      nochanges++;
    }
  }

  if (nochanges == 3) {
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIE;
  }

  // step if not at target
  /* if (x_pos < x_target) { */
  /*   tmc_set_dir(X_AXIS, TMC_FWD); */
  /*   /\* gpio_set(tmc_pins[X_AXIS].dir_port, tmc_pins[X_AXIS].dir_pin, X_AXIS_POS); *\/ */
  /*   gpio_toggle(tmc_pins[X_AXIS].step_port, tmc_pins[X_AXIS].step_pin); */
  /*   x_pos++; */
  /* } else if (x_pos > x_target) { */
  /*   tmc_set_dir(X_AXIS, TMC_REV); */
  /*   /\* gpio_set(tmc_pins[X_AXIS].dir_port, tmc_pins[X_AXIS].dir_pin, !X_AXIS_POS); *\/ */
  /*   gpio_toggle(tmc_pins[X_AXIS].step_port, tmc_pins[X_AXIS].step_pin); */
  /*   x_pos--; */
  /* } */

  /* if (y_pos < y_target) { */
  /*   tmc_set_dir(Y_AXIS, TMC_FWD); */
  /*   /\* gpio_set(tmc_pins[Y_AXIS].dir_port, tmc_pins[Y_AXIS].dir_pin, Y_AXIS_POS); *\/ */
  /*   gpio_toggle(tmc_pins[Y_AXIS].step_port, tmc_pins[Y_AXIS].step_pin); */
  /*   y_pos++; */
  /* } else if (y_pos > y_target) { */
  /*   tmc_set_dir(Y_AXIS, TMC_REV); */
  /*   /\* gpio_set(tmc_pins[Y_AXIS].dir_port, tmc_pins[Y_AXIS].dir_pin, !Y_AXIS_POS); *\/ */
  /*   gpio_toggle(tmc_pins[Y_AXIS].step_port, tmc_pins[Y_AXIS].step_pin); */
  /*   y_pos--; */
  /* } */

  /* if (z_pos < z_target) { */
  /*   tmc_set_dir(Z_AXIS, TMC_FWD); */
  /*   /\* gpio_set(tmc_pins[Z_AXIS].dir_port, tmc_pins[Z_AXIS].dir_pin, Z_AXIS_POS); *\/ */
  /*   gpio_toggle(tmc_pins[Z_AXIS].step_port, tmc_pins[Z_AXIS].step_pin); */
  /*   z_pos++; */
  /* } else if (z_pos > z_target) { */
  /*   tmc_set_dir(Z_AXIS, TMC_REV); */
  /*   /\* gpio_set(tmc_pins[Z_AXIS].dir_port, tmc_pins[Z_AXIS].dir_pin, !Z_AXIS_POS); *\/ */
  /*   gpio_toggle(tmc_pins[Z_AXIS].step_port, tmc_pins[Z_AXIS].step_pin); */
  /*   z_pos--; */
  /* } */

  /* if( (x_pos == x_target) && (y_pos == y_target) && (z_pos == z_target) ) { */
  /*   TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIE; */
  /* } */
}
