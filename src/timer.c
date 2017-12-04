// File       : timer.c
// Author     : Jeff Schornick
//
// Timer setup
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

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


// Interrupt handler for timer compare TA1CCR0 (blinker)
void TA1_0_IRQHandler(void)
{
  // reset timer interrupt flag
  TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

  heartbeat += 0x1000;  // intensity change
  // Rescale the linear heartbeat value to rise and fall
  TIMER_A0->CCR[2] = (heartbeat < 1<<15) ? (heartbeat >> 11) : (0xffff-heartbeat) >> 11;
}
