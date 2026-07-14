/******************************************************************************
 * File NTC_sensor_state.h
 *
 *  Created on: 2 de jul. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/


/******************************************************************************
 * Description
 *
 * Usage        :
 * Known Errors :
 * ToDo         :
 *****************************************************************************/

/******************************************************************************
 * Multiple include protection
 **********************♠*******************************************************/
#ifndef STATE_HANDLING_NTC_SENSOR_STATE_H_
#define STATE_HANDLING_NTC_SENSOR_STATE_H_



/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "../NTC/NTC.h"
#include <stdbool.h>

/*******************************************************************************
 * Macros
 ******************************************************************************/

/*******************************************************************************
 * Defines
 ******************************************************************************/

#define INITIAL_DT_S 4
#define INITIAL_SIMULATED_TEMP 25.0f
#define INITIAL_FILTERED_TEMP 25.0f
#define INITIAL_RAMP_TEMPERATURE_STEP 1000

/*******************************************************************************
 * Typedef & Enums
 *******************************************************************************/

struct ntc_sensor_state {
  st_steps_information_t steps_info;
  st_ramp_information_t ramp_info;
  volatile float simulated_temp;
  volatile float tau_s;               // Low-pass filter's time constant
  volatile float dt_s;                // Data acquisition period
  volatile float temperature_filtered;
  enum_detector_class_t detector_class;
  bool is_temperature_simulated;
  bool is_temperature_filtered;
};

/*******************************************************************************
 * Externs
 ******************************************************************************/

/*******************************************************************************
 * Interface Functions
 ******************************************************************************/

void NTC_sensor_state_init (void);
void set_NTC_sensor_detector_class (enum_detector_class_t detector_class);

/*******************************************************************************
 * END
 ******************************************************************************/

#endif /* STATE_HANDLING_NTC_SENSOR_STATE_H_ */
