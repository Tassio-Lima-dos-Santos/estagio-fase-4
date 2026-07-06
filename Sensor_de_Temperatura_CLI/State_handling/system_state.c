/******************************************************************************
 * File system_state.c
 *
 *  Created on: 23 de jun. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "system_state.h"
#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "nvm3_app.h"
#include "../LED_handling/blink.h"

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/

/******************************************************************************
 * Extern
 *****************************************************************************/

struct state_variables_singleton state_variables;

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
void sync_memory_and_IO_state();

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
void system_state_init(void){
  led_state_init();
  alarm_state_init();
  NTC_sensor_state_init();
}

void save_cli_callback(sl_cli_command_arg_t *arguments){
  (void) arguments;

  save_state_to_flash();
}

void erase_cli_callback(sl_cli_command_arg_t *arguments){
  (void) arguments;

  erase_state_in_flash();
}

void load_cli_callback(sl_cli_command_arg_t *arguments){
  (void) arguments;

  load_memory_state_from_flash();

  sync_memory_and_IO_state();
}

/*******************************************************************************
 * Function name: sync_memory_and_IO_state
 *
 * Description  : Syncronizes the IO states of the LED, how they are actually behaving, with the memory states,
 *                how the MCU thinks they are behaving
 * Parameters   : void
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
void sync_memory_and_IO_state(void){
  sync_memory_and_IO_state_led();
  sync_memory_and_IO_state_alarm();
}
