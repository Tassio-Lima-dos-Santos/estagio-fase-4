/******************************************************************************
 * File NTC.c
 *
 *  Created on: 19 de jun. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "NTC.h"
#include "../State_handling/NTC_sensor_state.h"
#include "../State_handling/system_state.h"
#include "IADC.h"
#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"
#include "zigbee_app_framework_event.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

#define GS_NTC_TEMP {\
    {-30,127500},{-25,97100},{-20,74800},{-15,58080},{-10,45440},{-5,35820},\
    {0,28460},{5,22780},{10,18360},{15,14900},{20,12170},{25,10000},{30,8264},\
    {35,6890},{40,5738},{45,4810},{50,4064},{55,3448},{60,2934},\
    {65,2504},{70,2146},{75,1845},{80,1592},{85,1378},{90,1195},\
    {95,1039}, {100,966}, {105,795}, {110,703}\
}


#define VDD 3000 // 3000 mV
#define R_FIXO 10000 // 10 k ohm

/******************************************************************************
 * Data types
 *****************************************************************************/

typedef struct {
    float iTempCelsius;
    float dRntc;
} st_ntc_temp_t;

/******************************************************************************
 * Static Variables
 *****************************************************************************/

static const st_ntc_temp_t stNtcTempTable[] = GS_NTC_TEMP;

// Low-pass filter related variables
static sl_zigbee_event_t filtered_temperature_polling_event;

// Temperature ramp variables
static sl_zigbee_event_t ramp_simulation_step_event;


/******************************************************************************
 * Extern
 *****************************************************************************/

extern struct state_variables_singleton state_variables;
static struct ntc_sensor_state *NTC_state = &(state_variables.ntc_state);

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static float map(float value, float in_min, float in_max, float out_min, float out_max);
static float NTC_milivoltage_to_resistance(float milivolts);
static float NTC_resistance_to_temperature(float resistance);
static float NTC_milivoltage_to_temperature(float milivolts);
static float NTC_read_raw_temperature(void);


static void filtered_temperature_polling_event_handler(sl_zigbee_event_t *event);

static void ramp_simulation_step_event_handler(sl_zigbee_event_t *event);

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
void NTC_init(void){
  IADC_NTC_init();

  sl_zigbee_event_init(&filtered_temperature_polling_event, filtered_temperature_polling_event_handler);
  sl_zigbee_event_set_delay_ms(&filtered_temperature_polling_event, (NTC_state->dt_s * 1000));
}

static float NTC_milivoltage_to_resistance(float milivolts_NTC){
  // R_NTC = (V_NTC / (VDD - V_NTC) ) * R_FIXO

  float NTC_resistance = (milivolts_NTC / (VDD - milivolts_NTC) ) * R_FIXO;

  return (NTC_resistance);
}

static float NTC_resistance_to_temperature(float resistance_NTC){
  float reference_resistance_below = 0;
  float reference_temperature_below = 0;

  float reference_resistance_above = 0;
  float reference_temperature_above = 0;

  uint8_t LUT_length = sizeof(stNtcTempTable)/sizeof(stNtcTempTable[0]);
  for(int i = 1; i < LUT_length; i++){
    reference_resistance_above = stNtcTempTable[i - 1].dRntc;
    reference_resistance_below = stNtcTempTable[i].dRntc;

    if(reference_resistance_below < resistance_NTC && reference_resistance_above > resistance_NTC){
      reference_temperature_above = stNtcTempTable[i - 1].iTempCelsius;
      reference_temperature_below = stNtcTempTable[i].iTempCelsius;
      break;
    }
  }

  if(reference_temperature_below == 0) return (0);

  float temperature = map(resistance_NTC, reference_resistance_below, reference_resistance_above, reference_temperature_below, reference_temperature_above);

  return (temperature);
}

static float NTC_milivoltage_to_temperature(float milivolts){
  float resistance = NTC_milivoltage_to_resistance(milivolts);
  float temperature = NTC_resistance_to_temperature(resistance);

  return (temperature);
}

static float NTC_read_raw_temperature(void){
  float temperature;

  if(NTC_state->is_temperature_simulated){
    temperature = NTC_state->simulated_temp;
  }
  else{
    float milivolts = IADC_read_milivolts();
    temperature = NTC_milivoltage_to_temperature(milivolts);
  }

  return (temperature);
}

float NTC_read_temperature(void){
  float return_value;

  if(NTC_state->is_temperature_filtered){
    return_value = (NTC_state->temperature_filtered);
  }
  else{
    return_value = (NTC_read_raw_temperature());
  }

  if(return_value < MIN_READABLE_TEMPERATURE) return_value = MIN_READABLE_TEMPERATURE;
  if(return_value > MAX_READABLE_TEMPERATURE) return_value = MAX_READABLE_TEMPERATURE;

  return (return_value);
}

