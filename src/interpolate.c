// File       : interpolate.c
// Author     : Jeff Schornick
//
// Step interpolation algorithms
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdlib.h>  // malloc
#include <math.h>  // sqrtl
#include "uart.h"
#include "tmc.h"
#include "motion.h"

void rapid_interpolate(int32_t *start_pos, int32_t *end_pos, motion_t *motion)
{
  int32_t dx = end_pos[X_AXIS] - start_pos[X_AXIS];
  int32_t dy = end_pos[Y_AXIS] - start_pos[Y_AXIS];
  int32_t dz = end_pos[Z_AXIS] - start_pos[Z_AXIS];
  uart_queue_str("\r\nRapid interpolate:\r\n");
  uart_queue_str("  (dx, dy, dz) = (");
  uart_queue_sdec(dx);
  uart_queue_str(", ");
  uart_queue_sdec(dy);
  uart_queue_str(", ");
  uart_queue_sdec(dz);
  uart_queue_str(")\r\n");

  // determine direction for each axis
  motion->dirs[X_AXIS] = (dx>0) ? TMC_FWD : TMC_REV;
  motion->dirs[Y_AXIS] = (dy>0) ? TMC_FWD : TMC_REV;
  motion->dirs[Z_AXIS] = (dz>0) ? TMC_FWD : TMC_REV;

  dx = abs(dx);
  dy = abs(dy);
  dz = abs(dz);

  uint32_t longest = (dx > dy) ?  ((dx > dz) ? dx : dz)  :  ((dy > dz) ? dy : dz);
  motion->steps = malloc( sizeof(step_timing_t) * longest);
  step_timing_t *steps = motion->steps;

  // stepper timer hits about every 30.5 us, calculate timer ticks per step at rapid rate
  // (1s * 1e6  / rate) / (30.5us)
  uint32_t step_ticks = (2000000 / rapid_rate) / 61;

  for( uint32_t i=0; i<longest; i++ ) {
    steps[i].x = (i < dx) ? 1 : 0;
    steps[i].y = (i < dy) ? 1 : 0;
    steps[i].z = (i < dz) ? 1 : 0;
    steps[i].x_flip = 0;
    steps[i].y_flip = 0;
    steps[i].z_flip = 0;
    steps[i].timer_ticks = step_ticks;
  }
  motion->count = longest;
}

