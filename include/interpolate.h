// File       : interpolate.h
// Author     : Jeff Schornick
//
// Step interpolation algorithms
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#ifndef __INTERPOLATE_H
#define __INTERPOLATE_H

#include "motion.h"

void rapid_interpolate(int32_t *start_pos, int32_t *end_pos, motion_t *motion);
void linear_interpolate(int32_t *start_pos, int32_t *end_pos, uint16_t rate, motion_t *motion);
void arc_interpolate(int32_t *start_pos, int32_t *end_pos, int32_t x_off, int32_t y_off, uint8_t rot, uint16_t rate, motion_t *motion);

#endif /* __INTPOLATE_H */
