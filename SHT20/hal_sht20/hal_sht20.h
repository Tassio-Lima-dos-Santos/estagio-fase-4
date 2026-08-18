/******************************************************************************
 * File hal_sht20.h
 *
 *  Created on: 17 de ago. de 2026
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
#ifndef HAL_SHT20_HAL_SHT20_H_
#define HAL_SHT20_HAL_SHT20_H_



/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * Macros
 ******************************************************************************/

/*******************************************************************************
 * Defines
 ******************************************************************************/

/*******************************************************************************
 * Typedef & Enums
 ******************************************************************************/

typedef enum {
  // Success
  SHT20_RETURN_DONE = 0,

  // Fail
  SHT20_RETURN_INVALID_ARG = -1,
  SHT20_RETURN_INTERNAL_ERR = -2,
  SHT20_RETURN_I2C_ERR = -3,
  SHT20_RETURN_CORRUPT_DATA = -4,
  SHT20_RETURN_TIMEOUT = -5,

} hal_sht20_return_t;

typedef enum {
  HUMIDITY_12BITS_TEMPERATURE_14BITS,
  HUMIDITY_8BITS_TEMPERATURE_12BITS,
  HUMIDITY_10BITS_TEMPERATURE_13BITS,
  HUMIDITY_11BITS_TEMPERATURE_11BITS,
} temperature_and_humidity_res_t;

/*******************************************************************************
 * Externs
 ******************************************************************************/

/*******************************************************************************
 * Interface Functions
 ******************************************************************************/

hal_sht20_return_t SHT20_init(uint8_t scl_port, uint8_t scl_pin, uint8_t sda_port, uint8_t sda_pin);
hal_sht20_return_t measure_temperature(float *temperature);
hal_sht20_return_t measure_humidity(float *humidity);
hal_sht20_return_t enable_on_chip_heater(void);
hal_sht20_return_t disable_on_chip_heater(void);
hal_sht20_return_t enable_OTP_reload(void);
hal_sht20_return_t disable_OTP_reload(void);
hal_sht20_return_t check_battery(bool *is_battery_good);
hal_sht20_return_t change_measure_resolution(temperature_and_humidity_res_t resolution);
hal_sht20_return_t reset_SHT20(void);

/*******************************************************************************
 * END
 ******************************************************************************/

#endif /* HAL_SHT20_HAL_SHT20_H_ */