static float map(float value, float in_min, float in_max, float out_min, float out_max) {
  float result = (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  return (result);
}

static void filtered_temperature_polling_event_handler(sl_zigbee_event_t *event){
  float k = NTC_state->dt_s / (NTC_state->tau_s + NTC_state->dt_s);  // coeficiente do filtro
  float temp_raw = NTC_read_raw_temperature();

  NTC_state->temperature_filtered = k * temp_raw + (1.0f - k) * NTC_state->temperature_filtered;

  sl_zigbee_event_set_delay_ms(event, (NTC_state->dt_s * 1000));
}

void temperature_ramp_cli_callback(sl_cli_command_arg_t *arguments){
  int32_t initial_temperature = sl_cli_get_argument_int32(arguments, 0);
  int32_t final_temperature = sl_cli_get_argument_int32(arguments, 1);
  uint32_t duration = sl_cli_get_argument_uint32(arguments, 2);

  bool is_initial_temperature_valid = (initial_temperature > MIN_READABLE_TEMPERATURE && initial_temperature < MAX_READABLE_TEMPERATURE);
  bool is_final_temperature_valid = (final_temperature > MIN_READABLE_TEMPERATURE && final_temperature < MAX_READABLE_TEMPERATURE);
  bool is_duration_valid = (duration < MAX_SIMULATION_DURATION);

  if(!(is_initial_temperature_valid && is_final_temperature_valid && is_duration_valid)){
    printf("Invalid arguments!\r\n");
    return;
  }

  start_ramp_simulation(initial_temperature, final_temperature, duration);
  printf("Ramp simulation started!\r\n");
}

void disable_simulation_cli_callback(sl_cli_command_arg_t *arguments){
  stop_ramp_simulation();

  NTC_state->is_temperature_simulated = false;
}

void set_temperature_cli_callback(sl_cli_command_arg_t *arguments){
  int32_t set_temperature = sl_cli_get_argument_int32(arguments, 0);

  NTC_state->is_temperature_simulated = true;
  NTC_state->simulated_temp = set_temperature;

  printf("Temperature set to %ld C\r\n", set_temperature);
}

void simulated_temperature_cli_callback(sl_cli_command_arg_t *arguments){
  uint8_t enable = sl_cli_get_argument_uint8(arguments, 0);

  if(enable == 1){
    NTC_state->is_temperature_simulated = true;
    printf("Temperature simulation enabled!\r\n");
  }
  else if(enable == 0){
    NTC_state->is_temperature_simulated = false;
    printf("Temperature simulation disabled!\r\n");
  }
  else{
    printf("Incorrect argument - enable: <0|1>\r\n");
  }
}

void set_detector_class_cli_callback(sl_cli_command_arg_t *arguments){
  uint8_t detector_class_int = sl_cli_get_argument_uint8(arguments, 0);

  if(detector_class_int > 7){
    printf("Invalid detector class\r\n");
    return;
  }

  enum_detector_class_t detector_class = (enum_detector_class_t) detector_class_int;
  set_NTC_sensor_detector_class(detector_class);

  printf("Detector class set successfully!\r\n");
}



static void ramp_simulation_step_event_handler(sl_zigbee_event_t *event){
  bool finish_condition;
  st_ramp_information_t *ramp_info = &(NTC_state->ramp_info);

  if(ramp_info->temperature_rate > 0) finish_condition = (NTC_state->simulated_temp >= ramp_info->final_temperature);
  else finish_condition = (NTC_state->simulated_temp <= ramp_info->final_temperature);

  if(finish_condition){
    printf("Ramp simulation finished!\r\nFinal temperature: %.1f\r\n", NTC_state->simulated_temp);
    return;
  }

  printf("Temperature simulated: %.1f\r\n", NTC_state->simulated_temp);

  NTC_state->simulated_temp += ramp_info->temperature_rate * (ramp_info->step / 1000.0f); // K/s * ms / 1000

  sl_zigbee_event_set_delay_ms(event, ramp_info->step);
}

void start_ramp_simulation(float initial_temperature, float final_temperature, float duration){
  stop_ramp_simulation();

  st_ramp_information_t *ramp_info = &(NTC_state->ramp_info);
  ramp_info->initial_temperature = initial_temperature;
  ramp_info->final_temperature = final_temperature;
  ramp_info->duration = duration;
  ramp_info->temperature_rate = (float) (final_temperature - initial_temperature) / (float) duration;

  NTC_state->is_temperature_simulated = true;
  NTC_state->simulated_temp = ramp_info->initial_temperature;

  sl_zigbee_event_init(&ramp_simulation_step_event, ramp_simulation_step_event_handler);

  sl_zigbee_event_set_delay_ms(&ramp_simulation_step_event, ramp_info->step);
}

void stop_ramp_simulation(void){
  if(sl_zigbee_event_is_scheduled(&ramp_simulation_step_event)){
    sl_zigbee_event_set_inactive(&ramp_simulation_step_event);
  }
}






