/******************************************************************************
 * File NTC_sensor_state.c
 *
 *  Created on: 2 de jul. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "NTC_sensor_state.h"
#include "system_state.h"

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/

static float detector_class_to_tau_LUT[] = {10, 40, 40, 40, 40, 40, 40, 40};

/******************************************************************************
 * Extern
 *****************************************************************************/

extern struct state_variables_singleton state_variables;

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
void NTC_sensor_state_init (void){
  set_NTC_sensor_detector_class(DETECTOR_CLASS_A1);
  state_variables.ntc_state.dt_s = INITIAL_DT_S;
  state_variables.ntc_state.is_temperature_filtered = false;
  state_variables.ntc_state.is_temperature_simulated = true;
  state_variables.ntc_state.simulated_temp = INITIAL_SIMULATED_TEMP;
  state_variables.ntc_state.temperature_filtered = INITIAL_FILTERED_TEMP;

  state_variables.ntc_state.steps_info.amount_steps = 0;
  state_variables.ntc_state.steps_info.current_step = 0;
  state_variables.ntc_state.steps_info.step_duration = 0;
  for(int i = 0; i < STEPS_ARRAY_SIZE; i++) state_variables.ntc_state.steps_info.steps_array[i] = 0;

  state_variables.ntc_state.ramp_info.duration = 0;
  state_variables.ntc_state.ramp_info.final_temperature = 0;
  state_variables.ntc_state.ramp_info.initial_temperature = 0;
  state_variables.ntc_state.ramp_info.step = INITIAL_RAMP_TEMPERATURE_STEP;
  state_variables.ntc_state.ramp_info.temperature_rate = 0;
}

void set_NTC_sensor_detector_class (enum_detector_class_t detector_class){
  state_variables.ntc_state.detector_class = detector_class;
  state_variables.ntc_state.tau_s = detector_class_to_tau_LUT[detector_class];
}










