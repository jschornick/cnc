// File       : tmc.c
// Author     : Jeff Schornick
//
// Functions to control the Trinamic TMC-26x series
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include "spi.h"
#include "gpio.h"
#include "tmc.h"
#include "uart.h"


tmc_config_t tmc_config[3];
tmc_pinout_t tmc_pins[3] = {
  { .cs_port   = TMC0_CS_PORT,   .cs_pin   = TMC0_CS_PIN,
    .en_port   = TMC0_EN_PORT,   .en_pin   = TMC0_EN_PIN,
    .step_port = TMC0_STEP_PORT, .step_pin = TMC0_STEP_PIN,
    .dir_port  = TMC0_DIR_PORT,  .dir_pin  = TMC0_DIR_PIN
  },
  { .cs_port   = TMC1_CS_PORT,   .cs_pin   = TMC1_CS_PIN,
    .en_port   = TMC1_EN_PORT,   .en_pin   = TMC1_EN_PIN,
    .step_port = TMC1_STEP_PORT, .step_pin = TMC1_STEP_PIN,
    .dir_port  = TMC1_DIR_PORT,  .dir_pin  = TMC1_DIR_PIN
  },
  { .cs_port   = TMC2_CS_PORT,   .cs_pin   = TMC2_CS_PIN,
    .en_port   = TMC2_EN_PORT,   .en_pin   = TMC2_EN_PIN,
    .step_port = TMC2_STEP_PORT, .step_pin = TMC2_STEP_PIN,
    .dir_port  = TMC2_DIR_PORT,  .dir_pin  = TMC2_DIR_PIN
  },
};

#define drvconf_init (DRVCONF                   \
                      | DRVCONF_VSENSE_305      \
                      | DRVCONF_SDOFF_STEP )

#define drvctl_init (DRVCTL                     \
                     | DRVCTL_MRES_1            \
                     | DRVCTL_DEDGE_BOTH        \
                     | DRVCTL_INTPOL_OFF )

#define chopconf_init ( CHOPCONF                \
                        | CHOPCONF_CHM_CONST    \
                        | CHOPCONF_TOFF(8) )

// sense: 0.07 Ohms
// I_rms = (CS+1)/32 * (V_FS/R_sense) * 1/sqrt(2)
#define sgcsconf_init ( SGCSCONF                \
                        | SGCSCONF_CS(5) )
//| SGCSCONF_CS(15);

#define smarten_init (SMARTEN)


void tmc_init(void)
{
  uint8_t i;
  // Slave select
  for(i=0; i<3; i++) {
    // Set SPI CS as output and initialize high (inactive)
    gpio_set_output(tmc_pins[i].cs_port, tmc_pins[i].cs_pin);
    gpio_set_high(tmc_pins[i].cs_port, tmc_pins[i].cs_pin);

    // Set MOSFET enable as output and initialize high (inactive)
    gpio_set_output(tmc_pins[i].en_port, tmc_pins[i].en_pin);
    gpio_set_high(tmc_pins[i].en_port, tmc_pins[i].en_pin);

    // Set Step/Dir pins as outputs and initialize
    gpio_set_output(tmc_pins[i].step_port, tmc_pins[i].step_pin);
    gpio_set_high(tmc_pins[i].step_port, tmc_pins[i].step_pin);

    gpio_set_output(tmc_pins[i].dir_port, tmc_pins[i].dir_pin);
    gpio_set_high(tmc_pins[i].dir_port, tmc_pins[i].dir_pin);
  }

  for(i=0; i<3; i++) {
    tmc_config[i].drvconf = drvconf_init;
    tmc_config[i].drvctl = drvctl_init;
    tmc_config[i].chopconf = chopconf_init;
    tmc_config[i].sgcsconf = sgcsconf_init;
    tmc_config[i].smarten = smarten_init;
    tmc_send(i, drvconf_init);
    tmc_send(i, drvctl_init);
    tmc_send(i, chopconf_init);
    tmc_send(i, sgcsconf_init);
    tmc_send(i, smarten_init);
  }


}

// shift in 24 bits (last 20 bits will be command)
// read in 24 bits (first 20 bits will be response)?
uint32_t tmc_send(uint8_t tmc, uint32_t tx_data)
{

  uint32_t response = 0x0;

  uint8_t tx_byte;

  uart_queue_str("\r\nTMC[");
  uart_queue_hex(tmc,4);
  uart_queue_str("] Sending: ");
  uart_queue_hex(tx_data, 20);
  uart_queue_str("\r\n");

  gpio_set_low(tmc_pins[tmc].cs_port, tmc_pins[tmc].cs_pin);

  tx_byte = tx_data >> 16;
  response = spi_send_byte(tx_byte) << 12;

  tx_byte = (tx_data >> 8) & 0xff ; // next 8-bits
  response |= spi_send_byte(tx_byte) << 4;

  tx_byte = tx_data & 0xff; // last 8 bits
  response |= spi_send_byte(tx_byte) >> 4;

  gpio_set_high(tmc_pins[tmc].cs_port, tmc_pins[tmc].cs_pin);

  uart_queue_str("TMC: Received: ");
  uart_queue_hex(response, 20);
  uart_queue_str("\r\n");

  return response;
}
