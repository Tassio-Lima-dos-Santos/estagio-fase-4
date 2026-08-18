/******************************************************************************
 * File HDC_1080_sensor_config.h
 *
 *  Created on: 15 de jul. de 2026
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
#ifndef HDC_HAL_HDC_1080_SENSOR_CONFIG_H_
#define HDC_HAL_HDC_1080_SENSOR_CONFIG_H_



/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "em_gpio.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/

/*******************************************************************************
 * Defines
 ******************************************************************************/

#ifndef HDC_1080_SENSOR_PERIPHERAL
#define HDC_1080_SENSOR_PERIPHERAL              I2C0
#endif // HDC_1080_SENSOR_PERIPHERAL

// HDC 1080 ENABLE on PB01
#ifndef HDC_1080_SENSOR_ENABLE_PORT
#define HDC_1080_SENSOR_ENABLE_PORT                gpioPortB
#endif // HDC_1080_SENSOR_ENABLE_PORT
#ifndef HDC_1080_SENSOR_ENABLE_PIN
#define HDC_1080_SENSOR_ENABLE_PIN                 0
#endif // HDC_1080_SENSOR_ENABLE_PIN

// I2C0 SCL on PC02
#ifndef HDC_1080_SENSOR_SCL_PORT
#define HDC_1080_SENSOR_SCL_PORT                gpioPortC
#endif // HDC_1080_SENSOR_SCL_PORT
#ifndef HDC_1080_SENSOR_SCL_PIN
#define HDC_1080_SENSOR_SCL_PIN                 2
#endif // HDC_1080_SENSOR_SCL_PIN

// I2C0 SDA on PC03
#ifndef HDC_1080_SENSOR_SDA_PORT
#define HDC_1080_SENSOR_SDA_PORT                gpioPortC
#endif // HDC_1080_SENSOR_SDA_PORT
#ifndef HDC_1080_SENSOR_SDA_PIN
#define HDC_1080_SENSOR_SDA_PIN                 3
#endif // HDC_1080_SENSOR_SDA_PIN

/*******************************************************************************
 * Typedef & Enums
 ******************************************************************************/

/*******************************************************************************
 * Externs
 ******************************************************************************/

/*******************************************************************************
 * Interface Functions
 ******************************************************************************/

/*******************************************************************************
 * END
 ******************************************************************************/

#endif /* HDC_HAL_HDC_1080_SENSOR_CONFIG_H_ */
