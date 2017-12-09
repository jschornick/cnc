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
#include "tmc.h"

volatile int32_t pos[] = {0, 0, 0} ;
uint32_t max_rate = 300;

motion_t *motion = 0;
motion_t *next_motion = 0;
uint32_t motion_tick = 0;

void rapid(uint8_t tmc, uint32_t steps)
{
  int32_t xyz[] = {0, 0, 0};
  uart_queue_str("Rapid : ");
  uart_queue_dec(steps);
  if(tmc_get_dir(tmc) == TMC_FWD) {
    xyz[tmc] = steps;
  } else {
    xyz[tmc] = -steps;
  }
  uart_queue_str(" steps\r\n");
  if (!next_motion) {
    next_motion = new_motion(xyz[X_AXIS], xyz[Y_AXIS], max_rate, 7);
    motion_start();
  } else {
    uart_queue_str("Motion queue full!\r\n");
  }
}

void goto_pos(int32_t x, int32_t y, int32_t z)
{
  uart_queue_str("Goto... ");
  motion_start();
}

void home(void)
{
  if (!next_motion) {
    next_motion = new_motion(-pos[X_AXIS],-pos[Y_AXIS], max_rate, 42);
    motion_start();
  } else {
    uart_queue_str("Motion queue full!\r\n");
  }
}

void set_max_rate(uint32_t speed)
{
  max_rate = speed;
}

void motion_start(void)
{
  TIMER_A1->CCR[0] = 1;
  TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;
}

#include <math.h>  // sqrtl
#include <stdlib.h>  // malloc

// rate is in steps/s ??
void linear_interpolate_2d(int32_t *start_pos, int32_t *end_pos, uint16_t rate, motion_t *motion)
{
  int32_t dx = end_pos[X_AXIS] - start_pos[X_AXIS];
  int32_t dy = end_pos[Y_AXIS] - start_pos[Y_AXIS];
  int32_t d = sqrtl(dx*dx + dy*dy);

  uart_queue_str("\r\nstep/s = ");
  uart_queue_dec(rate);
  uart_queue_str("\r\n");
  uart_queue_str("(dx, dy) = (");
  uart_queue_sdec(dx);
  uart_queue_str(", ");
  uart_queue_sdec(dy);
  uart_queue_str(") = ");
  uart_queue_dec(d);

  uint32_t t_us = (1000000 /rate) * d;    // move time in us
  uart_queue_str("\r\nmove_time =  ");
  uart_queue_dec(t_us);
  uart_queue_str(" us\r\n");

  uint32_t max_x = (t_us * max_rate)/1000000 + 1;
  uint32_t max_y = (t_us * max_rate)/1000000 + 1;

  if ((abs(dx) > max_x) || (abs(dy) > max_y)) {
    uart_queue_str(" WARNING: Motion interpolates above max rate!\r\n");
  }

  // determine direction for each axis
  motion->dirs[X_AXIS] = (dx>0) ? TMC_FWD : TMC_REV;
  motion->dirs[Y_AXIS] = (dy>0) ? TMC_FWD : TMC_REV;

  dx = abs(dx);
  dy = abs(dy);

  // time between steps in us
  uint32_t x_dt = -1;
  uint32_t y_dt = -1;
  if (dx) {
    x_dt = t_us / dx;
  }

  if (dy) {
    y_dt = t_us / dy;
  }

  uart_queue_str("x_dt = ");
  uart_queue_dec(x_dt);
  uart_queue_str(" us\r\ny_dt = ");
  uart_queue_dec(y_dt);
  uart_queue_str(" us\r\n");

  // Interpolate

  uint32_t next_x = x_dt;
  uint32_t next_y = y_dt;

  motion->steps = malloc( sizeof(step_timing_t) * (dx+dy) );  // worst case, no simultaneous steps
  step_timing_t *steps = motion->steps;

  uint32_t i = 0;
  uint32_t t = 0;

  uint32_t skip_ticks;
  while( (dx>0) || (dy>0) ) {

    // see which axis steps soonest
    if (next_x < next_y) {
      skip_ticks = (next_x - t) * 2 / 61 + 1;  // 61 us every 2 ticks
    } else {
      /* uart_queue_str("Next y @t = "); */
      /* uart_queue_dec(next_y); */
      /* uart_queue_str(" us\r\n"); */
      skip_ticks = (next_y - t) * 2 / 61 + 1;
    }

    // default to no step
    steps[i].axes[X_AXIS] = 0;
    steps[i].axes[Y_AXIS] = 0;
    steps[i].axes[Z_AXIS] = 0;

    t += (61*skip_ticks) / 2;  // skip ahead ticks, 30.5us each
    /* uart_queue_str("t = "); */
    /* uart_queue_dec(t); */
    steps[i].timer_ticks = skip_ticks;
    if (t >= next_x) {
      /* uart_queue_str(" X "); */
      steps[i].axes[X_AXIS] = 1;
      next_x += x_dt;
      dx--;
    }
    if (t >= next_y) {
      /* uart_queue_str("  Y"); */
      steps[i].axes[Y_AXIS] = 1;
      next_y += y_dt;
      dy--;
    }
    /* uart_queue_str("\r\n"); */

    i++;
  }

  motion->count = i;

  /* uart_queue_str("\r\n----\r\n"); */
  /* for( i = 0; i < motion->count; i++ ) { */
  /*   uart_queue_str("tk = "); */
  /*   uart_queue_dec(motion->steps[i].timer_ticks); */
  /*   if (motion->steps[i].axes[X_AXIS]) { */
  /*     uart_queue_str(": X "); */
  /*   } else { */
  /*     uart_queue_str(":   "); */
  /*   } */
  /*   if (motion->steps[i].axes[Y_AXIS]) { */
  /*     uart_queue_str("Y "); */
  /*   } */
  /*   uart_queue_str("\r\n"); */
  /* } */

}

motion_t * new_motion(int32_t x, int32_t y, uint16_t speed, uint8_t id)
{
  int32_t start[3];
  int32_t end[3];

  start[X_AXIS] = 0;
  start[Y_AXIS] = 0;
  start[Z_AXIS] = 0;
  end[X_AXIS] = x;
  end[Y_AXIS] = y;
  end[Z_AXIS] = 0;

  motion_t *motion = malloc( sizeof(motion_t) );
  motion->id = id;

  linear_interpolate_2d(start, end, speed, motion);
  return motion;
}

void free_motion(motion_t *motion)
{
  if(motion) {
    if(motion->steps) {
      free(motion->steps);
    }
    free(motion);
  } else {
    uart_queue_str("\r\n !! Tried to free NULL !!\r\n");
  }
}

