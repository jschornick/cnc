// File       : fifo.h
// Author     : Jeff Schornick
//
// FIFO data structure
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#ifndef __FIFO_H
#define __FIFO_H

#include <stdint.h>
#include <stddef.h>

#define FIFO_ERR 0
#define FIFO_OK 1

typedef struct
{
  volatile char *buffer;  /* The address of the data buffer allocation */
  volatile char *head;    /* Points to the latest item added */
  volatile char *tail;    /* Points one spot behind the earliest item added */
  size_t size;               /* Total number of items the buffer can store */
  volatile size_t count;     /* Current number of items being stored */
} fifo_t;

void fifo_init(fifo_t *fifo, size_t size);

uint8_t fifo_push(fifo_t *fifo, char val);

uint8_t fifo_pop(fifo_t *fifo, char *val);

#endif /* __FIFO_H */
