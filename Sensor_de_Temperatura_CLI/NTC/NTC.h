/******************************************************************************
 * File NTC.h
 *
 *  Created on: 19 de jun. de 2026
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
 *****************************************************************************/
#ifndef NTC_NTC_H_
#define NTC_NTC_H_

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "sl_cli_arguments.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/

/*******************************************************************************
 * Defines
 ******************************************************************************/

#define MAX_READABLE_TEMPERATURE      125
#define MIN_READABLE_TEMPERATURE      -40
#define MAX_SIMULATION_DURATION       3600 // 1 hora

/*******************************************************************************
 * Typedef & Enums
 ******************************************************************************/

typedef struct {
  volatile int32_t initial_temperature; // in °C
  volatile int32_t final_temperature;   // in °C
  volatile uint32_t duration;           // in seconds
  volatile uint32_t step;               // in ms
  volatile float temperature_rate;    // in K/s
} st_ramp_information_t;

typedef enum {
  DETECTOR_CLASS_A1 = 0,
  DETECTOR_CLASS_A2 = 1,
  DETECTOR_CLASS_B = 2,
  DETECTOR_CLASS_C = 3,
  DETECTOR_CLASS_D = 4,
  DETECTOR_CLASS_E = 5,
  DETECTOR_CLASS_F = 6,
  DETECTOR_CLASS_G = 7,
} enum_detector_class_t;

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/*******************************************************************************
 * Interface Functions
 ******************************************************************************/

void NTC_init                             (void);
float NTC_read_temperature                (void);
void start_ramp_simulation                (float initial_temperature, float temperature_rate, float duration); // Temperature rate in K/s and duration in s
void stop_ramp_simulation                 (void);
void temperature_ramp_cli_callback        (sl_cli_command_arg_t *arguments);
void disable_simulation_cli_callback      (sl_cli_command_arg_t *arguments);
void set_temperature_cli_callback         (sl_cli_command_arg_t *arguments);
void simulated_temperature_cli_callback   (sl_cli_command_arg_t *arguments);
void filtered_temperature_cli_callback    (sl_cli_command_arg_t *arguments);
void set_detector_class_cli_callback      (sl_cli_command_arg_t *arguments);

/*******************************************************************************
 * END
 ******************************************************************************/

#endif /* NTC_NTC_H_ */
