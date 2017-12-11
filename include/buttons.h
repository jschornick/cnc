// File       : buttons.h
// Author     : Jeff Schornick
//
// MSP432 button configuration
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#ifndef __BUTTONS_H
#define __BUTTONS_H

#define BUTTON1_PORT P1
#define BUTTON1_PIN  PIN1
#define BUTTON1 BUTTON1_PORT, BUTTON1_PIN

#define BUTTON2_PORT P1
#define BUTTON2_PIN  PIN4
#define BUTTON2 BUTTON2_PORT, BUTTON2_PIN

#define BUTTON3_PORT P3
#define BUTTON3_PIN  PIN0
#define BUTTON3 BUTTON3_PORT, BUTTON3_PIN

extern volatile uint8_t B1_flag;
extern volatile uint8_t B2_flag;
extern volatile uint8_t B3_flag;

void button_init(void);
void enable_limit_switch(void);
void disable_limit_switch(void);

#endif /* __BUTTONS_H */
