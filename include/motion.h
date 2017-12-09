// File       : motion.h
// Author     : Jeff Schornick
//
// Motion functions
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#ifndef __MOTION_H
#define __MOTION_H

#include <stdint.h>

// X-axis: carriage (along gantry)
// Y-axis: gantry
// Z-axis: up/down

#define X_AXIS 2  // 261
#define Y_AXIS 0  // strong 262
#define Z_AXIS 1  // weak 262

extern uint32_t max_rate;

typedef struct {
  uint16_t timer_ticks;
  uint8_t axes[3];
} step_timing_t;


// interpolated motion set
typedef struct motion_s {
  uint8_t id;
  uint8_t dirs[3];  // step direcitons per axis, set once per motion
  uint16_t count;   // total step changes
  step_timing_t *steps;  // malloc'd array of timestamped steps
} motion_t;

extern volatile int32_t pos[];
extern int32_t target[];

extern motion_t *motion;
extern motion_t *next_motion;
extern uint32_t motion_tick;

void rapid(uint8_t tmc, uint32_t steps);

void linear_interpolate_2d(int32_t *start_pos, int32_t *end_pos, uint16_t rate, motion_t *motion);

motion_t *new_motion(int32_t x, int32_t y, uint16_t speed, uint8_t id);
void free_motion(motion_t *);
void motion_start(void);

void goto_pos(int32_t x, int32_t y, int32_t z);

void set_max_rate(uint32_t speed);
void home(void);


#endif /* __MOTION_H */
