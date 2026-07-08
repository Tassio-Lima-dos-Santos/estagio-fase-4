/******************************************************************************
 * File circular_stack.c
 *
 *  Created on: 6 de jul. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "circular_stack.h"
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*******************************************************************************
 * Function name:
 *
 * Description  :
 * Parameters   :
 * Returns      :
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
float circular_stack_pop(st_circular_stack_t *self){
  if(self == NULL) return 0;
  if(self->size == 0) return 0;

  if(self->head == 0){
    self->head = CIRCULAR_STACK_SIZE - 1;
  }
  else if(self->head != 0){
    self->head = self->head - 1;
  }

  float return_value = self->array[self->head];

  self->size--;
  return (return_value);
}

float circular_stack_peek(st_circular_stack_t *self, int position){
  if(self == NULL) return 0;
  if(position < 0 || position >= self->size) return 0;

  int index = self->head - (position + 1);
  if(index < 0){
    index += CIRCULAR_STACK_SIZE;
  }

  float return_value = self->array[index];

  return (return_value);
}

void circular_stack_push(st_circular_stack_t *self, float data){
  if(self == NULL) return;

  // Caso a fila fique cheia
  if( (((self->head) % CIRCULAR_STACK_SIZE) == self->tail) && self->size != 0){
    self->tail = (self->tail + 1) % CIRCULAR_STACK_SIZE;
    self->size--;
  }

  self->array[self->head] = data;
  self->head = (self->head + 1) % CIRCULAR_STACK_SIZE;

  self->size++;
}

void circular_stack_clear(st_circular_stack_t *self){
  if(self == NULL) return;

  self->size = 0;
  self->head = 0;
  self->tail = 0;
  memset(self->array, 0, sizeof(self->array));
}
