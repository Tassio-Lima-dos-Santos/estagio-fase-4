/**
 ******************************************************************************
 * @Company	: Global Sonic
 * @file	: gs_gpio.c
 * @author	: mario
 * @date	: 4 de set. de 2023
 * @brief	: What this file does
 ******************************************************************************
 */

/******************************************************************************
 *							INCLUDES
 *****************************************************************************/
/* ---------------------- C language standard library ------------------------*/
/* ---------------------- Stack include --------------------------------------*/
#include PLATFORM_HEADER
#include "em_gpio.h"
#include "zigbee_app_framework_event.h"
#include "sl_malloc.h"
#include "sl_common.h"
#include "error-def.h"

/* ---------------------- Others include -------------------------------------*/
//#include "gs_types.h"
#include "gs_hal_gpio.h"


/******************************************************************************
 *							DEFINES
 *****************************************************************************/
/* --------------------- Private macros --------------------------------------*/
#define _GPIOINT_IF_EVEN_MASK ((_GPIO_IF_MASK) & 0x55555555UL)
#define _GPIOINT_IF_ODD_MASK  ((_GPIO_IF_MASK) & 0xAAAAAAAAUL)

/* --------------------- Private typedef -------------------------------------*/
typedef struct st_digital_out_control{
	boolean isInvert;
	int8u currentStatus;
	int8u port;
	int8u pin;
	int8u percent;
	int32u period;
	int32u interactions;
	int32u timeUp;
	int32u timeDown;
	int32u countInterac;
	sl_zigbee_event_t event;
	struct st_digital_out_control *next;
} st_digital_out_control_t;

typedef struct st_in_analog_control{
	boolean inMilli;
	int8u port;
	int8u pin;
	int8u ref;
	struct st_in_analog_control *next;
} st_in_analog_control_t;

typedef struct st_interrupt_control{
	int8u port;
	int8u pin;
	int8u pullUp;
	int8u noInt;
	interruptCallback_t func;
	sl_zigbee_event_t debounceEvent;
	struct st_interrupt_control *next;
} st_interrupt_control_t;

/* --------------------- Private variables -----------------------------------*/
static st_digital_out_control_t *outPutList = NULL;
static st_in_analog_control_t *inAnalog = NULL;
static st_interrupt_control_t *interruptList = NULL;
//Try start the IADC
static boolean initIadc = false;

/* --------------------- Private function prototypes -------------------------*/
static int8u gs_gpio_set_out_put(int8u port, int8u pin, int8u percent, int32u period,
				int32u interactions, boolean invert, boolean forceUpdate);

static int8u gs_gpio_analog_set(int8u port, int8u pin, int8u ref,
		boolean inMilliVolts);
static int32u gs_gpio_analog_read(int8u port, int8u pin);
static int32u gs_gpio_analog_read_vdd(void);

static int8u gs_gpio_get_in_pin_state(int8u port, int8u pin);
static int8u gs_gpio_interrupt_set(int8u port, int8u pin, int8u pullUp, interruptCallback_t callback);
static int8u gs_gpio_set_enable_interrupt(int8u port, int8u pin);
static int8u gs_gpio_set_disable_interrupt(int8u port, int8u pin);
static void gs_gpio_interrutp_debounce_event(sl_zigbee_event_t *event);

/* --------------------- Public objects --------------------------------------*/
static const st_gpio_class_t gpioClass = {
		.setOutPut = &gs_gpio_set_out_put,

		.analogRead = &gs_gpio_analog_read,
		.setAnalog = &gs_gpio_analog_set,
		.getVin = &gs_gpio_analog_read_vdd,

		.getInPinState = &gs_gpio_get_in_pin_state,
		.setInterrup = &gs_gpio_interrupt_set,
		.setEnableInterrup = &gs_gpio_set_enable_interrupt,
		.setDisableInterrup = &gs_gpio_set_disable_interrupt
};
/* --------------------- Bodies of private functions -------------------------*/

/******************************************************************************
 *							FUNCTIONS
 *****************************************************************************/

const st_gpio_class_t *gs_hal_gpio_class(void)
{
	return &gpioClass;
}

