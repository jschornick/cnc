// File       : motion.c
// Author     : Jeff Schornick
//
// Motion functions
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include <stdlib.h>  // malloc
#include "uart.h"
#include "timer.h"
#include "tmc.h"
#include "interpolate.h"
#include "motion.h"

volatile int32_t pos[] = {0, 0, 0} ;
uint32_t rapid_rate = 300;

motion_t *motion = 0;
motion_t *next_motion = 0;
uint32_t motion_tick = 0;
uint32_t motion_enabled = 0;

void rapid(uint8_t tmc, int32_t steps)
{
  int32_t xyz[] = {0, 0, 0};
  uart_queue_str("Rapid : ");
  uart_queue_dec(steps);
  /* if(tmc_get_dir(tmc) == TMC_FWD) { */
  /*   xyz[tmc] = steps; */
  /* } else { */
  /*   xyz[tmc] = -steps; */
  /* } */
  xyz[tmc] = steps;
  uart_queue_str(" steps\r\n");
  if (!next_motion) {
    next_motion = new_rapid_motion(xyz[X_AXIS], xyz[Y_AXIS], xyz[Z_AXIS], 1);
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
    //next_motion = new_linear_motion(-pos[X_AXIS], -pos[Y_AXIS], -pos[Z_AXIS], rapid_rate, 0);
    next_motion = new_rapid_motion(-pos[X_AXIS], -pos[Y_AXIS], -pos[Z_AXIS], 0);
    motion_start();
  } else {
    uart_queue_str("Motion queue full!\r\n");
  }
}

void motion_start(void)
{
  if(!motion_enabled) {
    uart_queue_str("Motion enabled\r\n");
  }
  /* TIMER_A1->CCR[0] = 1; */
  motion_enabled = 1;
  /* TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE; */
  step_timer_on();
}

void motion_stop(void)
{
  motion_enabled = 0;
  uart_queue_str("Motion disabled\r\n");
}


motion_t * new_rapid_motion(int32_t x, int32_t y, int32_t z, uint16_t id)
{
  int32_t start[3];
  int32_t end[3];

  start[X_AXIS] = 0;
  start[Y_AXIS] = 0;
  start[Z_AXIS] = 0;
  end[X_AXIS] = x;
  end[Y_AXIS] = y;
  end[Z_AXIS] = z;

  motion_t *motion = malloc( sizeof(motion_t) );
  motion->id = id;

  rapid_interpolate(start, end, motion);
  return motion;
}

motion_t * new_linear_motion(int32_t x, int32_t y, int32_t z, uint16_t speed, uint16_t id)
{
  int32_t start[3];
  int32_t end[3];

  start[X_AXIS] = 0;
  start[Y_AXIS] = 0;
  start[Z_AXIS] = 0;
  end[X_AXIS] = x;
  end[Y_AXIS] = y;
  end[Z_AXIS] = z;

  motion_t *motion = malloc( sizeof(motion_t) );
  motion->id = id;

  linear_interpolate(start, end, speed, motion);
  return motion;
}

motion_t *new_arc_motion(int32_t x, int32_t y, int32_t x_off, int32_t y_off, uint8_t rotation, uint16_t speed, uint16_t id)
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

  arc_interpolate(start, end, x_off, y_off, rotation, speed, motion);
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