// rate is in steps/s ??
void linear_interpolate(int32_t *start_pos, int32_t *end_pos, uint16_t rate, motion_t *motion)
{
  int32_t dx = end_pos[X_AXIS] - start_pos[X_AXIS];
  int32_t dy = end_pos[Y_AXIS] - start_pos[Y_AXIS];
  int32_t dz = end_pos[Z_AXIS] - start_pos[Z_AXIS];
  int32_t d = sqrtl(dx*dx + dy*dy + dz*dz);

  uart_queue_str("\r\nLinear interpolate:");
  uart_queue_str("\r\n  step/s = ");
  uart_queue_dec(rate);
  uart_queue_str("\r\n");
  uart_queue_str("  (dx, dy, dz) = (");
  uart_queue_sdec(dx);
  uart_queue_str(", ");
  uart_queue_sdec(dy);
  uart_queue_str(", ");
  uart_queue_sdec(dz);
  uart_queue_str(") = ");
  uart_queue_dec(d);

  uint32_t t_us = (1000000/rate) * d;    // move time in us
  uart_queue_str("\r\n  move time =  ");
  uart_queue_dec(t_us);
  uart_queue_str(" us\r\n");

  uint32_t max_x = (t_us * MAX_RATE)/1000000 + 1;
  uint32_t max_y = (t_us * MAX_RATE)/1000000 + 1;
  uint32_t max_z = (t_us * MAX_RATE)/1000000 + 1;

  if ((abs(dx) > max_x) || (abs(dy) > max_y) || (abs(dz) > max_z)) {
    uart_queue_str("  WARNING: Motion interpolates above max rate!\r\n");
  }

  // determine direction for each axis
  motion->dirs[X_AXIS] = (dx>0) ? TMC_FWD : TMC_REV;
  motion->dirs[Y_AXIS] = (dy>0) ? TMC_FWD : TMC_REV;
  motion->dirs[Z_AXIS] = (dz>0) ? TMC_FWD : TMC_REV;

  dx = abs(dx);
  dy = abs(dy);
  dz = abs(dz);

  // time between steps in us
  uint32_t x_dt = dx ? (t_us / dx) : -1;
  uint32_t y_dt = dy ? (t_us / dy) : -1;
  uint32_t z_dt = dz ? (t_us / dz) : -1;

  uart_queue_str("  dt = (");
  uart_queue_dec(x_dt);
  uart_queue_str(", ");
  uart_queue_dec(y_dt);
  uart_queue_str(", ");
  uart_queue_dec(z_dt);
  uart_queue_str(") us\r\n");

  // Interpolate

  uint32_t next_x = x_dt;
  uint32_t next_y = y_dt;
  uint32_t next_z = z_dt;

  motion->steps = malloc( sizeof(step_timing_t) * (dx+dy+dz) );  // worst case, no simultaneous steps
  step_timing_t *steps = motion->steps;

  uint32_t i = 0;
  uint32_t t = 0;

  uint32_t skip_ticks;
  while( (dx>0) || (dy>0) || (dz>0) ) {

    // see which axis steps soonest
    if (next_x < next_y) {
      // x or z
      if (next_x < next_z) {
        skip_ticks = (next_x - t) * 2 / 61 + 1;  // 61 us every 2 ticks
      } else {
        skip_ticks = (next_z - t) * 2 / 61 + 1;
      }
    } else {
      // y or z
      if (next_y < next_z) {
        skip_ticks = (next_y - t) * 2 / 61 + 1;
      } else {
        skip_ticks = (next_z - t) * 2 / 61 + 1;
      }
    }

    // default to no step
    steps[i].x = 0;
    steps[i].y = 0;
    steps[i].z = 0;
    steps[i].x_flip = 0;
    steps[i].y_flip = 0;
    steps[i].z_flip = 0;

    t += (61*skip_ticks) / 2;  // skip ahead ticks, 30.5us each
    /* uart_queue_str("t = "); */
    /* uart_queue_dec(t); */
    steps[i].timer_ticks = skip_ticks;
    if (t >= next_x) {
      /* uart_queue_str(" X "); */
      steps[i].x = 1;
      next_x += x_dt;
      dx--;
    }
    if (t >= next_y) {
      /* uart_queue_str("  Y"); */
      steps[i].y = 1;
      next_y += y_dt;
      dy--;
    }
    if (t >= next_z) {
      /* uart_queue_str("  Y"); */
      steps[i].z = 1;
      next_z += z_dt;
      dz--;
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


//
void arc_interpolate(int32_t *start_pos, int32_t *end_pos, int32_t x_off, int32_t y_off, uint8_t rot, uint16_t rate, motion_t *motion)
{

  /* int32_t dx = end_pos[X_AXIS] - start_pos[X_AXIS]; */
  /* int32_t dy = end_pos[Y_AXIS] - start_pos[Y_AXIS]; */

  int32_t r = sqrtl(x_off*x_off + y_off*y_off);

  int32_t x0 = x_off;
  int32_t y0 = y_off;

  uart_queue_str("\r\nArc interpolate:");
  /* uart_queue_str("\r\n  step/s = "); */
  /* uart_queue_dec(rate); */
  /* uart_queue_str("\r\n"); */
  /* uart_queue_str("  (dx, dy) = ("); */
  /* uart_queue_sdec(dx); */
  /* uart_queue_str(", "); */
  /* uart_queue_sdec(dy); */
  /* uart_queue_str(")\r\n"); */

  uart_queue_str("  (x0, y0) = (");
  uart_queue_sdec(x0);
  uart_queue_str(", ");
  uart_queue_sdec(y0);
  uart_queue_str(")\r\n");

  uart_queue_str("  r = ");
  uart_queue_dec(r);
  uart_queue_str("\r\n");

  /* int32_t dx = end_pos[X_AXIS] - start_pos[X_AXIS]; */
  /* int32_t dy = end_pos[Y_AXIS] - start_pos[Y_AXIS]; */

  // counter clockwise arc with x_off<0, y_off=0
  //    arc goes up and left
  motion->dirs[X_AXIS] = TMC_REV;
  motion->dirs[Y_AXIS] = TMC_FWD;

  // TODO: this should either vary during motion, or at least be based on arc length
  uint32_t step_ticks = (2000000 / rate) / 61;

  //motion->steps = malloc( sizeof(step_timing_t) * (abs(dx)+abs(dy)) );  // worst case
  motion->steps = malloc( sizeof(step_timing_t) * 8*r );

  #include <string.h>
  memset(motion->steps, 0, sizeof(step_timing_t) * 8*r);

  step_timing_t *steps = motion->steps;
  uint32_t i = 0;


  uart_flush();

  char maj_axis = 'Y'; // axis that changes every
  int8_t maj_pol = 1;
  int8_t min_pol = -1;
  int8_t min_stepped;

  for( uint8_t octant = 0; octant<8; octant++ ) {

    int32_t maj_steps = 0;
    int32_t min_steps = 0;
    int32_t maj_err = 1;
    int32_t min_err = 1;
    int32_t err = 1 - (2*r);

    uart_queue_str("Octant: "); uart_queue_dec(octant); uart_queue_str("\r\n"); uart_queue_str("Maj: "); uart_queue(maj_axis); uart_queue_str("\r\n");

    // r- minor >= major
    while (r > maj_steps + min_steps) {
      steps[i].timer_ticks = step_ticks;

      /* uart_queue_dec(i); uart_queue_str(": ("); uart_queue_dec(maj_steps); uart_queue_str(", "); uart_queue_dec(min_steps); uart_queue_str(") "); */

      if (err <= 0) {
        /* uart_queue_sdec(maj_pol); */
        if (maj_axis == 'X') {
          steps[i].x = 1;
          /* uart_queue_str("X "); */
        } else {
          steps[i].y = 1;
          /* uart_queue_str("Y "); */
        }
        maj_steps++;
        err += maj_err;
        maj_err +=2;
      }

      min_stepped = 0;
      if (err > 0) {
        min_stepped = 1;
        min_steps++;
        min_err += 2;
        err += min_err - (2*r);
      }

      // minor axis
      if (octant%2 == 0) {
        if (min_stepped) {
          /* uart_queue_sdec(min_pol); */
          if (maj_axis == 'X') {
            steps[i].y = 1;
            /* uart_queue_str("Y"); */
          } else {
            steps[i].x = 1;
            /* uart_queue_str("X"); */
          }
        }
      } else {
        if (!min_stepped) {
          /* uart_queue_sdec(min_pol); */
          if (maj_axis == 'X') {
            steps[i].y = 1;
            /* uart_queue_str("Y"); */
          } else {
            steps[i].x = 1;
            /* uart_queue_str("X"); */
          }
        }
      }

      /* uart_queue_str("Data: ["); */
      /* uart_queue_hex(steps[i].step_data,6); */
      /* uart_queue_str("]\r\n"); */

      /* uart_queue_str("\r\n"); */
      i++;
      /* uart_flush(); */
    }
    uart_queue_str("---\r\n");

    if ( (octant%2) == 0 ) {
      // beginning of odd octant
      maj_axis = (maj_axis == 'X') ? 'Y' : 'X';
      int8_t tmp = maj_pol;
      maj_pol = min_pol;
      min_pol = tmp;
    } else {
      // beginning of nonzero even octant
      min_pol *= -1;
      // flip minor axis polarity
      if (maj_axis == 'X') {
        steps[i].y_flip = 1;
        uart_queue_str("Y-flip: [");
        uart_queue_hex(steps[i].step_data,6);
        uart_queue_str("]\r\n");
      } else {
        steps[i].x_flip = 1;
        uart_queue_str("X-flip: [");
        uart_queue_hex(steps[i].step_data,6);
        uart_queue_str("]\r\n");
      }
    }


  }
  motion->count = i;

  int32_t x = start_pos[X_AXIS];
  int32_t y = start_pos[Y_AXIS];

  int8_t xmod = (motion->dirs[X_AXIS] == TMC_FWD) ? +1 : -1;
  int8_t ymod = (motion->dirs[Y_AXIS] == TMC_FWD) ? +1 : -1;

  uart_queue_str("\r\n----\r\n");
  for( i = 0; i < motion->count; i++ ) {
    uart_queue_dec(i);
    uart_queue_str(": [");
    uart_queue_hex(steps[i].step_data,6);
    uart_queue_str("] ");
    /* uart_queue_str("tk = "); */
    /* uart_queue_dec(motion->steps[i].timer_ticks); */

    if(steps[i].x_flip) {
      xmod *= -1;
      uart_queue('*');
    }
    if (steps[i].x) {
      if (xmod == 1) {
        uart_queue_str(" +X");
      }else {
        uart_queue_str(" -X");
      }
      x += xmod;
    } else {
      uart_queue_str("   ");
    }

    if(steps[i].y_flip) {
      ymod *= -1;
      uart_queue('*');
    }
    if (steps[i].y) {
      if (ymod == 1) {
        uart_queue_str(" +Y");
      } else {
        uart_queue_str(" -Y");
      }
      y += ymod;
    }
    uart_queue_str("\r\n");
  }

  uart_queue_str("End: (");
  uart_queue_sdec(x);
  uart_queue_str(", ");
  uart_queue_sdec(y);
  uart_queue_str(")\r\n");
}
