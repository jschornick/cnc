// File       : interpolate.c
// Author     : Jeff Schornick
//
// Step interpolation algorithms
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details
//
// Attribution: Rasterized circle algorithm: https://en.wikipedia.org/wiki/Midpoint_circle_algorithm


#include <stdlib.h>  // malloc
#include <string.h>  // memset
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
void arc_interpolate(int32_t *start_pos, int32_t *end_pos, int32_t x_off, int32_t y_off, int8_t rot, uint16_t rate, motion_t *motion)
{

  int32_t dx = end_pos[X_AXIS] - start_pos[X_AXIS];
  int32_t dy = end_pos[Y_AXIS] - start_pos[Y_AXIS];

  int32_t r = sqrtl(x_off*x_off + y_off*y_off);

  int32_t x0 = x_off;
  int32_t y0 = y_off;

  uart_queue_str("\r\nArc interpolate:");
  uart_queue_str("\r\n  step/s = ");
  uart_queue_dec(rate);
  uart_queue_str("\r\n");
  uart_queue_str("  (dx, dy) = (");
  uart_queue_sdec(dx);
  uart_queue_str(", ");
  uart_queue_sdec(dy);
  uart_queue_str(")\r\n");

  uart_queue_str("  (x0, y0) = (");
  uart_queue_sdec(x0);
  uart_queue_str(", ");
  uart_queue_sdec(y0);
  uart_queue_str(")\r\n");
  uart_queue_str("  rot = ");
  uart_queue_sdec(rot);

  uart_queue_str("  r = ");
  uart_queue_dec(r);
  uart_queue_str("\r\n");


  // counter clockwise arc with x_off<0, y_off=0
  //    arc goes up and left

  int8_t start_oct, end_oct;

  int32_t end_x_off = x_off - dx;
  int32_t end_y_off = y_off - dy;

  if (x_off < 0) {
    if (y_off < 0) {
      start_oct = (abs(x_off) < abs(y_off)) ? 1 : 0;
    } else {
      start_oct = (abs(x_off) < abs(y_off)) ? 6 : 7;
    }
  } else {
    if (y_off < 0) {
      start_oct = (abs(x_off) < abs(y_off)) ? 2 : 3;
    } else {
      start_oct = (abs(x_off) < abs(y_off)) ? 5 : 4;
    }
  }

  if (end_x_off < 0) {
    if (end_y_off < 0) {
      end_oct = (abs(end_x_off) < abs(end_y_off)) ? 1 : 0;
    } else {
      end_oct = (abs(end_x_off) < abs(end_y_off)) ? 6 : 7;
    }
  } else {
    if (end_y_off < 0) {
      end_oct = (abs(end_x_off) < abs(end_y_off)) ? 2 : 3;
    } else {
      end_oct = (abs(end_x_off) < abs(end_y_off)) ? 5 : 4;
    }
  }
  uint8_t octants = ( (rot == 1)
                      ? (8 + end_oct - start_oct) % 8 + 1
                      : (8 + start_oct - end_oct) % 8 + 1 );

  uart_queue_str("\r\n  S_oct: ");
  uart_queue_sdec(start_oct);
  uart_queue_str("\r\n  E_oct: ");
  uart_queue_sdec(end_oct);
  uart_queue_str("\r\n  Octs = ");
  uart_queue_dec(octants);
  uart_queue_str("\r\n");

  int8_t oct2maj_axis[] = {'Y', 'X', 'X', 'Y', 'Y', 'X', 'X', 'Y'};
  int8_t oct2min_axis[] = {'X', 'Y', 'Y', 'X', 'X', 'Y', 'Y', 'X'};

  // G2 (CW)
  /* int8_t oct2maj_pol[] = {-1, 1, 1, 1, 1, -1, -1, -1}; */
  /* int8_t oct2min_pol[] = {1, -1, 1, 1, -1, 1, -1, -1}; */

  // G3 (CCW)
  int8_t oct2maj_pol[] = {1, -1, -1, -1, -1, 1, 1, 1};
  int8_t oct2min_pol[] = {-1, 1, -1, -1, 1, -1, 1, 1};

  // TODO: this should either vary during motion, or at least be based on arc length
  uint32_t step_ticks = (2000000 / rate) / 61;

  //motion->steps = malloc( sizeof(step_timing_t) * (abs(dx)+abs(dy)) );  // worst case
  motion->steps = malloc( sizeof(step_timing_t) * 8*r );
  memset(motion->steps, 0, sizeof(step_timing_t) * 8*r);

  step_timing_t *steps = motion->steps;
  uint32_t i = 0;


  uart_flush();

  char maj_axis = oct2maj_axis[start_oct];   // axis with a larger delta across the octant
  char min_axis;
  int8_t maj_pol = rot * oct2maj_pol[start_oct];  // CCW --> increasing octants
  int8_t min_pol = rot * oct2min_pol[start_oct];
  /* char maj_axis = 'Y'; // axis that changes every */
  /* int8_t maj_pol = 1; */
  /* int8_t min_pol = -1; */

  // starting direction for motion based on octant and rotation direction
  if (maj_axis == 'X') {
    motion->dirs[X_AXIS] = (maj_pol == 1) ? TMC_FWD : TMC_REV;
    motion->dirs[Y_AXIS] = (min_pol == 1) ? TMC_FWD : TMC_REV;
  } else {
    motion->dirs[Y_AXIS] = (maj_pol == 1) ? TMC_FWD : TMC_REV;
    motion->dirs[X_AXIS] = (min_pol == 1) ? TMC_FWD : TMC_REV;
  }


  //for( uint8_t octant = 0; octant<8; octant++ ) {
  for( uint8_t oct_i = 0; oct_i < octants; oct_i++) {
    uint8_t octant = (8+start_oct + (rot*oct_i)) % 8;

    maj_axis = oct2maj_axis[octant];
    min_axis = oct2min_axis[octant];
    maj_pol = rot * oct2maj_pol[octant];
    min_pol = rot * oct2min_pol[octant];

    int32_t min_pos = r - 1;
    int32_t maj_pos = 0;
    int32_t maj_err_mod = 1;
    int32_t min_err_mod = 1;
    int32_t err = 1 - (2*r);
    int8_t do_min_step;

    uart_queue_str("Octant: "); uart_queue_dec(octant); uart_queue_str(" Maj: "); uart_queue(maj_axis);
    uart_queue_str(" Pols: "); uart_queue_sdec(maj_pol); uart_queue_str(", "); uart_queue_sdec(min_pol); uart_queue_str("\r\n");

    // r- minor >= major
    while (min_pos >= maj_pos) {
      steps[i].timer_ticks = step_ticks;

      /* uart_queue_dec(i); uart_queue_str(": ("); uart_queue_dec(maj_steps); uart_queue_str(", "); uart_queue_dec(min_steps); uart_queue_str(") "); */
      uart_queue_str("Pt: ");
      if (maj_axis == 'X') {
        uart_queue_sdec(maj_pos); uart_queue_str(", "); uart_queue_sdec(min_pos);
      } else {
        uart_queue_sdec(min_pos); uart_queue_str(", "); uart_queue_sdec(maj_pos);
      }
      uart_queue_str("\r\n");

      // determine if we should step the major (fast changing) axis
      if (err <= 0) {
        /* uart_queue_sdec(maj_pol); */
        if (maj_axis == 'X') {
          steps[i].x = 1;
          /* uart_queue_str("X "); */
        } else {
          steps[i].y = 1;
          /* uart_queue_str("Y "); */
        }

        maj_pos++;
        err += maj_err_mod;
        maj_err_mod +=2;
      }

      // determine if we should step the minor access
      do_min_step = 0;
      if (err > 0) {
        do_min_step = 1;
        min_pos--;
        min_err_mod += 2;
        err += min_err_mod - (2*r);
      }

      // adjacent quadrants have mirrored minor axis step rates
      if ( (octant % 2) == (rot == -1)) { // normal behavior
        if (do_min_step) {
          /* uart_queue_sdec(min_pol); */
          if (min_axis == 'Y') {
            steps[i].y = 1;
            /* uart_queue_str("Y"); */
          } else {
            steps[i].x = 1;
            /* uart_queue_str("X"); */
          }
        }
      } else { // mirrored behavior
        if (!do_min_step) {
          /* uart_queue_sdec(min_pol); */
          if (min_axis == 'Y') {
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
    /* uart_queue_str("---\r\n"); */

    // axis polarity only changes when it is the minor axis
    // if there's a change coming, flip axis direction

    // polarity of minor access flips when
    //     CW  (-1)  goes even -> odd
    //     CCW (+1)  goes odd -> even
    if( ((octant%2 == 0) && (rot == -1)) ||
        ((octant%2 == 1) && (rot == 1)) ) {
      if (min_axis == 'X') {
        steps[i].x_flip = 1;
      } else {
        steps[i].y_flip = 1;
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
    /* uart_queue_str(": ["); */
    /* uart_queue_hex(steps[i].step_data,6); */
    /* uart_queue_str("] "); */
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
