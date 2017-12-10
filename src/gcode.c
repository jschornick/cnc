// File       : gcode.c
// Author     : Jeff Schornick
//
// G-code parsing and representation
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stddef.h>
#include "fifo.h"
#include "uart.h"
#include "motion.h"
#include "gcode.h"

fifo_t gcode_input_fifo;
uint16_t gcode_cmd_head;
uint16_t gcode_cmd_tail;
uint16_t gcode_cmd_count;

uint8_t gcode_enabled = 0;  // does queued g-code get run?

uint8_t gcode_motion_mode = GCODE_RAPID;
uint8_t gcode_coord_mode = GCODE_ABSOLUTE;

// static gcode state variables
uint16_t gcode_feed_rate = 0;
uint32_t gcode_x = 0;
uint32_t gcode_y = 0;
uint32_t gcode_z = 0;

gcode_line_t gcode_cmd_queue[GCODE_CMD_QUEUE_SIZE];

void init_gcode_state(void)
{
  gcode_motion_mode = GCODE_RAPID;
  gcode_coord_mode = GCODE_ABSOLUTE;
  gcode_feed_rate = 0;

  // Alternately reset to machine XYZ state??
  gcode_x = 0;
  gcode_y = 0;
  gcode_z = 0;
}

void gcode_to_motion(size_t index)
{
  gcode_line_t *line = &gcode_cmd_queue[index];
  uint16_t id = index;
  /* uart_queue_str("Running G-code:\r\n"); */
  /* print_gcode_line(index); */
  uart_queue_str("\r\nRunning G-code cmd # ");
  uart_queue_dec(index);
  //if (gcode_cmd_queue[index].set[GCODE('N')]) {
  if (GCODE_IS_SET(&gcode_cmd_queue[index], 'N')) {
    uart_queue_str(" (line #");
    uart_queue_dec(gcode_cmd_queue[index].value[GCODE('N')]);
    uart_queue_str(")");
    id = gcode_cmd_queue[index].value[GCODE('N')];
  }
  uart_queue_str("\r\n");

  /* if( line->set[GCODE('G')] ) { */
  if( GCODE_IS_SET(line, 'G') ) {
    switch (line->value[GCODE('G')]) {
      case GCODE_RAPID:
        gcode_motion_mode = GCODE_RAPID;
        break;
      case GCODE_LINEAR:
        gcode_motion_mode = GCODE_LINEAR;
        break;
      case GCODE_CW:
        gcode_motion_mode = GCODE_CW;
        break;
      case GCODE_CCW:
        gcode_motion_mode = GCODE_CCW;
        break;
      case GCODE_ABSOLUTE:
        gcode_coord_mode = GCODE_ABSOLUTE;
        break;
      case GCODE_RELATIVE:
        gcode_coord_mode = GCODE_RELATIVE;
        break;
      // otherwise keep existing mode
    }
  }

  int32_t dx;
  int32_t dy;
  int32_t dz;

  int32_t i,j;

  if (gcode_coord_mode == GCODE_RELATIVE) {
    dx = (GCODE_IS_SET(line,'X')) ? line->value[GCODE('X')] : 0;
    dy = (GCODE_IS_SET(line,'Y')) ? line->value[GCODE('Y')] : 0;
    dz = (GCODE_IS_SET(line,'Z')) ? line->value[GCODE('Z')] : 0;
  } else {  // absolute positioning
    dx = (GCODE_IS_SET(line,'X')) ? line->value[GCODE('X')] - gcode_x : 0;
    dy = (GCODE_IS_SET(line,'Y')) ? line->value[GCODE('Y')] - gcode_y : 0;
    dz = (GCODE_IS_SET(line,'Z')) ? line->value[GCODE('Z')] - gcode_z : 0;
  }
  gcode_x += dx;
  gcode_y += dy;
  gcode_z += dz;

  i = (GCODE_IS_SET(line,'I')) ? line->value[GCODE('I')] : 0;
  j = (GCODE_IS_SET(line,'J')) ? line->value[GCODE('J')] : 0;

  gcode_feed_rate = (GCODE_IS_SET(line,'F')) ? line->value[GCODE('F')] : gcode_feed_rate;

  if ( dx || dy || dz ) {
    switch(gcode_motion_mode) {
      case GCODE_LINEAR:
        next_motion = new_linear_motion(dx, dy, dz, gcode_feed_rate, id);
        break;
      case GCODE_RAPID:
        next_motion = new_rapid_motion(dx, dy, dz, id);
        break;
      case GCODE_CW:
        next_motion = new_arc_motion(dx, dy, i, j, GCODE_CW, gcode_feed_rate, id);
        break;
      case GCODE_CCW:
        next_motion = new_arc_motion(dx, dy, i, j, GCODE_CCW, gcode_feed_rate, id);
        break;
    }
  }
}

