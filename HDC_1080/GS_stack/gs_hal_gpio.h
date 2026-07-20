/**
 ******************************************************************************
 * @Company	: Global Sonic
 * @file	: gs_gpio.h
 * @author	: mario
 * @date	: 4 de set. de 2023
 * @brief	: What this file does
 ******************************************************************************
 */

/******************************************************************************
 *							HOW TO USE
 *****************************************************************************
 Describe the steps required to use
 *****************************************************************************/

#ifndef GS_HAL_GPIO_H_
#define GS_HAL_GPIO_H_

/* -------------------------- Includes ---------------------------------------*/
/* -------------------------- Define -----------------------------------------*/

#ifndef HAL_GPIO_DEBOUNCE
#define HAL_GPIO_DEBOUNCE	250
#endif

#define GS_MAX_PERIOD	(HALF_MAX_INT32U_VALUE - 1)

/* -------------------------- Typedef ----------------------------------------*/
typedef void (*interruptCallback_t)(int8u port, int8u pin);

typedef struct {

	//OutPut Control - Configure the GPIO and set
	int8u (*setOutPut)(int8u port, int8u pin, int8u percent, int32u period,
				int32u interactions, boolean invert, boolean forceUpdate);


	//Configure the GPIO as Analog - the ref must be a "en_iadc_reference_t"
	int8u (*setAnalog)(int8u port, int8u pin, int8u ref, boolean inMilliVolts);
	//Read the IADC
	int32u (*analogRead)(int8u port, int8u pin);
	//Read the VDD
	int32u (*getVin)(void);

	//configure the interrupt to work with any edge
	int8u (*setInterrup)(int8u port, int8u pin, int8u pullUp, interruptCallback_t callback);
	//return the GPIO pin in state
	int8u (*getInPinState)(int8u port, int8u pin);
	//Disable the GPIO, so the interrupt don't work
	//This only works after the "setInterrut"
	int8u (*setDisableInterrup)(int8u port, int8u pin);
	//Enable the GPIO, so the interrupt work
	//This only works after the "setInterrut"
	int8u (*setEnableInterrup)(int8u port, int8u pin);

} st_gpio_class_t;

/* -------------------------- Public objects ---------------------------------*/

const st_gpio_class_t *gs_hal_gpio_class(void);

#endif /* GS_HAL_GPIO_H_ */
