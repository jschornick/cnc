// File       : menu.c
// Author     : Jeff Schornick
//
// CNC controller menu system
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include "uart.h"
#include "tmc.h"
#include "motion.h"
#include "gcode.h"
#include "menu.h"

// The selected stepper controller

Input_State_t input_state;
Menu_State_t menu_state;
uint32_t input_value;

uint32_t code_step = 0;
uint32_t code[][3] = { {-200, 200, 0},
                       {200, 200, 0},
                       {200, -200, 0},
                       {-200, -200, 0},
                       {-200, 200, 0},
                       {200, -200}};

void (*input_callback)(void *arg);

void update_current_scale_cb(void *arg) {
  uint32_t val = *((uint32_t *) arg);
  if (val <= 0x1f) {
    tmc_set_current_scale(tmc, val);
    uart_queue_str("Config updated!\r\n");
  } else {
    uart_queue_str("Value ");
    uart_queue_hex(val, 32);
    uart_queue_str("\r\n");
    uart_queue_str("Out of range!\r\n");
  }
}

void update_microstep_cb(void *arg) {
  uint32_t val = *((uint32_t *) arg);
  if (val <= 0x08) {
    tmc_set_microstep(tmc, val);
    uart_queue_str("Config updated!\r\n");
  } else {
    uart_queue_str("Value ");
    uart_queue_hex(val, 32);
    uart_queue_str("\r\n");
    uart_queue_str("Out of range!\r\n");
  }
}

void rapid_motion_cb(void *arg) {
  rapid(tmc, *((uint32_t *) arg));
}

void set_max_rate_cb(void *arg) {
  set_max_rate( *(uint32_t *) arg);
}

void display_config(uint8_t tmc)
{
  uart_queue_str("Configuration for stepper #");
  uart_queue_hex(tmc, 4);
  uart_queue_str("\r\n\r\n");
  uart_queue_str("Driver Config (DRVCONF)   : ");
  uart_queue_hex(tmc_config[tmc].drvconf.raw, 20);
  uart_queue_str("\r\n");
  uart_queue_str("  Voltage sense           : ");
  uart_queue_hex(tmc_config[tmc].drvconf.VSENSE, 1);
  uart_queue_str("\r\n");
  uart_queue_str("  Step Interface  (0=EN)  : ");
  uart_queue_hex(tmc_config[tmc].drvconf.SDOFF, 1);
  uart_queue_str("\r\n");
  uart_queue_str("  Slope control (low)     : ");
  uart_queue_hex(tmc_config[tmc].drvconf.SLPL, 2);
  uart_queue_str("\r\n");
  uart_queue_str("  Slope control (high)    : ");
  uart_queue_hex(tmc_config[tmc].drvconf.SLPH, 2);
  uart_queue_str("\r\n");
  uart_queue_str("  Short-to-ground (0=EN)  : ");
  uart_queue_hex(tmc_config[tmc].drvconf.DISS2G, 1);
  uart_queue_str("\r\n");
  uart_queue_str("  S2G Time                : ");
  uart_queue_hex(tmc_config[tmc].drvconf.TS2G, 2);
  uart_queue_str("\r\n\r\n");

  uart_queue_str("Driver Control (DRVCTL)   : ");
  uart_queue_hex(tmc_config[tmc].drvctl.raw, 20);
  uart_queue_str("\r\n");
  uart_queue_str("  Step Resolution (8=full): ");
  uart_queue_hex(tmc_config[tmc].drvctl.MRES, 4);
  uart_queue_str("\r\n");
  uart_queue_str("  Step edge (1=BOTH)      : ");
  uart_queue_hex(tmc_config[tmc].drvctl.DEDGE, 1);
  uart_queue_str("\r\n");
  uart_queue_str("  Step interpolate (1=ON) : ");
  uart_queue_hex(tmc_config[tmc].drvctl.INTPOL, 1);
  uart_queue_str("\r\n\r\n");

  uart_queue_str("Chopper Config (CHOPCONF) : ");
  uart_queue_hex(tmc_config[tmc].chopconf.raw, 20);
  uart_queue_str("\r\n");
  uart_queue_str("  Mode (0=spread, 1=const): ");
  uart_queue_hex(tmc_config[tmc].chopconf.CHM, 1);
  uart_queue_str("\r\n");
  uart_queue_str("  Decay time (0=freewheel): ");
  uart_queue_hex(tmc_config[tmc].chopconf.TOFF, 4);
  uart_queue_str("\r\n");
  uart_queue_str("  Blank time              : ");
  uart_queue_hex(tmc_config[tmc].chopconf.TBL, 2);
  uart_queue_str("\r\n\r\n");

  uart_queue_str("StallGuard (SGCSCONF)     : ");
  uart_queue_hex(tmc_config[tmc].sgcsconf.raw, 20);
  uart_queue_str("\r\n");
  uart_queue_str("  Current scaling         : ");
  uart_queue_hex(tmc_config[tmc].sgcsconf.CSCALE, 5);
  uart_queue_str("\r\n");
  uart_queue_str("  Threshold (7-bit signed): ");
  uart_queue_hex(tmc_config[tmc].sgcsconf.SGT, 7);
  uart_queue_str("\r\n");
  uart_queue_str("  Filtering (1=ON)        : ");
  uart_queue_hex(tmc_config[tmc].sgcsconf.SFILT, 1);
  uart_queue_str("\r\n\r\n");

  uart_queue_str("CoolStep (SMARTEN)        : ");
  uart_queue_hex(tmc_config[tmc].smarten.raw, 20);
  uart_queue_str("\r\n");
  uart_queue_str("  Minimum (0=disabled)    : ");
  uart_queue_hex(tmc_config[tmc].smarten.SEMIN, 4);
  uart_queue_str("\r\n\r\n");
}