void run_gcode(void)
{
  if (gcode_cmd_count) {
    /* uart_queue_str("G-code lines queued: "); */
    /* uart_queue_dec(gcode_cmd_count); */
    /* uart_queue_str("\r\n"); */
    if (!next_motion) {

      gcode_to_motion(gcode_cmd_tail);

      gcode_cmd_count--;
      if (gcode_cmd_tail++ == GCODE_CMD_QUEUE_SIZE) {
        gcode_cmd_tail = 0;
      }

    } else {
      /* uart_queue_str("...but motion queue full!\r\n"); */
    }
    motion_start();
  }
}

void gcode_zero_line(size_t index)
{
  /* for (uint8_t i=0; i<GCODE_MAX; i++) { */
  /*   gcode_cmd_queue[index].set[i] = 0; */
  /* } */
  gcode_cmd_queue[index].set = 0;
}


void init_parser(void)
{
  // init raw text character fifo
  fifo_init(&gcode_input_fifo, GCODE_INPUT_FIFO_SIZE);

  // init cmd queue
  gcode_cmd_head = 0;
  gcode_cmd_tail = 0;
  gcode_cmd_count = 0;

}


void add_to_gcode_line(size_t index, char code, int32_t value)
{
  /* gcode_cmd_queue[index].set[GCODE(code)] = 1; */
  gcode_cmd_queue[index].set |= 1<<(GCODE(code));
  gcode_cmd_queue[index].value[GCODE(code)] = value;
}

void print_gcode_line(size_t index)
{
  uart_queue_str("\r\nG-code cmd # ");
  uart_queue_dec(index);
  //if (gcode_cmd_queue[index].set[GCODE('N')]) {
  if (GCODE_IS_SET(&gcode_cmd_queue[index], 'N')) {
    uart_queue_str(" (line #");
    uart_queue_dec(gcode_cmd_queue[index].value[GCODE('N')]);
    uart_queue_str(")");
  }
  uart_queue_str("\r\n");
  /* for (uint8_t i = 0; i<GCODE_CODE_MAX; i++) { */
  for (char i='A'; i<='Z'; i++) {
    if ( GCODE_IS_SET(&gcode_cmd_queue[index],i) ) {
      uart_queue_str("  ");
      uart_queue(i);
      uart_queue_str(" = ");
      uart_queue_sdec(gcode_cmd_queue[index].value[GCODE(i)]);
      uart_queue_str("\r\n");
    }
  }
}

void end_gcode_line()
{
  /* print_gcode_line(gcode_cmd_head); */
  gcode_cmd_head++;
  // TODO: test wrapping
  if (gcode_cmd_head == GCODE_CMD_QUEUE_SIZE) {
    gcode_cmd_head = 0;
  }
  gcode_cmd_count++;
}


void parse_gcode()
{
  char c;
  char code;
  int32_t value;
  int8_t sign;
  gcode_parser_state_t state = GCODE_PARSE_CODE;

  gcode_zero_line(gcode_cmd_head);

  /* uart_queue_str("Parsing: "); */
  while (fifo_pop(&gcode_input_fifo, &c) == FIFO_OK) {

    /* uart_queue(c); */
    switch(state) {

      case GCODE_PARSE_CODE:
        switch(c) {
          case ' ':
            break;
          case '\r':
            end_gcode_line();
            break;
          case '\n':
            break;
          default:
            if( (c >= 'A') && (c <= 'Z') ) {
              code = c;
              state = GCODE_PARSE_VALUE_START;
            } else {
              uart_queue_str("Unexpected G-code char!\r\n");
            }
            break;
        }
        break;

      case GCODE_PARSE_VALUE_START:
        sign = 1;
        value = 0;
        state = GCODE_PARSE_VALUE;
      case GCODE_PARSE_VALUE:
        switch(c) {
          case '-':
            if (value == 0) {
              sign = -1;
            }
            break;
        case '.':
          uart_queue_str("Decimal not supported!\r\n");
          break;
        case ' ':
          add_to_gcode_line(gcode_cmd_head, code, value*sign);
          state = GCODE_PARSE_CODE;
          break;
        case '\r':
          add_to_gcode_line(gcode_cmd_head, code, value*sign);
          print_gcode_line(gcode_cmd_head);
          end_gcode_line();
          state = GCODE_PARSE_CODE;
          break;
        case '\n':
          break;
        default:
          if ((c >= '0') && (c <= '9')) {
            value *= 10;
            value += c - '0';
          } else {
            uart_queue_str("Expected digit!\r\n");
          }
          break;
        }
        break;
    }

  } // end while FIFO_OK

  uart_queue_str("\r\n");
  uart_queue_str("Done parsing\r\n");
}
