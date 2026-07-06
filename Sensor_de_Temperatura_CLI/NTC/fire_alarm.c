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
#include "circular_stack.h"
#include "../State_handling/system_state.h"
#include "../State_handling/alarm_state.h"
#include "sl_sleeptimer.h"
#include "zigbee_app_framework_event.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

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
static float temperature_data[2] = {0};

static float detector_class_to_min_response_temp_LUT[] = {54, 54, 69, 84, 99, 114, 129, 144};
static float detector_class_to_max_response_temp_LUT[] = {65, 70, 85, 100, 115, 130, 145, 160};
static float detector_class_to_typical_temp_LUT[] = {25, 25, 40, 55, 70, 85, 100, 115};

static int limite_inferior_resposta_LUT[] = {29, 8, 5, 1, 1, 1}; // Minutos de tempo de resposta com base na razão de elevação

static st_circular_stack_t last_minutes_measures_stack = {
    .size = 0,
    .head = 0,
    .tail = 0,
};

static st_circular_stack_t temperature_rate_stack = {
    .size = 0,
    .head = 0,
    .tail = 0,
};


static volatile uint32_t iso_test_begin_timestamp = 0;
static volatile uint32_t iso_test_final_timestamp = 0;

/******************************************************************************
 * Extern
 *****************************************************************************/

static struct ntc_sensor_state *NTC_state = &(state_variables.ntc_state);

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static void loop_temperature_event_handler(sl_zigbee_event_t *event);
static void temperature_verification_event_handler(sl_zigbee_event_t *event);

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

  float temperature = NTC_read_temperature();

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

void iso_test_simulation_cli_callback (sl_cli_command_arg_t *arguments){
  uint8_t detector_class_int = sl_cli_get_argument_uint8(arguments, 0);
  uint8_t temperature_rate = sl_cli_get_argument_uint8(arguments, 1);
  float duration;

  if(detector_class_int > 7){
    printf("Invalid detector class\r\n");
    return;
  }
  switch (temperature_rate) {
    case 1:
      duration = 2420;
      break;
    case 3:
      duration = 820;
      break;
    case 5:
      duration = 500;
      break;
    case 10:
      duration = 260;
      break;
    case 20:
      duration = 140;
      break;
    case 30:
      duration = 100;
      break;
    default:
      printf("Invalid temperature rate, use only those specified in the table\r\n");
      return;
      break;
  }

  stop_ramp_simulation();
  disarm_alarm();

  enum_detector_class_t detector_class = (enum_detector_class_t) detector_class_int;
  set_NTC_sensor_detector_class(detector_class);

  NTC_state->is_temperature_simulated = true;
  NTC_state->simulated_temp = detector_class_to_typical_temp_LUT[detector_class];
  NTC_state->temperature_filtered = detector_class_to_typical_temp_LUT[detector_class];

  set_alarm(
      (detector_class_to_min_response_temp_LUT[detector_class] + TRIGGER_TEMP_PADDING),
      (detector_class_to_typical_temp_LUT[detector_class] + SAFE_TEMP_PADDING)
      );

  iso_test_begin_timestamp = sl_sleeptimer_get_tick_count();

  start_ramp_simulation(detector_class_to_typical_temp_LUT[detector_class], detector_class_to_max_response_temp_LUT[detector_class], duration);
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
  turn_off_alarm();

  sl_zigbee_event_set_inactive(&temperature_verification_event);

  set_alarm_state(false, 0, 0);
}

static void loop_temperature_event_handler(sl_zigbee_event_t *event){
  float temperature = NTC_read_temperature();

  printf("\r\nCurrent temperature: %.1lf C\r\n", temperature);

  sl_zigbee_event_set_delay_ms(event, global_loop_temperature_period);
}

static void temperature_verification_event_handler(sl_zigbee_event_t *event){
  float triggering_temperature = temperature_data[0];
  float safe_temperature       = temperature_data[1];

  if( (triggering_temperature <= safe_temperature) || (safe_temperature < MIN_SAFE_TEMPERATURE) || (triggering_temperature > MAX_TRIGGERING_TEMPERATURE) ) return;

  float current_temperature = NTC_read_temperature();

#ifndef ALARM_TEMP_C

  if (current_temperature >= triggering_temperature) {
    trigger_alarm();
  } else if (current_temperature <= safe_temperature) {
    turn_off_alarm();
  }

#else // if def(ALARM_TEMP_C)

  if (temp >= ALARM_TEMP_C) {
    trigger_alarm();
  }

#endif

  sl_zigbee_event_set_delay_ms(event, global_temperature_verification_period);
}

void trigger_alarm (void){
  if(state_variables.alarm_state.is_triggered == true) return;

  state_variables.alarm_state.is_triggered = true;

  printf("Alarm triggered!!!\r\n");
  start_blink(ALARM_BLINK_PERIOD);

  if(iso_test_begin_timestamp != 0){
    iso_test_final_timestamp = sl_sleeptimer_get_tick_count();
    uint32_t freq = sl_sleeptimer_get_timer_frequency();
    float duration = (iso_test_final_timestamp - iso_test_begin_timestamp) / freq;

    printf("Response time: %f\r\n", duration);

    iso_test_begin_timestamp = 0;
    iso_test_final_timestamp = 0;
  }
}

void turn_off_alarm (void){
  if(state_variables.alarm_state.is_triggered == false) return;

  state_variables.alarm_state.is_triggered = false;

  printf("Safe temperature reached! Alarm turned off.\r\n");
  stop_blink();
}




