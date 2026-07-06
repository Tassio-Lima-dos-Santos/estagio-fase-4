/******************************************************************************
 * File fire_alarm.h
 *
 *  Created on: 23 de jun. de 2026
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
#ifndef NTC_FIRE_ALARM_H_
#define NTC_FIRE_ALARM_H_



/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "NTC.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/

/*******************************************************************************
 * Defines
 ******************************************************************************/

#define TEMPERATURE_VERIFICATION_PERIOD 4000 // 4000 ms between each verification
#define MAX_TRIGGERING_TEMPERATURE      MAX_READABLE_TEMPERATURE
#define MIN_SAFE_TEMPERATURE            MIN_READABLE_TEMPERATURE
#define ALARM_BLINK_PERIOD              100
#define TRIGGER_TEMP_PADDING            1    // Padding between the minimal response temp to the actual trigger temp
#define SAFE_TEMP_PADDING               5    // Padding between the typical temp to the actual safe temp

/*******************************************************************************
 * Typedef & Enums
 ******************************************************************************/

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/*******************************************************************************
 * Interface Functions
 ******************************************************************************/

void get_temperature_cli_callback     (sl_cli_command_arg_t *arguments);
void loop_temperature_cli_callback    (sl_cli_command_arg_t *arguments);
void set_alarm_cli_callback           (sl_cli_command_arg_t *arguments);
void disarm_alarm_cli_callback        (sl_cli_command_arg_t *arguments);
void iso_test_simulation_cli_callback (sl_cli_command_arg_t *arguments);
void set_alarm                        (int32_t triggering_temperature, int32_t safe_temperature);
void disarm_alarm                     (void);
void trigger_alarm                    (void);
void turn_off_alarm                   (void);

/*******************************************************************************
 * END
 ******************************************************************************/

#endif /* NTC_FIRE_ALARM_H_ */
