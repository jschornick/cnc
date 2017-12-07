// File       : timer.h
// Author     : Jeff Schornick
//
// Timer setup
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#ifndef __TIMER_H
#define __TIMER_H

void timer_init(void);

void step_timer_on(void);
void step_timer_period(uint16_t);

#endif /* __TIMER_H */
