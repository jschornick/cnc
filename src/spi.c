// File       : spi.c
// Author     : Jeff Schornick
//
// MSP432 SPI driver using interrupts and a pair of RX/TX buffers.
// EUSCIB1 is connected SPI bus for the TMC stepper drivers
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include <stdio.h>
#include "msp432p401r.h"
#include "gpio.h"
#include "uart.h"

volatile uint8_t spi_tx_data;
volatile uint8_t spi_rx_data;
volatile uint8_t spi_rx_flag;

// Based on TI Resource explorer example msp432p401x_euscib0_spi_09
void spi_init(void)
{

  // MSP432P401R datasheet, p160
  // UCB1 SPI pins
  // SCK  : 6.3
  // MOSI : 6.4
  // MISO : 6.5
  P6->SEL0 |= BIT3 | BIT4 | BIT5;         // Set for SPI
  P6->SEL1 &= ~(BIT3 | BIT4 | BIT5);

  EUSCI_B1->CTLW0 |= EUSCI_B_CTLW0_SWRST; // Put eUSCI state machine in reset

  // CKPL=? : clock polarity
  //           0 = inactive low
  //           1 = inactive high
  // CKPH=? : clock phase
  //           0 = data changes on first CLK edge, captured on following
  //           1 = data captured on first CLK edge, changes on following
  // UC7BIT=0 : 8-bit
  // UCMODE=0 : 3-bit SPI (?)
  EUSCI_B1->CTLW0 = EUSCI_B_CTLW0_SWRST | // Remain eUSCI state machine in reset
    //EUSCI_B_CTLW0_CKPH |  // phase =0
    EUSCI_B_CTLW0_CKPL |  // polarity=1
    EUSCI_B_CTLW0_MST |             // Set as SPI master
    EUSCI_B_CTLW0_SYNC |            // Put in synchronous (SPI) mode
    EUSCI_B_CTLW0_MSB;              // MSB first

  //EUSCI_B1->CTLW0 |= EUSCI_B_CTLW0_SSEL__ACLK; // ACLK
  EUSCI_B1->CTLW0 |= EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK
  EUSCI_B1->BRW = 0x08;       // clock div=5, fBitClock = fBRCLK/(UCBRx+1).
  EUSCI_B1->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;// Initialize USCI state machine

  spi_rx_flag = 0x0;

  // slave select initialized in tmc_init()
}

uint8_t spi_send_byte(uint8_t tx_data)
{
  uint8_t rx_data = 0;
  /* uart_queue_str("SPI: Out:"); */
  /* uart_queue_uint8(tx_data); */

  while (!(EUSCI_B1->IFG & EUSCI_B_IFG_TXIFG));
  EUSCI_B1->TXBUF = tx_data;

  while (!(EUSCI_B1->IFG & EUSCI_B_IFG_RXIFG));
  rx_data = EUSCI_B1->RXBUF;

  /* uart_queue_str("  In:"); */
  /* uart_queue_uint8(rx_data); */
  /* uart_queue_str("\r\n"); */

  return rx_data;
}


