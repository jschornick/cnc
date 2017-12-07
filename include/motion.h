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

/* extern volatile uint32_t x_pos; */
/* extern volatile uint32_t y_pos; */
/* extern volatile uint32_t z_pos; */

extern volatile int32_t pos[];
extern int32_t target[];

/* extern int32_t x_target; */
/* extern int32_t y_target; */
/* extern int32_t z_target; */

void rapid_x(uint32_t steps);
void rapid_y(uint32_t steps);
void rapid_z(uint32_t steps);

void goto_pos(int32_t x, int32_t y, int32_t z);

void rapid_speed(uint32_t speed);
void home(void);

#endif /* __MOTION_H */
