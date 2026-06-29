/******************************************************************************
 * File alarm_state.c
 *
 *  Created on: 23 de jun. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "alarm_state.h"
#include "system_state.h"
#include "../NTC/fire_alarm.h"

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
void set_alarm_state(bool is_set, int16_t triggering_temperature, int16_t safe_temperature){
  if(is_set != false && is_set != true) return;

  state_variables.alarm_state.is_set = is_set;
  state_variables.alarm_state.is_triggered = false;
  state_variables.alarm_state.triggering_temperature = triggering_temperature;
  state_variables.alarm_state.safe_temperature = safe_temperature;
}

void sync_memory_and_IO_state_alarm(void){
  struct alarm_state current_alarm_state = state_variables.alarm_state;

  if(current_alarm_state.is_set == true){
    set_alarm(current_alarm_state.triggering_temperature, current_alarm_state.safe_temperature);
  }
  else if(current_alarm_state.is_set == false){
    disarm_alarm();
  }

  if(current_alarm_state.is_triggered == true){
    trigger_alarm();
  }
  else if(current_alarm_state.is_triggered == false){
    turn_off_alarm();
  }
}






