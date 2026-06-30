/******************************************************************************
 * File fire_alarm.c
 *
 *  Created on: 23 de jun. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "fire_alarm.h"
#include <stdint.h>
#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"
#include "NTC.h"
#include "../State_handling/system_state.h"
#include "../State_handling/alarm_state.h"
#include "sl_sleeptimer.h"
#include "zigbee_app_framework_event.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

// Periodic polling implementation

// ISO 7240-5 related defines
//#define ALARM_TEMP_C     58.0f  // 58 °C, 4 degrees above the minimum static temperature of response, for avoiding false positives
#define CONFIRM_COUNT    3      // 3 consecutive reads = 3 s

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/


static sl_zigbee_event_t temperature_verification_event;
static uint16_t global_temperature_verification_period;

static sl_zigbee_event_t loop_temperature_event;
static uint16_t global_loop_temperature_period;

// Array used for saving the data of triggering and safe temperature of a set alarm
static double temperature_data[2] = {0};

static uint8_t alarm_count = 0;

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static void loop_temperature_event_handler(sl_zigbee_event_t *event);
static void temperature_verification_event_handler(sl_zigbee_event_t *event);


void trigger_alarm (void);
void turn_off_alarm (void);

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

void get_temperature_cli_callback (sl_cli_command_arg_t *arguments){
  (void) arguments;

  double temperature = NTC_read_temperature();

  printf("Current temperature: %.1lf C\r\n", temperature);
}

void loop_temperature_cli_callback (sl_cli_command_arg_t *arguments){
  uint8_t enable = sl_cli_get_argument_uint8(arguments, 0);

  if(enable != 0 && enable != 1){
    printf("Invalid argument!\r\n");
    return;
  }

  if(enable == 1){
    if(sl_zigbee_event_is_scheduled(&loop_temperature_event)) sl_zigbee_event_set_inactive(&loop_temperature_event);

    sl_zigbee_event_init(&loop_temperature_event, loop_temperature_event_handler);

    global_loop_temperature_period = TEMPERATURE_VERIFICATION_PERIOD;

    sl_zigbee_event_set_delay_ms(&loop_temperature_event, global_loop_temperature_period);
  }
  else if(enable == 0){
    sl_zigbee_event_set_inactive(&loop_temperature_event);
  }
}

void set_alarm_cli_callback (sl_cli_command_arg_t *arguments){
  int32_t triggering_temperature = sl_cli_get_argument_int32(arguments, 0);
  int32_t safe_temperature = sl_cli_get_argument_int32(arguments, 1);

  if( (triggering_temperature <= safe_temperature) || (safe_temperature < MIN_SAFE_TEMPERATURE) || (triggering_temperature > MAX_TRIGGERING_TEMPERATURE) ){
    printf("Invalid temperature values\r\n");
    return;
  }

  set_alarm(triggering_temperature, safe_temperature);

  printf("Alarm is set!\r\n");
}

void disarm_alarm_cli_callback (sl_cli_command_arg_t *arguments){
  disarm_alarm();

  printf("Alarm is disarmed!\r\n");
}

void set_alarm (int32_t triggering_temperature, int32_t safe_temperature){
  if( (triggering_temperature <= safe_temperature) || (safe_temperature < MIN_SAFE_TEMPERATURE) || (triggering_temperature > MAX_TRIGGERING_TEMPERATURE) ) return;

  if(state_variables.alarm_state.is_set == true) disarm_alarm();

  set_alarm_state(true, triggering_temperature, safe_temperature);

  temperature_data[0] = triggering_temperature;
  temperature_data[1] = safe_temperature;

  sl_zigbee_event_init(&temperature_verification_event, temperature_verification_event_handler);

  global_temperature_verification_period = TEMPERATURE_VERIFICATION_PERIOD;

  sl_zigbee_event_set_delay_ms(&temperature_verification_event, global_temperature_verification_period);
}

void disarm_alarm (void){
  set_alarm_state(false, 0, 0);

  sl_zigbee_event_set_inactive(&temperature_verification_event);

  turn_off_alarm();
}

static void loop_temperature_event_handler(sl_zigbee_event_t *event){
  double temperature = NTC_read_temperature();

  printf("\r\nCurrent temperature: %.1lf C\r\n", temperature);

  sl_zigbee_event_set_delay_ms(event, global_loop_temperature_period);
}

static void temperature_verification_event_handler(sl_zigbee_event_t *event){
  double triggering_temperature = temperature_data[0];
  double safe_temperature       = temperature_data[1];

  if( (triggering_temperature <= safe_temperature) || (safe_temperature < MIN_SAFE_TEMPERATURE) || (triggering_temperature > MAX_TRIGGERING_TEMPERATURE) ) return;

  double current_temperature = NTC_read_temperature();

#ifndef ALARM_TEMP_C

  if (current_temperature >= triggering_temperature) {
    alarm_count++;
    if (alarm_count >= CONFIRM_COUNT) {
      trigger_alarm();
      alarm_count = 0;
    }
  } else if (current_temperature <= safe_temperature) {
      turn_off_alarm();
      alarm_count = 0;  // reseta se baixar
  }
  else {
    alarm_count = 0;  // reseta se baixar
  }

#else // if def(ALARM_TEMP_C)

  if (temp >= ALARM_TEMP_C) {
    alarm_count++;
    if (alarm_count >= CONFIRM_COUNT) {
      trigger_alarm();
      alarm_count = 0;  // reseta se baixar
    }
  } else {
      alarm_count = 0;  // reseta se baixar
  }

#endif

  sl_zigbee_event_set_delay_ms(event, global_temperature_verification_period);
}

void trigger_alarm (void){
  if(state_variables.alarm_state.is_triggered == true) return;

  state_variables.alarm_state.is_triggered = true;

  printf("Alarm triggered!!!\r\n");
  start_blink(ALARM_BLINK_PERIOD);
}

void turn_off_alarm (void){
  if(state_variables.alarm_state.is_triggered == false) return;

  state_variables.alarm_state.is_triggered = false;

  printf("Safe temperature reached! Alarm turned off.\r\n");
  stop_blink();
}