static st_digital_out_control_t *getOutPutConfigByGpio(int8u port, int8u pin)
{
	st_digital_out_control_t *current = outPutList;
	while (current != NULL) {
		if (current->port == port && current->pin == pin) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

static st_digital_out_control_t *getOutPutConfigByEvent(sl_zigbee_event_t *event)
{
	st_digital_out_control_t *current = outPutList;
	while (current != NULL) {
		if (event == &current->event) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static void gs_gpio_out_put_event(sl_zigbee_event_t *event)
{
	st_digital_out_control_t *cfg = getOutPutConfigByEvent(event);

	if (event != NULL && cfg != NULL) {
		//is up or down time?
		if (cfg->currentStatus) {
			//time to turn down
			if (cfg->isInvert) {
				GPIO_PinOutSet(cfg->port, cfg->pin);
			} else {
				GPIO_PinOutClear(cfg->port, cfg->pin);
			}
			//if exist interactions
			if (cfg->interactions) {
				//check limit, while limit not research restart event
				if (cfg->countInterac < cfg->interactions) {
					sl_zigbee_event_set_delay_ms(&cfg->event, cfg->timeDown);
				} else {
					cfg->countInterac = 0;
				}
			} else {
				//if we don't have a interaction limitation start event
				sl_zigbee_event_set_delay_ms(&cfg->event, cfg->timeDown);
			}
			cfg->currentStatus = 0;
		} else {
			//time to turn up
			if (cfg->isInvert) {
				GPIO_PinOutClear(cfg->port, cfg->pin);
			} else {
				GPIO_PinOutSet(cfg->port, cfg->pin);
			}
			//Interactions
			if (cfg->interactions) {
				cfg->countInterac++;
			}
			sl_zigbee_event_set_delay_ms(&cfg->event, cfg->timeUp);
			cfg->currentStatus = 1;
		}
	}
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int8u gs_gpio_set_out_put(int8u port, int8u pin, int8u percent, int32u period,
				int32u interactions, boolean invert, boolean forceUpdate)
{
	//Check if the port and pin are valid
	if (!GPIO_PORT_PIN_VALID(port, pin)) {
		return EMBER_APPLICATION_ERROR_0;
	}

	//check the values
	percent = (percent > 100) ? 100 : percent;
	period = (period > EMBER_MAX_EVENT_DELAY_MS) ?
	EMBER_MAX_EVENT_DELAY_MS :
													period;
	int32u timeUp = 0;
	int32u timeDown = 0;
	if (percent < 100) {
		timeUp = (int32u) ((double) period
				* (double) ((double) percent / (double) 100.0));
		timeDown = period - timeUp;
	} else {
		timeUp = period;
		timeDown = 0;
	}

	// Check if exist configuration for this port and pin
	st_digital_out_control_t *obj = getOutPutConfigByGpio(port, pin);
	if (obj == NULL) {
		//configurations not found, create then
		obj = (st_digital_out_control_t*) sl_malloc(
				sizeof(st_digital_out_control_t));
		if (obj == NULL) {
			return EMBER_APPLICATION_ERROR_1;
		}
		//configure the new item
		obj->currentStatus = 0;
		obj->port = port;
		obj->pin = pin;
		obj->percent = percent;
		obj->period = period;
		obj->interactions = interactions;
		obj->isInvert = invert;
		obj->timeUp = timeUp;
		obj->timeDown = timeDown;
		obj->countInterac = 0;
		obj->next = NULL;

		GPIO_PinModeSet(port, pin, gpioModePushPull, invert);

		//configure the event
		sl_zigbee_event_init(&obj->event, gs_gpio_out_put_event);
		//add the new element to the list
		if (outPutList == NULL) {
			outPutList = obj;
		} else {
			st_digital_out_control_t *current = outPutList;
			while (current->next != NULL) {
				current = current->next;
			}
			current->next = obj;
		}
		//if we create the new item, so start this
		sl_zigbee_event_set_active(&obj->event);
	} else {
		//if we already have this item, check if is a update
		if (obj->percent != percent || obj->period != period
				|| obj->isInvert != invert
				|| obj->interactions != interactions) {
			//update
			obj->currentStatus = 0;
			obj->percent = percent;
			obj->period = period;
			obj->interactions = interactions;
			obj->isInvert = invert;
			obj->timeUp = timeUp;
			obj->timeDown = timeDown;
			sl_zigbee_event_set_active(&obj->event);
		}
		//gives a chance to update even if the values have not changed
		//to force the execution
		if(forceUpdate){
			sl_zigbee_event_set_active(&obj->event);
		}
	}

	//if the percent is 100% or 0% we don't need
	//any event for this, just configure the output
	if (percent == 100) {
		sl_zigbee_event_set_inactive(&obj->event);
		if (obj->isInvert) {
			GPIO_PinOutClear(obj->port, obj->pin);
		} else {
			GPIO_PinOutSet(obj->port, obj->pin);
		}
	} else if (!percent) {
		sl_zigbee_event_set_inactive(&obj->event);
		if (obj->isInvert) {
			GPIO_PinOutSet(obj->port, obj->pin);
		} else {
			GPIO_PinOutClear(obj->port, obj->pin);
		}
	}

	return EMBER_SUCCESS;
}

static st_in_analog_control_t *getInAnalogConfigByGpio(int8u port, int8u pin)
{
	st_in_analog_control_t *current = inAnalog;
	while (current != NULL) {
		if (current->port == port && current->pin == pin) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int32u gs_gpio_analog_read_vdd(void)
{
	return 0;
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int32u gs_gpio_analog_read(int8u port, int8u pin)
{
	return 0;
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int8u gs_gpio_analog_set(int8u port, int8u pin, int8u ref,
		boolean inMilliVolts)
{
	//Check if the port and pin are valid
	if (!GPIO_PORT_PIN_VALID(port, pin)) {
		return EMBER_APPLICATION_ERROR_0;
	}

	//check if the port and pin are configured
	st_in_analog_control_t *cfg = getInAnalogConfigByGpio(port, pin);

	if (cfg == NULL) {
		//create a new instance
		cfg = (st_in_analog_control_t*) sl_malloc(
				sizeof(st_in_analog_control_t));
		if (cfg == NULL) {
			return EMBER_APPLICATION_ERROR_1;
		}

		cfg->port = port;
		cfg->pin = pin;
		cfg->ref = ref;
		cfg->inMilli = inMilliVolts;
		cfg->next = NULL;

		//Configure the GPIO
		GPIO_PinModeSet(cfg->port, cfg->pin, gpioModeInput, 0);

		//add the list
		if (inAnalog == NULL) {
			inAnalog = cfg;
		} else {
			st_in_analog_control_t *current = inAnalog;
			while (current->next != NULL) {
				current = current->next;
			}
			current->next = cfg;
		}
		return EMBER_SUCCESS;
	}

	return EMBER_APPLICATION_ERROR_2;
}


/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static void  gs_gpio_interrupt_init(void)
{
	if (CORE_NvicIRQDisabled(GPIO_ODD_IRQn)) {
		NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
		NVIC_EnableIRQ(GPIO_ODD_IRQn);
	}
	if (CORE_NvicIRQDisabled(GPIO_EVEN_IRQn)) {
		NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
		NVIC_EnableIRQ(GPIO_EVEN_IRQn);
	}
}

static st_interrupt_control_t *getInterrutConfigByGpio(int8u port, int8u pin)
{
	st_interrupt_control_t *current = interruptList;
	while (current != NULL) {
		if (current->port == port && current->pin == pin) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

static st_interrupt_control_t *getInterrutConfigByIntNumber(int8u noInt)
{
	st_interrupt_control_t *current = interruptList;
	while (current != NULL) {
		if (current->noInt == noInt) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int8u getInterruptNumber(int8u pin)
{
	CORE_DECLARE_IRQ_STATE;
	unsigned int intNo = 0xFF;

	CORE_ENTER_ATOMIC();

	uint32_t intToCheck;
	uint32_t intGroupStart = (pin & 0xFFC);
	uint32_t intsEnabled = GPIO_EnabledIntGet();

	// loop through the interrupt group, starting
	// from the pin number, and take
	// the first available
	for (uint8_t i = 0; i < 4; i++) {
		intToCheck = intGroupStart + ((pin + i) & 0x3); // modulo 4
		if (((intsEnabled >> intToCheck) & 0x1) == 0) {
			intNo = (unsigned int) intToCheck;
			break;
		}
	}

	CORE_EXIT_ATOMIC();

	return (int8u) intNo;
}


/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int8u gs_gpio_interrupt_set(int8u port, int8u pin, int8u pullUp, interruptCallback_t callback)
{

	//Check if the port and pin are valid
	if (!GPIO_PORT_PIN_VALID(port, pin)) {
		return EMBER_APPLICATION_ERROR_0;
	}

	if (callback == NULL) {
		return EMBER_APPLICATION_ERROR_1;
	}

	//check configuration
	st_interrupt_control_t *interrup = getInterrutConfigByGpio(port, pin);

	if (interrup == NULL) { //if don't exist configuration for this port and pin
		gs_gpio_interrupt_init();

		interrup = (st_interrupt_control_t*) sl_malloc(
				sizeof(st_interrupt_control_t));

		int8u numberInt = getInterruptNumber(pin);
		if (numberInt == 0xFF) {
			return EMBER_APPLICATION_ERROR_2;
		}
		interrup->port = port;
		interrup->pin = pin;
		interrup->pullUp = pullUp;
		interrup->func = callback;
		interrup->noInt = numberInt;
		interrup->next = NULL;

		sl_zigbee_event_init(&interrup->debounceEvent,
				gs_gpio_interrutp_debounce_event);

		if (interruptList == NULL) {
			interruptList = interrup;
		} else {
			st_interrupt_control_t *current = interruptList;
			while (current->next != NULL) {
				current = current->next;
			}
			current->next = interrup;
		}

		//configure the GPIO - Input
		GPIO_PinModeSet(interrup->port, interrup->pin, gpioModeInputPullFilter,
				interrup->pullUp);
		//Configure the GPIO interrupt - both edge
		GPIO_ExtIntConfig(port, pin, numberInt, true, true, true);

		return EMBER_SUCCESS;
	} else {
		return EMBER_APPLICATION_ERROR_3;
	}
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int8u gs_gpio_set_disable_interrupt(int8u port, int8u pin)
{
	st_interrupt_control_t *interrup = getInterrutConfigByGpio(port, pin);
	if (interrup == NULL) {
		return EMBER_APPLICATION_ERROR_0;
	}
	GPIO_PinModeSet(interrup->port, interrup->pin, gpioModeDisabled,
			interrup->pullUp);
	return EMBER_SUCCESS;
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int8u gs_gpio_set_enable_interrupt(int8u port, int8u pin)
{
	st_interrupt_control_t *interrup = getInterrutConfigByGpio(port, pin);
	if (interrup == NULL) {
		return EMBER_APPLICATION_ERROR_0;
	}
	GPIO_PinModeSet(interrup->port, interrup->pin, gpioModeInputPullFilter,
			interrup->pullUp);
	return EMBER_SUCCESS;
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int8u gs_gpio_get_in_pin_state(int8u port, int8u pin)
{
	return GPIO_PinInGet(port, pin);
}

static st_interrupt_control_t *getInterrutConfigByEvent(sl_zigbee_event_t *event)
{
	st_interrupt_control_t *current = interruptList;
	while (current != NULL) {
		if (&current->debounceEvent == event) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

static void gs_gpio_interrutp_debounce_event(sl_zigbee_event_t *event)
{
	st_interrupt_control_t *current = getInterrutConfigByEvent(event);
	if(current != NULL){
	  /* call user callback */
	  if (current->func != NULL) {
		  current->func(current->port,current->pin);
	  }
	}
}

/***************************************************************************//**
 * @brief
 *   Function calls users callback for registered pin interrupts.
 *
 * @details
 *   This function is called when GPIO interrupts are handled by the IRQHandlers.
 *   Function gets even or odd interrupt flags and calls user callback
 *   registered for that pin. Function iterates on flags starting from MSB.
 *
 * @param iflags
 *  Interrupt flags which shall be handled by the dispatcher.
 *
 ******************************************************************************/
static void GPIOINT_IRQDispatcher(uint32_t iflags)
{
  uint32_t irqIdx;
  st_interrupt_control_t *current;

  /* check for all flags set in IF register */
  while (iflags != 0U) {
    irqIdx = SL_CTZ(iflags);

    /* clear flag*/
    iflags &= ~(1UL << irqIdx);

    current = getInterrutConfigByIntNumber(irqIdx);
    if (current != NULL) {
//      /* call user callback */
//      if (current->func != NULL) {
//    	  current->func(current->port,current->pin);
//      }
    	sl_zigbee_event_set_delay_ms(&current->debounceEvent,HAL_GPIO_DEBOUNCE);
    }
  }
}

/***************************************************************************//**
 * @brief
 *   GPIO EVEN interrupt handler. Interrupt handler clears all IF even flags and
 *   call the dispatcher passing the flags which triggered the interrupt.
 *
 ******************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  uint32_t iflags;

  /* Get all even interrupts. */
  iflags = GPIO_IntGetEnabled() & _GPIOINT_IF_EVEN_MASK;

  /* Clean only even interrupts. */
  GPIO_IntClear(iflags);

  GPIOINT_IRQDispatcher(iflags);
}

/***************************************************************************//**
 * @brief
 *   GPIO ODD interrupt handler. Interrupt handler clears all IF odd flags and
 *   call the dispatcher passing the flags which triggered the interrupt.
 *
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  uint32_t iflags;

  /* Get all odd interrupts. */
  iflags = GPIO_IntGetEnabled() & _GPIOINT_IF_ODD_MASK;

  /* Clean only odd interrupts. */
  GPIO_IntClear(iflags);

  GPIOINT_IRQDispatcher(iflags);
}

