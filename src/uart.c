// File       : uart.c
// Author     : Jeff Schornick
//
// MSP432 UART driver using interrupts and a pair of RX/TX buffers.
// The EUSCIA0 device is the UART channeled over USB to the host PC.
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stdint.h>
#include <stddef.h>
#include "msp432p401r.h"
#include "fifo.h"
#include "uart.h"

#define UART_TX_READY (EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG)
#define UART_IRQ_DISABLE (EUSCI_A0->IE &= ~(EUSCI_A_IE_TXIE|EUSCI_A_IE_RXIE))
#define UART_IRQ_ENABLE (EUSCI_A0->IE |= EUSCI_A_IE_TXIE|EUSCI_A_IE_RXIE)

fifo_t rx_fifo;
fifo_t tx_fifo;

// Function: uart_init
//
// Initializes the EUSCI A0 as a basic UART in 8n1 mode at 57.6K baud.
// Current configuration expectes a processor clock of 3MHz.
//
// Also initializes UART FIFOs for queued transmissions.
//
// See MSP432P4xx TRM section 22.3.1 for details on the initialization routine.
void uart_init(void)
{
  // Configure Port 1 pins to select UART module
  // RX : P1.2
  // TX : P1.3
  P1->SEL0 |= BIT2 | BIT3;
  P1->SEL1 &= ~(BIT2 | BIT3);

  // Reset device before configuration
  EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST;

  // UART Clock = SMCLK (low-speed subsystem master clock) = 3MHz
  EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SSEL__SMCLK | EUSCI_A_CTLW0_SWRST;

  // Baud rate calculation
  //   See MSP432P4xx TRM section 22.3.10
  //   baud_clock/baud_rate = 3MHz/57600 = 52.0833  > 16 (so use oversampling)
  //   52.083 / 16  = 3.255 -> BRW = 3
  //   3.255-3 * 16 = 4     -> BRF = 4
  // Calculator: http://processors.wiki.ti.com/index.php/USCI_UART_Baud_Rate_Gen_Mode_Selection
  //EUSCI_A0->BRW = 3;
  EUSCI_A0->BRW = 26;

  // MCTLW
  //   BRF = 4  (bits 7-4)
  //   BRS = 0  (bits 15-8)
  //   OS16 = 1 (bit0)
  //EUSCI_A0->MCTLW = EUSCI_A_MCTLW_OS16 | (4<<EUSCI_A_MCTLW_BRF_OFS);
  EUSCI_A0->MCTLW = EUSCI_A_MCTLW_OS16 | (1<<EUSCI_A_MCTLW_BRF_OFS);

  // clear reset to enable UART
  EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST;

  // reset flags (RX=0, TX=1) and enable interrupts
  EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;
  EUSCI_A0->IFG |=  EUSCI_A_IFG_TXIFG;

  //EUSCI_A0->IE |= EUSCI_A_IE_RXIE | EUSCI_A_IE_TXIE;
  EUSCI_A0->IE |= EUSCI_A_IE_RXIE;

  __NVIC_EnableIRQ(EUSCIA0_IRQn);

  fifo_init(&rx_fifo, UART_FIFO_SIZE);
  fifo_init(&tx_fifo, UART_FIFO_SIZE);

}

// Function: uart_putc
//
// Send a character directly out the UART.
void uart_putc(char c) {
  while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
  EUSCI_A0->TXBUF = c;
}

// prime the transmitter if ready and the fifo isn't empty
void uart_prime_tx(void) {
  char c;
  UART_IRQ_DISABLE;
  if (tx_fifo.count) {
    if (UART_TX_READY) {
      fifo_pop(&tx_fifo, &c);
      EUSCI_A0->TXBUF = c;
    }
  }
  UART_IRQ_ENABLE;;
}

// Function: uart_queue
//
// Queues a character for UART transmission
void uart_queue(char c) {
  fifo_push(&tx_fifo, c);
  uart_prime_tx();
}

// Function: uart_queue_str
//
// Queues a null-terminated string for UART transmission
void uart_queue_str(char *str) {
  while(*str != 0) {
    fifo_push(&tx_fifo, *str);
    str++;
  }
  uart_prime_tx();
}

// Function: uart_queue
//
// Queues a character for UART transmission
void uart_queue_uint8(uint8_t byte) {
  fifo_push(&tx_fifo, NIBBLE_TO_ASCII(byte>>4));
  fifo_push(&tx_fifo, NIBBLE_TO_ASCII(byte&0xf));
  uart_prime_tx();
}

void uart_queue_hex(uint32_t val, uint8_t bits)
{
  val &= ~((~0L)<<bits); // remove any extraneous bits
  // round to nearest nibble (multiple of 4)
  if(bits&0x3) {
    bits = (bits& ~(0x3)) + 4;
  }
  while(bits > 0) {
    bits -= 4;
    fifo_push(&tx_fifo, NIBBLE_TO_ASCII( (val>>bits) & 0xf));
  }
  uart_prime_tx();
}

void uart_queue_dec(uint32_t val) {
  char buf[11];  // max uint_32 is 10 digits
  uint8_t i = 10;

  buf[i] = '\0';
  do {
    i--;
    buf[i] = '0' + (val % 10);
    val /= 10;
  } while (val != 0);
  uart_queue_str(&buf[i]);
}

void uart_queue_sdec(int32_t val) {

  if (val < 0) {
    uart_queue('-');
    uart_queue_dec(-val);
  } else {
    uart_queue_dec(val);
  }
}

void EUSCIA0_IRQHandler(void)
{
  // FXIFG automatically cleared on read from RXBUF
  // TXIFG automatically cleared on write to TXBUF
  uint8_t flags = EUSCI_A0->IFG;
  char val;

  // Received a byte
  if (flags & EUSCI_A_IFG_RXIFG) {
    fifo_push(&rx_fifo, EUSCI_A0->RXBUF);
  }

  // Clear to send
  if (flags & EUSCI_A_IFG_TXIFG) {
    if(tx_fifo.count) {
      // Send next character, keep interrupt active
      fifo_pop(&tx_fifo, &val);
      EUSCI_A0->TXBUF = val;
    } else {
      // fifo empty, disable tx interrupt
      EUSCI_A0->IE &= ~EUSCI_A_IE_TXIE;
    }
  }
}
