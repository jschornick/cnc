// File       : gcode.h
// Author     : Jeff Schornick
//
// G-code parsing and representation
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details


#ifndef __GCODE_H
#define __GCODE_H

#include <stdbool.h>
#include "fifo.h"

/* typedef enum { */
/*   uint8_t interpolate;  // t/f */
/*   uint8_t relative;  // t/f */
/* } gcode_move_mode_t; */

extern uint8_t gcode_enabled;

typedef enum {
  GCODE_G = 0,
  GCODE_M,
  GCODE_X,
  GCODE_Y,
  GCODE_Z,
  GCODE_F,
  GCODE_CODE_MAX
} gcode_code_t;

typedef struct {
  /* struct { */
  /*   uint16_t GCODE_G : 1; */
  /*   uint16_t GCODE_M : 1; */
  /*   uint16_t GCODE_X : 1; */
  /* } */
  bool set[GCODE_CODE_MAX];
  int32_t  value[GCODE_CODE_MAX];
} gcode_line_t;

typedef enum {
  GCODE_PARSE_CODE,
  GCODE_PARSE_VALUE_START,
  GCODE_PARSE_VALUE
} gcode_parser_state_t;

#define GCODE_CMD_QUEUE_SIZE 1000
extern gcode_line_t gcode_cmd_queue[];
extern uint16_t gcode_cmd_count;

#define GCODE_INPUT_FIFO_SIZE 1000
extern fifo_t gcode_input_fifo;

void init_parser(void);

void parse_gcode();

void print_gcode_line(size_t index);

void run_gcode(void);

#endif /* __GCODE_H */
