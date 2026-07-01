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

#include "sl_cli_arguments.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/

/*******************************************************************************
 * Defines
 ******************************************************************************/

//#define SOLUCAO_CALCULO
#define SOLUCAO_TABELA

#define TEMPERATURE_VERIFICATION_PERIOD 4000 // 4000 ms between each verification
#define MAX_TRIGGERING_TEMPERATURE      150
#define MIN_SAFE_TEMPERATURE            -50
#define ALARM_BLINK_PERIOD              100

/*******************************************************************************
 * Typedef & Enums
 ******************************************************************************/

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/*******************************************************************************
 * Interface Functions
 ******************************************************************************/

void get_temperature_cli_callback   (sl_cli_command_arg_t *arguments);
void loop_temperature_cli_callback  (sl_cli_command_arg_t *arguments);
void set_alarm_cli_callback         (sl_cli_command_arg_t *arguments);
void disarm_alarm_cli_callback      (sl_cli_command_arg_t *arguments);
void set_alarm                      (int32_t triggering_temperature, int32_t safe_temperature);
void disarm_alarm                   (void);
void trigger_alarm                  (void);
void turn_off_alarm                 (void);

/*******************************************************************************
 * END
 ******************************************************************************/

#endif /* NTC_FIRE_ALARM_H_ */
