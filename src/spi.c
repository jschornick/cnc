// File       : spi.c
// Author     : Jeff Schornick
//
// MSP432 SPI driver using interrupts and a pair of RX/TX buffers.
// EUSCIB0 is connected SPI bus for the TMC stepper drivers
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include <stdio.h>
#include "msp432p401r.h"
#include "uart.h"

volatile uint8_t spi_tx_data;
volatile uint8_t spi_rx_data;
volatile uint8_t spi_rx_flag;

// Based on TI Resource explorer example msp432p401x_euscib0_spi_09
void spi_init(void)
{
  P1->SEL0 |= BIT5 | BIT6 | BIT7;         // Set P1.5, P1.6, and P1.7 as
                                          // SPI pins functionality

  // ???
  P1->SEL1 &= ~(BIT5 | BIT6 | BIT7);

  EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_SWRST; // Put eUSCI state machine in reset

  // CKPL=0 : clock polarity, inactive low,
  // CKPH=0 : clock phase, data sent on first low-high edge, sampled on next
  // UC7BIT=0 : 8-bit
  // UCMODE=0 : 3-bit SPI (?)
  EUSCI_B0->CTLW0 = EUSCI_B_CTLW0_SWRST | // Remain eUSCI state machine in reset
    EUSCI_B_CTLW0_CKPH |  // phase =1
    EUSCI_B_CTLW0_CKPL |  // polarity=1
    EUSCI_B_CTLW0_MST |             // Set as SPI master
    EUSCI_B_CTLW0_SYNC |            // Put in synchronous (SPI) mode
    EUSCI_B_CTLW0_MSB;              // MSB first

  //EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_SSEL__ACLK; // ACLK
  EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK
  EUSCI_B0->BRW = 0x03;                   // clock div=3, fBitClock = fBRCLK/(UCBRx+1).
  EUSCI_B0->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;// Initialize USCI state machine

  spi_rx_flag = 0x0;

  // Slave select
  P4->DIR |= BIT1; // set SS pin as output
  P4->OUT |= BIT1; // set high (SS inactive)

  // Enable eUSCI_B0 interrupt in NVIC module
  //__NVIC_EnableIRQ(EUSCIB0_IRQn);

}


// shift in 24 bits (last 20 bits will be command)
// read in 24 bits (first 20 bits will be response)?
uint32_t spi_send(uint32_t tx_data)
{

  uint32_t response = 0x0;

  uint8_t tx_byte;

  //sprintf(str, "\r\nSPI: Sending %0lx\r\n", tx_data);
  //uart_queue_str(str);

  P4->OUT &= ~BIT1; // enable slave (active low)

  tx_byte = tx_data >> 16;
  //sprintf(str, "\r\nSPI: Byte out %0x\r\n", tx_byte);
  //uart_queue_str(str);

  while (!(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG));
  EUSCI_B0->TXBUF = tx_byte; // send first 8-bits (4-bits of command)

  // Wait until shifted in
  while (!(EUSCI_B0->IFG & EUSCI_B_IFG_RXIFG));
  response = EUSCI_B0->RXBUF;

  while (!(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG));
  EUSCI_B0->TXBUF = (tx_data >> 8) & 0xff ; // next 8-bits

  response <<= 8;
  while (!(EUSCI_B0->IFG & EUSCI_B_IFG_RXIFG));
  response |= EUSCI_B0->RXBUF;

  while (!(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG));
  EUSCI_B0->TXBUF = (tx_data >> 16) & 0xff; // last 8 bits

  // read last 4 bits of response from top of received byte
  response <<= 4;
  while (!(EUSCI_B0->IFG & EUSCI_B_IFG_RXIFG));
  response |= (EUSCI_B0->RXBUF>>4);

  P4->OUT |= BIT1; // disable slave, latch command

  return response;
}

void spi1_init(void)
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

  // Slave select
  P4->DIR |= BIT1; // set SS pin as output
  P4->OUT |= BIT1; // set high (SS inactive)

  // Enable eUSCI_B0 interrupt in NVIC module
  //__NVIC_EnableIRQ(EUSCIB0_IRQn);

}

uint8_t spi1_send_byte(uint8_t tx_data)
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

// shift in 24 bits (last 20 bits will be command)
// read in 24 bits (first 20 bits will be response)?
uint32_t spi1_send(uint32_t tx_data)
{

  uint32_t response = 0x0;

  uint8_t tx_byte;

  uart_queue_str("\r\nSPI: Sending: ");
  uart_queue_hex(tx_data, 20);
  uart_queue_str("\r\n");

  P4->OUT &= ~BIT1; // enable slave (active low)

  tx_byte = tx_data >> 16;
  response = spi1_send_byte(tx_byte) << 12;

  tx_byte = (tx_data >> 8) & 0xff ; // next 8-bits
  response |= spi1_send_byte(tx_byte) << 4;

  tx_byte = tx_data & 0xff; // last 8 bits
  response |= spi1_send_byte(tx_byte) >> 4;

  P4->OUT |= BIT1; // disable slave, latch command

  uart_queue_str("SPI: Received: ");
  uart_queue_hex(response, 20);
  uart_queue_str("\r\n");

  return response;
}

void EUSCIB0_IRQHandler(void)
{
  if (EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG)
    {
      EUSCI_B0->TXBUF = spi_tx_data;           // Transmit characters
      EUSCI_B0->IE &= ~EUSCI_B__TXIE;

      // Wait till a character is received
      while (!(EUSCI_B0->IFG & EUSCI_B_IFG_RXIFG));
      spi_rx_data = EUSCI_B0->RXBUF;

      // Clear the receive interrupt flag
      EUSCI_B0->IFG &= ~EUSCI_B_IFG_RXIFG;
    }
}
