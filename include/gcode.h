// File       : gcode.h
// Author     : Jeff Schornick
//
// G-code parsing and representation
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details


#ifndef __GCODE_H
#define __GCODE_H

typdef enum {
  G00,
  G01
} gcode_mode_t;

typdef enum {
  PARAM_X,
  PARAM_Y,
  PARAM_Z,
  PARAM_F,
  PARAM_END
} gcode_param_t

typedef struct {
} gcode_t



#endif /* __GCODE_H */
