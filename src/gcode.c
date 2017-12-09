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

gcode_line_t gcode_cmd_queue[GCODE_CMD_QUEUE_SIZE];

void gcode_to_motion(size_t index)
{
  uart_queue_str("Running G-code:\r\n");
  print_gcode_line(index);
}

void run_gcode(void)
{
  if (gcode_cmd_count) {
    uart_queue_str("G-code lines queued: ");
    uart_queue_dec(gcode_cmd_count);
    uart_queue_str("\r\n");
    if (!next_motion) {

      gcode_to_motion(gcode_cmd_tail);

      gcode_cmd_count--;
      if (gcode_cmd_tail++ == GCODE_CMD_QUEUE_SIZE) {
        gcode_cmd_tail = 0;
      }

    } else {
      uart_queue_str("...but motion queue full!\r\n");
    }
  }
}

void gcode_zero_line(size_t index)
{
  size_t i;
  for (i=0; i<GCODE_CODE_MAX; i++) {
    gcode_cmd_queue[index].set[i] = 0;
  }
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


void add_to_gcode_line(size_t index, gcode_code_t code, int32_t value)
{
  gcode_cmd_queue[index].set[code] = 1;
  gcode_cmd_queue[index].value[code] = value;
  /* uart_queue_str("Adding "); */
  /* uart_queue_dec(code); */
  /* uart_queue_str(" : "); */
  /* uart_queue_sdec(value); */
  /* uart_queue_str("\r\n"); */
}

void print_gcode_line(size_t index)
{
  uart_queue_str("Line # ");
  uart_queue_dec(index);
  uart_queue_str(" codes:\r\n");
  for (uint8_t i = 0; i<GCODE_CODE_MAX; i++) {
    if (gcode_cmd_queue[index].set[i] == 1) {
      uart_queue_str("  ");
      uart_queue_dec(i);
      uart_queue_str(" = ");
      uart_queue_sdec(gcode_cmd_queue[index].value[i]);
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
  gcode_code_t code;
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
          case 'G':
            code = GCODE_G;
            state = GCODE_PARSE_VALUE_START;
            break;
          case 'M':
            code = GCODE_M;
            state = GCODE_PARSE_VALUE_START;
            break;
          case 'X':
            code = GCODE_X;
            state = GCODE_PARSE_VALUE_START;
            break;
          case 'Y':
            code = GCODE_Y;
            state = GCODE_PARSE_VALUE_START;
            break;
          case 'F':
            code = GCODE_F;
            state = GCODE_PARSE_VALUE_START;
            break;
          case 'Z':
            code = GCODE_Z;
            state = GCODE_PARSE_VALUE_START;
            break;
          case ' ':
            break;
          case '\r':
            end_gcode_line();
            break;
          case '\n':
            break;
          default:
            uart_queue_str("Unexpected G-code char!\r\n");
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
          uart_queue_str("Decimal values not supported!\r\n");
          break;
        case ' ':
          add_to_gcode_line(gcode_cmd_head, code, value*sign);
          state = GCODE_PARSE_CODE;
          break;
        case '\r':
          add_to_gcode_line(gcode_cmd_head, code, value*sign);
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
