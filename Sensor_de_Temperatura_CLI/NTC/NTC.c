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

#ifdef TEMPERATURE_FILTER
// Filtro IIR de primeira ordem — equivale ao modelo térmico τ·dθ/dt + θ = T_ar
// tau_s = constante de tempo em segundos (ex: 20 s para A1)
// dt_s  = período de amostragem em segundos
#define TEMPERATURE_POLLING_PERIOD 4000 // 1000 ms between each polling
#define TAU_S   20.0f
#define DT_S    (TEMPERATURE_POLLING_PERIOD / 1000.0f)
#endif // TEMPERATURE_FILTER

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
#ifdef TEMPERATURE_FILTER
static sl_zigbee_event_t filtered_temperature_polling_event;
static volatile float temperature_filtered = 25.0f;  // initiates at room temperature
#endif // TEMPERATURE_FILTER

// Temperature simulation variables
static bool is_temperature_simulated = false;
static volatile float simulated_temp = 25.0f;

// Temperature ramp variables
static sl_zigbee_event_t ramp_simulation_step_event;
typedef struct ramp_information {
  volatile int32_t initial_temperature; // in °C
  volatile int32_t final_temperature;   // in °C
  volatile uint32_t duration;           // in seconds
  volatile uint32_t step;               // in ms
  volatile float temperature_rate;    // in K/s
} ramp_information_t;

static ramp_information_t ramp_info = {
  .step = 1000
};


/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static float map(float value, float in_min, float in_max, float out_min, float out_max);
static float NTC_milivoltage_to_resistance(float milivolts);
static float NTC_resistance_to_temperature(float resistance);
static float NTC_milivoltage_to_temperature(float milivolts);
static float NTC_read_raw_temperature(void);

#ifdef TEMPERATURE_FILTER
static void filtered_temperature_polling_event_handler(sl_zigbee_event_t *event);
#endif // TEMPERATURE_FILTER

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

#ifdef TEMPERATURE_FILTER
  sl_zigbee_event_init(&filtered_temperature_polling_event, filtered_temperature_polling_event_handler);
  sl_zigbee_event_set_delay_ms(&filtered_temperature_polling_event, TEMPERATURE_POLLING_PERIOD);
#endif
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

  if(is_temperature_simulated){
    temperature = simulated_temp;
  }
  else{
    float milivolts = IADC_read_milivolts();
    temperature = NTC_milivoltage_to_temperature(milivolts);
  }

  return (temperature);
}

float NTC_read_temperature(void){
#ifndef TEMPERATURE_FILTER
  return (NTC_read_raw_temperature());
#else // ifdef TEMPERATURE_FILTER
  return (temperature_filtered);
#endif
}

static float map(float value, float in_min, float in_max, float out_min, float out_max) {
  float result = (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  return (result);
}

#ifdef TEMPERATURE_FILTER
static void filtered_temperature_polling_event_handler(sl_zigbee_event_t *event){
  float k = DT_S / (TAU_S + DT_S);  // coeficiente do filtro
  float temp_raw = NTC_read_raw_temperature();

  temperature_filtered = k * temp_raw + (1.0f - k) * temperature_filtered;

  sl_zigbee_event_set_delay_ms(event, TEMPERATURE_POLLING_PERIOD);
}
#endif // TEMPERATURE_FILTER

void temperature_ramp_cli_callback  (sl_cli_command_arg_t *arguments){
  int32_t initial_temperature = sl_cli_get_argument_int32(arguments, 0);
  int32_t final_temperature = sl_cli_get_argument_int32(arguments, 1);
  uint32_t duration = sl_cli_get_argument_uint32(arguments, 2);

  ramp_info.initial_temperature = initial_temperature;
  ramp_info.final_temperature = final_temperature;
  ramp_info.duration = duration;
  ramp_info.temperature_rate = (float) (final_temperature - initial_temperature) / (float) duration;

  is_temperature_simulated = true;
  simulated_temp = ramp_info.initial_temperature;

  sl_zigbee_event_init(&ramp_simulation_step_event, ramp_simulation_step_event_handler);

  sl_zigbee_event_set_delay_ms(&ramp_simulation_step_event, ramp_info.step);
}

static void ramp_simulation_step_event_handler(sl_zigbee_event_t *event){
  bool finish_condition;
  if(ramp_info.temperature_rate > 0) finish_condition = (simulated_temp >= ramp_info.final_temperature);
  else finish_condition = (simulated_temp <= ramp_info.final_temperature);

  if(finish_condition){
    printf("Ramp simulation finished!\r\nFinal temperature: %f", simulated_temp);
    is_temperature_simulated = false;
    return;
  }

  simulated_temp += ramp_info.temperature_rate * (ramp_info.step / 1000.0f); // K/s * ms / 1000

  sl_zigbee_event_set_delay_ms(event, ramp_info.step);
}








