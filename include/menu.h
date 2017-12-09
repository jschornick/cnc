// File       : menu.h
// Author     : Jeff Schornick
//
// CNC controller menu system
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#ifndef __MENU_H
#define __MENU_H

#include <stdint.h>

#define ASCII_ESCAPE 0x1B

extern uint8_t tmc;

typedef enum {
  INPUT_MENU = 0,
  INPUT_DEC,
  INPUT_HEX,
  INPUT_GCODE,
} Input_State_t;

typedef enum {
  MENU_MAIN = 0,
  MENU_CONFIG,
  MENU_MOTION
} Menu_State_t;

extern Input_State_t input_state;
extern Menu_State_t menu_state;

void process_input(char c);
void display_main_menu(uint8_t mode);

#endif /*  __MENU_H */