void display_tmc_status(void)
{

  uint32_t resp_ustep;
  uint32_t resp_sg;
  uint32_t resp_sgcs;

  tmc_response_t *resp;

  //uint32_t drvconf;
  drvconf_t drvconf;

  drvconf = tmc_config[tmc].drvconf;
  // NB: setting changes aren't reflected until *next* response, so first write just sets read mode
  drvconf.RDSEL = TMC_RDSEL_USTEP;
  tmc_send(tmc, drvconf.raw);

  drvconf.RDSEL = TMC_RDSEL_SG;
  resp_ustep = tmc_send(tmc, drvconf.raw);

  drvconf.RDSEL = TMC_RDSEL_SGCS;
  resp_sg = tmc_send(tmc, drvconf.raw);

  drvconf.RDSEL = TMC_RDSEL_USTEP;
  resp_sgcs = tmc_send(tmc, drvconf.raw);

  uart_queue_str("\r\nTMC response:\r\n");
  uart_queue_str("-------------\r\n");

  uart_queue_str("Raw uStep : ");
  uart_queue_hex(resp_ustep, 20);
  uart_queue_str("\r\n");
  uart_queue_str("Raw SG    : ");
  uart_queue_hex(resp_sg, 20);
  uart_queue_str("\r\n");
  uart_queue_str("Raw SGCS  : ");
  uart_queue_hex(resp_sgcs, 20);
  uart_queue_str("\r\n");

  resp = (tmc_response_t *) &resp_ustep;
  uart_queue_str("(00) StallGuard status   : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) resp->ustep.SG) );
  uart_queue_str("\r\n");
  uart_queue_str("(01) Over temp shutdown  : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) resp->ustep.OT) );
  uart_queue_str("\r\n");
  uart_queue_str("(02) Over temp warning   : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) resp->ustep.OTPW) );
  uart_queue_str("\r\n");
  uart_queue_str("(03) Short to ground (A) : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) resp->ustep.S2GA) );
  uart_queue_str("\r\n");
  uart_queue_str("(04) Short to ground (B) : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) resp->ustep.S2GB) );
  uart_queue_str("\r\n");
  uart_queue_str("(05) Open load (A)       : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) resp->ustep.OLA) );
  uart_queue_str("\r\n");
  uart_queue_str("(06) Open load (B)       : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) resp->ustep.OLB) );
  uart_queue_str("\r\n");
  uart_queue_str("(07) Standstill          : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) resp->ustep.STST) );
  uart_queue_str("\r\n");

  uart_queue_str("     MSTEP               : ");
  if(resp->ustep.POL) {
    uart_queue('+');
  } else {
    uart_queue('-');
  }
  uart_queue_hex(resp->ustep.MSTEP, 9);
  uart_queue_str("\r\n");

  resp = (tmc_response_t *) &resp_sg;
  uart_queue_str("     SG2                 : ");
  uart_queue_hex(resp->sg.SG10, 10);
  uart_queue_str("\r\n");

  resp = (tmc_response_t *) &resp_sgcs;
  uart_queue_str("     CS                  : ");
  uart_queue_hex(resp->sgcs.SE5, 5);
  uart_queue_str("\r\n");
}


void display_main_menu(uint8_t mode) {
  if (mode == 2) {
    uart_queue_str("\r\n\r\n");
    uart_queue_str("Main menu\r\n");
    uart_queue_str("---------\r\n");
  }

  uart_queue_str("Main [");
  uart_queue_hex(tmc, 4);
  uart_queue_str("] > ");
}

void display_config_menu(uint8_t mode) {
  if (mode == 2) {
    uart_queue_str("\r\n\r\n");
    uart_queue_str("Config menu\r\n");
    uart_queue_str("-----------\r\n");
  }
  uart_queue_str("Config [");
  uart_queue_hex(tmc, 4);
  uart_queue_str("] > ");
}

void config_menu(char c)
{
  uint8_t show_menu = 1;
  switch(c) {
    case '0':
      tmc = 0;
      uart_queue_str("Stepper 0 selected.\r\n");
      break;
    case '1':
      tmc = 1;
      uart_queue_str("Stepper 1 selected.\r\n");
      break;
    case '2':
      tmc = 2;
      uart_queue_str("Stepper 2 selected.\r\n");
      break;
    case 'c':
      uart_queue_str("Adjust current limit\r\n");
      uart_queue_str("Current limit is: 0x");
      uart_queue_hex(tmc_get_current_scale(tmc), 5);
      uart_queue_str("\r\nNew limit (0x00 - 0x1f)? ");
      input_value = 0;
      input_state = INPUT_HEX;
      input_callback = update_current_scale_cb;
      show_menu = 0;
      break;
    case 's':
      uart_queue_str("Adjust microstepping\r\n");
      uart_queue_str("Current step resolution: 0x");
      uart_queue_hex(tmc_get_microstep(tmc), 4);
      uart_queue_str("\r\nNew resolution (0x0 - 0x8)? ");
      input_value = 0;
      input_state = INPUT_HEX;
      input_callback = update_microstep_cb;
      show_menu = 0;
      break;
    case 'l':
      uart_queue_str("List configuration\r\n");
      display_config(tmc);
      break;
    case ASCII_ESCAPE:
      uart_queue_str("Return to main\r\n");
      menu_state = MENU_MAIN;
      show_menu = 0;
      display_main_menu(2);
      break;
    case '?':
      uart_queue_str("\r\n");
      show_menu = 2;
      break;
    default:
      show_menu = 0;
      break;
  }
  if (show_menu) {
    display_config_menu(show_menu);
  }
}

void display_motion_menu(uint8_t mode) {
  if (mode == 2) {
    uart_queue_str("\r\n\r\n");
    uart_queue_str("Motion menu\r\n");
    uart_queue_str("-----------\r\n");
  }
  uart_queue_str("Motion [");
  if(tmc == X_AXIS) {
    uart_queue('X');
  } else if(tmc == Y_AXIS) {
    uart_queue('Y');
  } else {
    uart_queue('Z');
  }
  uart_queue_str("] (");
  uart_queue_sdec(pos[X_AXIS]);
  uart_queue_str(", ");
  uart_queue_sdec(pos[Y_AXIS]);
  uart_queue_str(", ");
  uart_queue_sdec(pos[Z_AXIS]);
  uart_queue_str(") > ");
}

void motion_menu(char c)
{
  uint8_t show_menu = 1;
  switch(c) {
    case 's':
      uart_queue_str("Step ");
      gpio_toggle(tmc_pins[tmc].step_port, tmc_pins[tmc].step_pin);
      if( tmc_get_dir(tmc) == TMC_FWD ) {
        uart_queue_str("++\r\n");
        pos[tmc]++;
      } else {
        pos[tmc]--;
        uart_queue_str("--\r\n");
      }
      break;
    case 'S':
      uart_queue_str("Step All\r\n");
      gpio_toggle(tmc_pins[0].step_port, tmc_pins[0].step_pin);
      gpio_toggle(tmc_pins[1].step_port, tmc_pins[1].step_pin);
      gpio_toggle(tmc_pins[2].step_port, tmc_pins[2].step_pin);
      break;
    case '<':
      uart_queue_str("Direction: --\r\n");
      gpio_low(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
      break;
    case '>':
      uart_queue_str("Direction: ++\r\n");
      gpio_high(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
      break;
    case 'x':
      tmc = X_AXIS;
      uart_queue_str("X-axis selected.\r\n");
      break;
    case 'y':
      tmc = Y_AXIS;
      uart_queue_str("Y-axis selected.\r\n");
      break;
    case 'z':
      tmc = Z_AXIS;
      uart_queue_str("Z-axis selected.\r\n");
      break;
    case 'r':
      uart_queue_str("Rapid move\r\n");
      uart_queue_str("Number of steps ? ");
      input_value = 0;
      input_state = INPUT_DEC;
      input_callback = rapid_motion_cb;
      show_menu = 0;
      break;
    case 'R':
      uart_queue_str("Set max rate\r\n");
      uart_queue_str("Max is ");
      uart_queue_dec(max_rate);
      uart_queue_str(" (steps/s). New rate? ");
      input_value = 0;
      input_state = INPUT_DEC;
      input_callback = set_max_rate_cb;
      show_menu = 0;
      break;
    case 'h':
      uart_queue_str("Return home\r\n");
      home();
      break;
    case '0':
      uart_queue_str("Zero axes\r\n");
      pos[X_AXIS] = 0;
      pos[Y_AXIS] = 0;
      pos[Z_AXIS] = 0;
      break;
    case 'c':
      uart_queue_str("Execute code\r\n");
      uart_queue_str("Step # ");
      uart_queue_dec(code_step);
      uart_queue_str("\r\n");
      goto_pos(code[code_step][0], code[code_step][1], code[code_step][2]);
      code_step++;
      code_step %= 6;
      break;
    case 't':
      uart_queue_str("New test motion\r\n");
      if (!next_motion) {
        next_motion = new_motion(42, 23, max_rate, 99);
        motion_start();
      } else {
        uart_queue_str("Motion queue full!\r\n");
      }
      break;

    case 'm':
      uart_queue_str("Running motions\r\n");
      motion_start();
      break;

    case ASCII_ESCAPE:
      uart_queue_str("Return to main\r\n");
      menu_state = MENU_MAIN;
      show_menu = 0;
      display_main_menu(2);
      break;
    case '?':
      uart_queue_str("\r\n");
      show_menu = 2;
      break;
    default:
      show_menu = 0;
      break;
  }
  if (show_menu) {
    display_motion_menu(show_menu);
  }
}


void read_dec(char c)
{
  if( (c >= '0') && (c <= '9') ) {
    uart_queue(c);
    input_value = (input_value * 10) + (c - '0');
  }
}

void read_hex(char c)
{
  if( (c >= '0') && (c <= '9') ) {
    uart_queue(c);
    input_value = (input_value * 16) + (c - '0');
  }
  if ( (c >= 'a') && (c <= 'f') ) {
    uart_queue(c);
    input_value = (input_value * 16) + (c - 'a' + 10);
  }
}

void main_menu(char c)
{
  uint8_t i = 0;

  uint8_t show_menu = 1;

  switch(c) {
  case '0':
    tmc = 0;
    uart_queue_str("Stepper 0 selected.\r\n");
    break;
  case '1':
    tmc = 1;
    uart_queue_str("Stepper 1 selected.\r\n");
    break;
  case '2':
    tmc = 2;
    uart_queue_str("Stepper 2 selected.\r\n");
    break;

  case 'r':
    uart_queue_str("Read TMC\r\n");
    display_tmc_status();
    break;
  case 'e':
    gpio_low(tmc_pins[tmc].en_port, tmc_pins[tmc].en_pin);
    uart_queue_str("MOSFETs enabled\r\n");
    break;
  case 'E':
    for(i = 0; i < 3; i++) {
      gpio_low(tmc_pins[i].en_port, tmc_pins[i].en_pin);
    }
    uart_queue_str("All MOSFETs enabled\r\n");
    break;
  case 'd':
    gpio_high(tmc_pins[tmc].en_port, tmc_pins[tmc].en_pin);
    uart_queue_str("MOSFETs disabled\r\n");
    break;
  case 'D':
    for(i = 0; i < 3; i++) {
      gpio_high(tmc_pins[i].en_port, tmc_pins[i].en_pin);
    }
    uart_queue_str("All MOSFETs disabled\r\n");
    break;
  case 'i':
    uart_queue_str("Re-initalizing TMCs\r\n");
    tmc_init();
    break;
  case 'c':
    display_config_menu(2);
    menu_state = MENU_CONFIG;
    show_menu = 0;
    break;
  case 'm':
    display_motion_menu(2);
    menu_state = MENU_MOTION;
    show_menu = 0;
    break;
  case 'g':
    uart_queue_str("G-code\r\n");
    input_state = INPUT_GCODE;
    show_menu = 0;
    break;
  case 'G':
    if (gcode_enabled) {
      uart_queue_str("G-code diabled!\r\n");
      gcode_enabled = 0;
    } else {
      uart_queue_str("G-code enabled!\r\n");
      gcode_enabled = 1;
    }
    break;
  case '?':
    show_menu = 2;
    break;
  default:
    show_menu = 0;
    break;
  }

  if (show_menu) {
    display_main_menu(show_menu);
  }
}


void process_input(char c)
{

  switch(input_state) {

    case INPUT_MENU:
      switch(menu_state) {
        case MENU_MAIN:
          main_menu(c);
          break;
        case MENU_CONFIG:
          config_menu(c);
          break;
        case MENU_MOTION:
          motion_menu(c);
          break;
      }
      break;

    case INPUT_HEX:
      switch(c) {
        case ASCII_ESCAPE:
          uart_queue_str("\r\n");
          input_state = INPUT_MENU;
          break;
        case '\r':
          uart_queue_str("\r\n");
          input_callback( (void *) &input_value);
          input_state = INPUT_MENU;
          break;
        default:
          read_hex(c);
          break;
      }
      break;

    case INPUT_DEC:
      switch(c) {
      case ASCII_ESCAPE:
        uart_queue_str("\r\n");
        input_state = INPUT_MENU;
        break;
      case '\r':
        uart_queue_str("\r\n");
        input_callback( (void *) &input_value);
        input_state = INPUT_MENU;
        break;
      default:
        read_dec(c);
        break;
      }
      break;

    case INPUT_GCODE:
      switch(c) {
        case ASCII_ESCAPE:
          uart_queue_str("\r\n");
          input_state = INPUT_MENU;
          display_main_menu(1);
          break;
        case '\r':
          fifo_push(&gcode_input_fifo, c);
          parse_gcode();
          break;
        default:
          fifo_push(&gcode_input_fifo, c);
          break;
      }
      break;
  }
}
