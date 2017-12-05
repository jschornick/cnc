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
#include "menu.h"

// The selected stepper controller
uint8_t tmc = 0;

Input_State_t input_state;
Menu_State_t menu_state;
uint32_t input_value;

void (*input_callback)(uint32_t);

void update_current_scale(uint32_t val) {
  if (val <= 0x1f) {
    tmc_config[tmc].sgcsconf &= ~SGCSCONF_CS_MASK;
    tmc_config[tmc].sgcsconf |= val;
    tmc_reconfigure(tmc, &tmc_config[tmc]);
    uart_queue_str("Config updated!\r\n");
  } else {
    uart_queue_str("Value ");
    uart_queue_hex(val, 32);
    uart_queue_str("\r\n");
    uart_queue_str("Out of range!\r\n");
  }
}

//void decode_response(uint32_t data)
void display_response(uint32_t ustep_data, uint32_t sg_data, uint32_t sgcs_data)
{

  ustep_resp_t * ustep_resp = (ustep_resp_t *) &ustep_data;
  sg_resp_t * sg_resp = (sg_resp_t *) &sg_data;
  sgcs_resp_t * sgcs_resp = (sgcs_resp_t *) &sgcs_data;

  uart_queue_str("\r\nTMC response:\r\n");
  uart_queue_str("-------------\r\n");

  uart_queue_str("Raw uStep : ");
  uart_queue_hex(ustep_data, 20);
  uart_queue_str("\r\n");
  uart_queue_str("Raw SG    : ");
  uart_queue_hex(sg_data, 20);
  uart_queue_str("\r\n");
  uart_queue_str("Raw SGCS  : ");
  uart_queue_hex(sgcs_data, 20);
  uart_queue_str("\r\n");

  uart_queue_str("(00) StallGuard status   : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->SG) );
  uart_queue_str("\r\n");
  uart_queue_str("(01) Over temp shutdown  : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->OT) );
  uart_queue_str("\r\n");
  uart_queue_str("(02) Over temp warning   : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->OTPW) );
  uart_queue_str("\r\n");
  uart_queue_str("(03) Short to ground (A) : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->S2GA) );
  uart_queue_str("\r\n");
  uart_queue_str("(04) Short to ground (B) : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->S2GB) );
  uart_queue_str("\r\n");
  uart_queue_str("(05) Open load (A)       : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->OLA) );
  uart_queue_str("\r\n");
  uart_queue_str("(06) Open load (B)       : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->OLB) );
  uart_queue_str("\r\n");
  uart_queue_str("(07) Standstill          : ");
  uart_queue(NIBBLE_TO_ASCII((uint8_t) ustep_resp->STST) );
  uart_queue_str("\r\n");

  uart_queue_str("     MSTEP               : ");
  if(ustep_resp->POL) {
    uart_queue('+');
  } else {
    uart_queue('-');
  }
  uart_queue_hex(ustep_resp->MSTEP, 9);
  uart_queue_str("\r\n");

  uart_queue_str("     SG2                 : ");
  uart_queue_hex(sg_resp->SG10, 10);
  uart_queue_str("\r\n");

  uart_queue_str("     CS                  : ");
  uart_queue_hex(sgcs_resp->SE5, 5);
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

void main_menu(char c)
{
  uint32_t resp_ustep;
  uint32_t resp_sg;
  uint32_t resp_sgcs;

  uint32_t drvconf;
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
    drvconf = tmc_config[tmc].drvconf & ~DRVCONF_RDSEL_MASK;
    tmc_send(tmc, drvconf | DRVCONF_RDSEL_USTEP);
    resp_ustep = tmc_send(tmc, drvconf | DRVCONF_RDSEL_SG);
    resp_sg = tmc_send(tmc, drvconf | DRVCONF_RDSEL_SGCS);
    resp_sgcs = tmc_send(tmc, drvconf | DRVCONF_RDSEL_USTEP);
    display_response(resp_ustep, resp_sg, resp_sgcs);
    break;
  case 's':
    uart_queue_str("Step\r\n");
    gpio_toggle(tmc_pins[tmc].step_port, tmc_pins[tmc].step_pin);
    break;
  case 'S':
    uart_queue_str("Step All\r\n");
    gpio_toggle(tmc_pins[0].step_port, tmc_pins[0].step_pin);
    gpio_toggle(tmc_pins[1].step_port, tmc_pins[1].step_pin);
    gpio_toggle(tmc_pins[2].step_port, tmc_pins[2].step_pin);
    break;
  case '<':
    uart_queue_str("Direction <<--\r\n");
    gpio_low(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
    break;
  case '>':
    uart_queue_str("Direction -->>\r\n");
    gpio_high(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
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
      input_callback = update_current_scale;
      show_menu = 0;
      break;
    case 's':
      uart_queue_str("Adjust microstepping\r\n");
      break;
    case 'l':
      uart_queue_str("List configuration\r\n");
      break;
    case 'm':
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


void update_dec(char c)
{
  if( (c >= '0') && (c <= '9') ) {
    uart_queue(c);
    input_value = (input_value * 10) + (c - '0');
  }
}

void update_hex(char c)
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
          input_callback(input_value);
          input_state = INPUT_MENU;
          break;
        default:
          update_hex(c);
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
        input_callback(input_value);
        input_state = INPUT_MENU;
        break;
      default:
        update_dec(c);
        break;
      }
      break;
  }
}
