/******************************************************************************
 * File LED_state.c
 *
 *  Created on: 12 de jun. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "LED_state.h"
#include "system_state.h"

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
void set_led_state(enum led_mode mode, uint16_t data){
  switch (mode) {
    case LED_IDLE:
      state_variables.led_state.mode = mode;
      state_variables.led_state.set = false;
      break;
    case LED_SET:
      state_variables.led_state.mode = mode;
      state_variables.led_state.set = (bool) data;
      break;
    case LED_BLINK:
      state_variables.led_state.mode = mode;
      state_variables.led_state.period = (uint16_t) data;
      break;
    default:
      break;
  }
}

void sync_memory_and_IO_state_led(void){
  struct led_state current_led_state = state_variables.led_state;

  switch(current_led_state.mode){
    case LED_IDLE:
      sl_led_sinalizacao.turn_off(sl_led_sinalizacao.context);
      break;

    case LED_SET:

      if(current_led_state.set) sl_led_sinalizacao.turn_on(sl_led_sinalizacao.context);
      else                      sl_led_sinalizacao.turn_off(sl_led_sinalizacao.context);
      break;
    case LED_BLINK:

      start_blink(current_led_state.period);

      break;
    default:

      break;
  }
}









