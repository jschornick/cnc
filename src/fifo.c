// File       : fifo.c
// Author     : Jeff Schornick
//
// FIFO data structure
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include <stddef.h>
#include <stdlib.h> /* malloc */
#include "msp432p401r.h"
#include "fifo.h"

// Function: fifo_init
//
// Initializes a static-sized FIFO queue. Buffer storage is dynamically
// allocated based on the size parameter.
void fifo_init(fifo_t *fifo, size_t size)
{
  fifo->buffer = malloc(size);
  fifo->head = fifo->buffer;
  fifo->tail = fifo->buffer;
  fifo->size = size;
  fifo->count = 0;
}

// Function: fifo_push
//
// Pushes a new item onto the FIFO, unless it is full.
uint8_t fifo_push(fifo_t *fifo, char val)
{
  if( fifo->count == fifo->size ) {
    return FIFO_ERR;
  }
  __disable_irq();
  if( ++(fifo->head) == (fifo->buffer + fifo->size) )
    {
      fifo->head = fifo->buffer;
    }
  *(fifo->head) = val;
  fifo->count++;
  __enable_irq();
  return FIFO_OK;
}

// Function: fifo_pop
//
// Pops the head of the FIFO queue and returns it in val.
// Val will be unchanged if called with an empty queue.
uint8_t fifo_pop(fifo_t *fifo, char *val)
{
  if( fifo->count == 0 ) {
    return FIFO_ERR;
  }
  __disable_irq();
  if( ++(fifo->tail) == (fifo->buffer + fifo->size) )
    {
      fifo->tail = fifo->buffer;
    }
  *val = *(fifo->tail);
  fifo->count--;
  __enable_irq();
  return FIFO_OK;
}

uint8_t fifo_empty(fifo_t *fifo)
{
  return (fifo->count == 0);
}
