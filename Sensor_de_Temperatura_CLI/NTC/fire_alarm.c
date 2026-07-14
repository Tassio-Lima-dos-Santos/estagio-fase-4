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

/* --------------------------------- Events --------------------------------- */
static sl_zigbee_event_t alarm_verification_event;
static sl_zigbee_event_t loop_temperature_event;

/* ------------------------------- Events Data ------------------------------ */
static float temperature_data[2] = {0}; // Array used for saving the data of triggering and safe temperature of a set alarm

/* ------------------------------ Look-Up Table ----------------------------- */
static const float detector_class_to_min_response_temp_LUT[] = {54, 54, 69, 84, 99, 114, 129, 144};
//static const float detector_class_to_max_response_temp_LUT[] = {65, 70, 85, 100, 115, 130, 145, 160};
static const float detector_class_to_typical_temp_LUT[] = {25, 25, 40, 55, 70, 85, 100, 115};
static const float temperature_rate_LUT[] = {1, 3, 5, 10, 20, 30};
static const int minimum_response_time_LUT[2][6] = {
    {1, 1, 1, 5, 8, 29}, // Minutos de tempo de resposta com base na razão de elevação
    {1, 1, 2, 5, 8, 29}, // Minutos de tempo de resposta com base na razão de elevação
};

static float average_temperature_rate_array[6] = {0};

static st_circular_stack_t historical_temperature_stack;

static float current_temperature;
static int   temperature_rate_count = 0;

static uint32_t iso_test_begin_timestamp = 0;
static uint32_t iso_test_final_timestamp = 0;

/******************************************************************************
 * Extern
 *****************************************************************************/

static struct ntc_sensor_state *NTC_state = &(state_variables.ntc_state);

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/* ----------------------------- Event Handlers ----------------------------- */
static void loop_temperature_event_handler(sl_zigbee_event_t *event);
static void alarm_verification_event_handler(sl_zigbee_event_t *event);

static void update_average_temperature_rate_array(st_circular_stack_t *temperature_stack);

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

    sl_zigbee_event_set_delay_ms(&loop_temperature_event, TEMPERATURE_VERIFICATION_PERIOD);
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

void show_average_temp_rates_cli_callback(sl_cli_command_arg_t *arguments){
  const int* minimum_response_time_array;
  if(NTC_state->detector_class == DETECTOR_CLASS_A1) minimum_response_time_array = minimum_response_time_LUT[0];
  else minimum_response_time_array = minimum_response_time_LUT[1];

  int array_size = sizeof(average_temperature_rate_array) / sizeof(average_temperature_rate_array[0]);
  for(int i = 2; i < array_size; i++){
    printf("Average temperature rate in the last %d minutes: %.2f K/min\r\n", minimum_response_time_array[i], average_temperature_rate_array[i]);
  }
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

  start_ramp_simulation(detector_class_to_typical_temp_LUT[detector_class], temperature_rate / 60.0f, duration);
}

void set_alarm (int32_t triggering_temperature, int32_t safe_temperature){
  if( (triggering_temperature <= safe_temperature) || (safe_temperature < MIN_SAFE_TEMPERATURE) || (triggering_temperature > MAX_TRIGGERING_TEMPERATURE) ) return;

  if(state_variables.alarm_state.is_set == true) disarm_alarm();

  set_alarm_state(true, triggering_temperature, safe_temperature);

  temperature_data[0] = triggering_temperature;
  temperature_data[1] = safe_temperature;

  sl_zigbee_event_init(&alarm_verification_event, alarm_verification_event_handler);

  sl_zigbee_event_set_delay_ms(&alarm_verification_event, TEMPERATURE_VERIFICATION_PERIOD);
}

void disarm_alarm (void){
  turn_off_alarm();

  temperature_rate_count = 0;
  memset(average_temperature_rate_array, 0, sizeof(average_temperature_rate_array));
  circular_stack_clear(&historical_temperature_stack);
  sl_zigbee_event_set_inactive(&alarm_verification_event);

  set_alarm_state(false, 0, 0);
}

static void loop_temperature_event_handler(sl_zigbee_event_t *event){
  float temperature = NTC_read_temperature();

  printf("\r\nCurrent temperature: %.1lf C\r\n", temperature);

  sl_zigbee_event_set_delay_ms(event, TEMPERATURE_VERIFICATION_PERIOD);
}

static void alarm_verification_event_handler(sl_zigbee_event_t *event){
  float triggering_temperature = temperature_data[0];
  float safe_temperature       = temperature_data[1];

  if( (triggering_temperature <= safe_temperature) || (safe_temperature < MIN_SAFE_TEMPERATURE) || (triggering_temperature > MAX_TRIGGERING_TEMPERATURE) ) return;

  current_temperature = NTC_read_temperature();

  temperature_rate_count += (TEMPERATURE_VERIFICATION_PERIOD / 1000);
  // A cada 1 minuto atualiza o array de average_temperature_rate
  if(temperature_rate_count >= 60){
    temperature_rate_count = 0;

    if(historical_temperature_stack.is_initialized != true) circular_stack_init(&historical_temperature_stack);

    circular_stack_push(&historical_temperature_stack, current_temperature);
    update_average_temperature_rate_array(&historical_temperature_stack);

    for(int i = 5; i >= 0; i--){
      if( average_temperature_rate_array[5 - i] > (temperature_rate_LUT[i] - 0.5) ){
        trigger_alarm();
        break;
      }
    }

  }

  if (current_temperature >= triggering_temperature) {
    trigger_alarm();
  }

  sl_zigbee_event_set_delay_ms(event, TEMPERATURE_VERIFICATION_PERIOD);
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

    printf("Response time: %.0f s\r\n", duration);

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

static void update_average_temperature_rate_array(st_circular_stack_t *temperature_stack){
  const int* minimum_response_time_array;
  if(NTC_state->detector_class == DETECTOR_CLASS_A1) minimum_response_time_array = minimum_response_time_LUT[0];
  else minimum_response_time_array = minimum_response_time_LUT[1];

  int array_size = sizeof(average_temperature_rate_array) / sizeof(average_temperature_rate_array[0]);
  for(int i = 0; i < array_size; i++){
    int period = minimum_response_time_array[i];
    float new_temperature = circular_stack_peek(temperature_stack, 0);
    float old_temperature = circular_stack_peek(temperature_stack, period);

    average_temperature_rate_array[i] = (new_temperature - old_temperature) / period;
  }
}


















