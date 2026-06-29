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
// #define USE_SLEEPTIMER
#define USE_ZIGBEE_EVENT

// ISO 7240-5 related defines
//#define ALARM_TEMP_C     58.0f  // 58 °C, 4 degrees above the minimum static temperature of response, for avoiding false positives
#define CONFIRM_COUNT    3      // 3 consecutive reads = 3 s
//#define TEMPERATURE_FILTER

#ifdef TEMPERATURE_FILTER
// Filtro IIR de primeira ordem — equivale ao modelo térmico τ·dθ/dt + θ = T_ar
// tau_s = constante de tempo em segundos (ex: 20 s para A1)
// dt_s  = período de amostragem em segundos

#define TAU_S   20.0f
#define DT_S    1.0f
#endif // TEMPERATURE_FILTER

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/

#ifdef USE_SLEEPTIMER

static sl_sleeptimer_timer_handle_t timer_temperature_verification;
static sl_sleeptimer_timer_handle_t timer_loop_temperature;

#elif defined(USE_ZIGBEE_EVENT)

static sl_zigbee_event_t temperature_verification_event;
static uint16_t global_temperature_verification_period;

static sl_zigbee_event_t loop_temperature_event;
static uint16_t global_loop_temperature_period;

#endif // defined(USE_ZIGBEE_EVENT)

// Array used for saving the data of triggering and safe temperature of a set alarm
static double temperature_data[2] = {0};

static uint8_t alarm_count = 0;

#ifdef TEMPERATURE_FILTER
static float temperature_filtered = 25.0f;  // começa na temp ambiente
#endif // TEMPERATURE_FILTER

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

#ifdef USE_SLEEPTIMER

void on_timeout_loop_temperature (sl_sleeptimer_timer_handle_t *handle,
                       void *data);
void on_timeout_temperature_verification (sl_sleeptimer_timer_handle_t *handle,
                       void *data);

#elif defined(USE_ZIGBEE_EVENT)

static void loop_temperature_event_handler(sl_zigbee_event_t *event);
static void temperature_verification_event_handler(sl_zigbee_event_t *event);

#endif // defined(USE_ZIGBEE_EVENT)

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
#ifdef USE_SLEEPTIMER

    sl_sleeptimer_start_periodic_timer_ms(&timer_loop_temperature,
                                             TEMPERATURE_VERIFICATION_PERIOD,
                                             on_timeout_loop_temperature,
                                             NULL,
                                             0,
                                             SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);

#elif defined(USE_ZIGBEE_EVENT)

    sl_zigbee_event_init(&loop_temperature_event, loop_temperature_event_handler);

    global_loop_temperature_period = TEMPERATURE_VERIFICATION_PERIOD;

    sl_zigbee_event_set_delay_ms(&loop_temperature_event, global_loop_temperature_period);

#endif // defined(USE_ZIGBEE_EVENT)

  }
  else if(enable == 0){
#ifdef USE_SLEEPTIMER

    sl_sleeptimer_stop_timer(&timer_loop_temperature);

#elif defined(USE_ZIGBEE_EVENT)

    sl_zigbee_event_set_inactive(&loop_temperature_event);

#endif // defined(USE_ZIGBEE_EVENT)
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

#ifdef USE_SLEEPTIMER

  sl_sleeptimer_start_periodic_timer_ms(&timer_temperature_verification,
                                         TEMPERATURE_VERIFICATION_PERIOD,
                                         on_timeout_temperature_verification,
                                         (void *) &temperature_data,
                                         0,
                                         SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);

#elif defined(USE_ZIGBEE_EVENT)

  sl_zigbee_event_init(&temperature_verification_event, temperature_verification_event_handler);

  global_temperature_verification_period = TEMPERATURE_VERIFICATION_PERIOD;

  sl_zigbee_event_set_delay_ms(&temperature_verification_event, global_temperature_verification_period);

#endif // defined(USE_ZIGBEE_EVENT)
}

void disarm_alarm (void){
  set_alarm_state(false, 0, 0);

#ifdef USE_SLEEPTIMER

  sl_sleeptimer_stop_timer(&timer_temperature_verification);

#elif defined(USE_ZIGBEE_EVENT)

  sl_zigbee_event_set_inactive(&temperature_verification_event);

#endif // defined(USE_ZIGBEE_EVENT)

  turn_off_alarm();
}

#ifdef USE_SLEEPTIMER
void on_timeout_temperature_verification (sl_sleeptimer_timer_handle_t *handle, void *data){
  (void) handle;
  double triggering_temperature = ((double *) data)[0];
  double safe_temperature       = ((double *) data)[1];

  if( (triggering_temperature <= safe_temperature) || (safe_temperature < MIN_SAFE_TEMPERATURE) || (triggering_temperature > MAX_TRIGGERING_TEMPERATURE) ) return;

  double current_temperature = NTC_read_temperature();

  if(current_temperature >= triggering_temperature) trigger_alarm();
  else if(current_temperature <= safe_temperature) turn_off_alarm();
}

void on_timeout_loop_temperature (sl_sleeptimer_timer_handle_t *handle, void *data){
  (void) data;
  (void) handle;

  double temperature = NTC_read_temperature();

  printf("\r\nCurrent temperature: %.1lf C\r\n", temperature);
}

#elif defined(USE_ZIGBEE_EVENT)

static void loop_temperature_event_handler(sl_zigbee_event_t *event){
#ifndef TEMPERATURE_FILTER
  double temperature = NTC_read_temperature();
#else // ifdef TEMPERATURE_FILTER
  float alpha = DT_S / (TAU_S + DT_S);  // coeficiente do filtro
  float temp_raw = NTC_read_temperature();

  temperature_filtered = alpha * temp_raw + (1.0f - alpha) * temperature_filtered;

  double temperature = temperature_filtered;
#endif // TEMPERATURE_FILTER

  printf("\r\nCurrent temperature: %.1lf C\r\n", temperature);

  sl_zigbee_event_set_delay_ms(event, global_loop_temperature_period);
}

static void temperature_verification_event_handler(sl_zigbee_event_t *event){
  double triggering_temperature = temperature_data[0];
  double safe_temperature       = temperature_data[1];

  if( (triggering_temperature <= safe_temperature) || (safe_temperature < MIN_SAFE_TEMPERATURE) || (triggering_temperature > MAX_TRIGGERING_TEMPERATURE) ) return;

#ifndef TEMPERATURE_FILTER
  double current_temperature = NTC_read_temperature();
#else // ifdef TEMPERATURE_FILTER
  float alpha = DT_S / (TAU_S + DT_S);  // coeficiente do filtro
  float temp_raw = NTC_read_temperature();

  temperature_filtered = alpha * temp_raw + (1.0f - alpha) * temperature_filtered;

  double current_temperature = temperature_filtered;
#endif // TEMPERATURE_FILTER

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

#endif // defined(USE_ZIGBEE_EVENT)

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






