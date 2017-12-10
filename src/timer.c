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
  //TIMER_A1->CCR[0] = 1000;  //   ~32.77 Hz
  TIMER_A1->CCR[0] = 500;  //   ~65 Hz

  // enable interrupt associated with CCR0 match
  __NVIC_EnableIRQ(TA1_0_IRQn);
}


void step_timer_on(void) {
  TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;
}

void step_timer_period(uint16_t period) {
  TIMER_A1->CCR[0] = period;
}

#include "uart.h"

// Interrupt handler for timer compare TA1CCR0 (stepping)
// Max freq = 32768 kHz (evey ~30.5us)
void TA1_0_IRQHandler(void)
{
  TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIE;
  // reset timer interrupt flag
  TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

  if(motion) {
    if (motion_tick == 0) {
      uart_queue_str("Motion #");
      uart_queue_dec(motion->id);
      uart_queue_str(" started!\r\n");
      tmc_set_dir(X_AXIS, motion->dirs[X_AXIS]);
      tmc_set_dir(Y_AXIS, motion->dirs[Y_AXIS]);
      tmc_set_dir(Z_AXIS, motion->dirs[Z_AXIS]);
    }

    if(motion_tick < motion->count) {
      TIMER_A1->CCR[0] = motion->steps[motion_tick].timer_ticks;
      /* uart_queue_hex((uint32_t) motion->steps[motion_tick].axes[X_AXIS],4); */
      /* uart_queue_hex((uint32_t) motion->steps[motion_tick].axes[Y_AXIS],4); */
      /* uart_queue_str("\r\n"); */
      for (uint8_t axis=0; axis<3; axis++) {
        if (motion->steps[motion_tick].axes[axis]) {
          gpio_toggle(tmc_pins[axis].step_port, tmc_pins[axis].step_pin);
          pos[axis] += (tmc_get_dir(axis) == TMC_FWD) ? 1 : -1;
        }
      }
      motion_tick++;
      TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;
    } else {  // motion complete
      free_motion(motion);
      motion = 0;
      uart_queue_str("Motion complete!\r\n");
    }
  }

  // promote queued motion if available
  if(!motion) {
    if (next_motion) {
      uart_queue_str("Prepping queued motion!\r\n");
      motion = next_motion;
      next_motion = 0;
      motion_tick = 0;
      TIMER_A1->CCR[0] = motion->steps[motion_tick].timer_ticks;
      TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;
    } else {
      uart_queue_str("No queued motions!\r\n");
    }
  }

}
