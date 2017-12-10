// File       : gcode.h
// Author     : Jeff Schornick
//
// G-code parsing and representation
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details


#ifndef __GCODE_H
#define __GCODE_H

#include "fifo.h"

extern uint8_t gcode_enabled;

#define GCODE(x) (x- 'A')
#define GCODE_CHAR(x) (x + 'A')
#define GCODE_MAX 26

#define GCODE_IS_SET(lineptr,x) ( (lineptr)->set & (1<<(x - 'A')) )

#define GCODE_RAPID  0
#define GCODE_LINEAR 1
#define GCODE_ABSOLUTE 90
#define GCODE_RELATIVE 91

// TODO: Storing the parsed G-code this way wastes memory and doesn't allow
// multiple codes of the same letter (e.g. G01 G90 on one line)
typedef struct {
  int32_t set;
  int16_t value[GCODE_MAX];
} gcode_line_t;

typedef enum {
  GCODE_PARSE_CODE,
  GCODE_PARSE_VALUE_START,
  GCODE_PARSE_VALUE
} gcode_parser_state_t;

#define GCODE_CMD_QUEUE_SIZE 500
extern gcode_line_t gcode_cmd_queue[];
extern uint16_t gcode_cmd_count;

#define GCODE_INPUT_FIFO_SIZE 1000
extern fifo_t gcode_input_fifo;

void init_parser(void);

void init_gcode_state(void);

void parse_gcode();

void print_gcode_line(size_t index);

void run_gcode(void);

#endif /* __GCODE_H */
