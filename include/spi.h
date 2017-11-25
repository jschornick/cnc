// File       : spi.h
// Author     : Jeff Schornick
//
// MSP432 SPI driver using interrupts and a pair of RX/TX buffers.
// EUSCIB0 is connected SPI bus for the TMC stepper drivers
//
// P1.5 : SCLK
// P1.6 : MOSI
// P1.7 : MISO
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#ifndef _SPI_H
#define _SPI_H

#include <stdint.h>

void spi_init(void);
uint32_t spi_send(uint32_t);

void spi1_init(void);
uint32_t spi1_send(uint32_t);

#endif /* _SPI_H */
