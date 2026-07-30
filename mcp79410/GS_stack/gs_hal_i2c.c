/**
 ******************************************************************************
 * @Company	: Global Sonic
 * @file	: gs_i2c.c
 * @author	: mario
 * @date	: 4 de set. de 2023
 * @brief	: What this file does
 ******************************************************************************
 */

/******************************************************************************
 *							INCLUDES
 *****************************************************************************/
/* ---------------------- C language standard library ------------------------*/
#include <stddef.h>
/* ---------------------- Stack include --------------------------------------*/
#include "em_i2c.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "sl_assert.h"
#include "sl_udelay.h"
#include "ember-types.h"
#include "sl_malloc.h"

/* ---------------------- Others include -------------------------------------*/
#include "gs_hal_gpio.h"
#include "gs_hal_i2c.h"


/******************************************************************************
 *							DEFINES
 *****************************************************************************/
/* --------------------- Private macros --------------------------------------*/
/* SCL hold time (in initialization function) in microseconds */
#ifndef SL_I2CSPM_SCL_HOLD_TIME_US
#define SL_I2CSPM_SCL_HOLD_TIME_US 100
#endif
/* transfer timeout (how many polls) */
#ifndef I2CSPM_TRANSFER_TIMEOUT
#define I2CSPM_TRANSFER_TIMEOUT 300000
#endif
/* --------------------- Private typedef -------------------------------------*/
typedef struct st_i2c_int_monitor{
	i2cInterruptMonitorCallback callback;
	struct st_i2c_int_monitor *next;
} st_i2c_int_monitor_t;
/* --------------------- Private variables -----------------------------------*/
static boolean isInitMonitor = false;

/* --------------------- Private function prototypes -------------------------*/
static int8u gs_i2c_init(I2C_TypeDef *i2c,
		int8u sclPort, int8u sclPin,
		int8u sdaPort, int8u sdaPin);
static int8u gs_i2c_transfer(int16u flags, int16u slaveAddress,
		int8u *pData0, int16u *pSizeData0, int8u *pData1, int16u *pSizeData1);


static int8u gs_i2c_register_callcb(i2cInterruptMonitorCallback cb);
static int8u gs_i2c_interrupt_monitor_init(int8u port, int8u pin);

/* --------------------- Public objects --------------------------------------*/
static const st_i2c_class_t i2cClass = {
		.i2cInit = &gs_i2c_init,
		.i2cSimpleTransfer = &gs_i2c_transfer,

		.interruptGpioMonitorInit = &gs_i2c_interrupt_monitor_init,
		.registerCallBack = &gs_i2c_register_callcb,
};

static st_i2c_int_monitor_t *callbackList = NULL;

/* --------------------- Bodies of private functions -------------------------*/

/******************************************************************************
 *							FUNCTIONS
 *****************************************************************************/

const st_i2c_class_t *gs_hal_i2c_class(void)
{
	return &i2cClass;
}


/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int8u gs_i2c_init(I2C_TypeDef *i2c,
		int8u sclPort, int8u sclPin,
		int8u sdaPort, int8u sdaPin)
{
	int i;
	CMU_Clock_TypeDef i2cClock;
	I2C_Init_TypeDef i2cInit;

	if (i2c == NULL) {
		return EMBER_APPLICATION_ERROR_0;
	}

	/* Select I2C peripheral clock */
	if (false) {
#if defined(I2C0)
	} else if (i2c == I2C0) {
		i2cClock = cmuClock_I2C0;
#endif
#if defined(I2C1)
	} else if (i2c == I2C1) {
		i2cClock = cmuClock_I2C1;
#endif
#if defined(I2C2)
	  } else if (i2c == I2C2) {
	    i2cClock = cmuClock_I2C2;
	#endif
	} else {
		/* I2C clock is not defined */
		EFM_ASSERT(false);
		return EMBER_APPLICATION_ERROR_1;
	}
	CMU_ClockEnable(i2cClock, true);

	/* Output value must be set to 1 to not drive lines low. Set
	 SCL first, to ensure it is high before changing SDA. */
	GPIO_PinModeSet(sclPort, sclPin, gpioModeWiredAndPullUp, 1);
	GPIO_PinModeSet(sdaPort, sdaPin, gpioModeWiredAndPullUp, 1);

	/* In some situations, after a reset during an I2C transfer, the slave
	 device may be left in an unknown state. Send 9 clock pulses to
	 set slave in a defined state. */
	for (i = 0; i < 9; i++) {
		GPIO_PinOutClear(sclPort, sclPin);
		sl_udelay_wait(SL_I2CSPM_SCL_HOLD_TIME_US);
		GPIO_PinOutSet(sclPort, sclPin);
		sl_udelay_wait(SL_I2CSPM_SCL_HOLD_TIME_US);
	}

	/* Enable pins and set location */
#if defined(I2C0)
	if (i2c == I2C0) {
		GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN
				| GPIO_I2C_ROUTEEN_SCLPEN;
		GPIO->I2CROUTE[0].SCLROUTE = (uint32_t) ((sclPin
				<< _GPIO_I2C_SCLROUTE_PIN_SHIFT)
				| (sclPort << _GPIO_I2C_SCLROUTE_PORT_SHIFT));
		GPIO->I2CROUTE[0].SDAROUTE = (uint32_t) ((sdaPin
				<< _GPIO_I2C_SDAROUTE_PIN_SHIFT)
				| (sdaPort << _GPIO_I2C_SDAROUTE_PORT_SHIFT));
	}
#endif
#if defined(I2C1)
	if (i2c == I2C1) {
		GPIO->I2CROUTE[1].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN
				| GPIO_I2C_ROUTEEN_SCLPEN;
		GPIO->I2CROUTE[1].SCLROUTE = (uint32_t) ((sclPin
				<< _GPIO_I2C_SCLROUTE_PIN_SHIFT)
				| (sclPort << _GPIO_I2C_SCLROUTE_PORT_SHIFT));
		GPIO->I2CROUTE[1].SDAROUTE = (uint32_t) ((sdaPin
				<< _GPIO_I2C_SDAROUTE_PIN_SHIFT)
				| (sdaPort << _GPIO_I2C_SDAROUTE_PORT_SHIFT));
	}
#endif
#if defined(I2C2)
	  if (i2c == I2C2) {
	    GPIO->I2CROUTE[2].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
	    GPIO->I2CROUTE[2].SCLROUTE = (uint32_t)((sclPin << _GPIO_I2C_SCLROUTE_PIN_SHIFT)
	                                            | (sclPort << _GPIO_I2C_SCLROUTE_PORT_SHIFT));
	    GPIO->I2CROUTE[2].SDAROUTE = (uint32_t)((sdaPin << _GPIO_I2C_SDAROUTE_PIN_SHIFT)
	                                            | (sdaPort << _GPIO_I2C_SDAROUTE_PORT_SHIFT));
	  }
#endif

	/* Set emlib init parameters */
	i2cInit.enable = true;
	i2cInit.master = true; /* master mode only */
	i2cInit.freq = I2C_FREQ_STANDARD_MAX;
	i2cInit.refFreq = 0;
	i2cInit.clhr = i2cClockHLRStandard;

	I2C_Init(i2c, &i2cInit);
	return EMBER_SUCCESS;
}

I2C_TransferReturn_TypeDef I2C_transfer(I2C_TypeDef *i2c, I2C_TransferSeq_TypeDef *seq)
{
  I2C_TransferReturn_TypeDef ret;
  uint32_t timeout = I2CSPM_TRANSFER_TIMEOUT;
  /* Do a polled transfer */
  ret = I2C_TransferInit(i2c, seq);
  while (ret == i2cTransferInProgress && timeout--) {
    ret = I2C_Transfer(i2c);
  }
  return ret;
}

/**
 * @brief		: Transfer data via I2C.
 *
 * @pre-cond.	: The software component I2CSPM must be enable with a instance
 *  "I2C" created and properly configured.
 *
 * @param[in]	: flags
 * 		The flags provide a way to use this function as a Read, Write or both.
 * 		See the flags definitions above.
 *
 * @param[in]	: slaveAddress
 * 		The Slave Address (7 or 10 bits. For 10 bits use the flag to indicate).
 *
 * @param[out]	: pData0
 * 		The data0 point.
 *
 * @param[out]	: pSizeData0
 * 		Point for data0 size.
 *
 * @param[out]	: pData1
 * 		The data1 point.
 *
 * @param[out]	: pSizeData1
 * 		Point for data1 size.
 *
 * 	@details
 * 		This method has been implemented to work as read, write, or both.
 * 		Therefore, depending on the operation, data points 0 and 1 have
 * 		different functions(according to the flag), as can be seen below:
 * 		- GS_DRV_I2C_FLAG_READ : 		will be copied to Data0 pointers, and
 * 										Data1 pointers can be null.
 * 		- GS_DRV_I2C_FLAG_WRITE : 		The data to be written must be in Data0
 * 										and data size must contain the size of
 * 										the data to be written.
 * 		- GS_DRV_I2C_FLAG_WRITE_READ : 	Data0 must contain the data to be written
 * 										and the data read will be written to Data1.
 * 		- GS_DRV_I2C_FLAG_WRITE_WRITE : Both Data0 and Data1 must contain the
 * 										information to be written.
 *
 *
 * @return		: Return 0 if all good, error code otherwise.
 */
static int8u gs_i2c_transfer(int16u flags, int16u slaveAddress,
		int8u *pData0, int16u *pSizeData0, int8u *pData1, int16u *pSizeData1)
{
	I2C_TransferReturn_TypeDef ret = i2cTransferUsageFault;
	I2C_TransferSeq_TypeDef transfer = { 0, 0, { { NULL, 0 }, { NULL, 0 } } };

	transfer.addr = slaveAddress;
	transfer.flags = flags;
	switch (transfer.flags) {
	case I2C_FLAG_READ:
	case I2C_FLAG_WRITE:
	case I2C_FLAG_WRITE_READ:
	case I2C_FLAG_WRITE_WRITE:
		if (pData0 != NULL) {
			transfer.buf[0].data = pData0;
			transfer.buf[0].len = *pSizeData0;
		}
		if (pData1 != NULL) {
			transfer.buf[1].data = pData1;
			transfer.buf[1].len = *pSizeData1;
		}
		//For any option the data0 is't a null pointer
		if ((pData0 != NULL)) {
			ret = I2C_transfer(GS_PRIMARY_I2C, &transfer);
			*pSizeData0 = transfer.buf[0].len;
			*pSizeData1 = transfer.buf[1].len;
		}
		break;
	default:
		return ret;
		break;
	}

	return (int8u) ret;
}

void gs_i2c_gpio_interrupt_cb(int8u port, int8u pin)
{
	if (callbackList != NULL) {
		st_i2c_int_monitor_t *current = callbackList;
		while (current != NULL) {
			current->callback();
			current = current->next;
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
static int8u gs_i2c_interrupt_monitor_init(int8u port, int8u pin)
{

	int8u status = EMBER_APPLICATION_ERROR_0;
	if(!isInitMonitor){
		status = gs_hal_gpio_class()->setInterrup(port, pin, 1U,
				gs_i2c_gpio_interrupt_cb);
		if(!status){
			isInitMonitor = true;
		}
	}
	return status;
}

/**
 * @brief		: Put your description here
 * @pre-cond.	: Conditions needed to APIs/drivers work, if it exists.
 * @post-cond.	: Conditions achieved after API/drivers executed, if it exists.
 * @param[in]	: <name> <description> Fill in this field where there is or is not an input parameter.
 * @param[out]	: <name> <description> Fill this field where there is or is not an output parameter.
 * @return		: <Return value description>.Fill this field wherever there is or not returning
 */
static int8u gs_i2c_register_callcb(i2cInterruptMonitorCallback cb)
{
	//if the monitoring for interrupt is not enable
	if(!isInitMonitor || cb == NULL){
		return EMBER_APPLICATION_ERROR_0;
	}

	st_i2c_int_monitor_t *new = (st_i2c_int_monitor_t*) sl_malloc(
			sizeof(st_i2c_int_monitor_t));

	if(new == NULL){
		return EMBER_APPLICATION_ERROR_0;
	}

	new->callback = cb;
	new->next = NULL;

	if (callbackList == NULL) {
			callbackList = new;
			return EMBER_SUCCESS;
	} else {
		st_i2c_int_monitor_t *current = callbackList;
		while(current->next != NULL){
			current = current->next;
		}
		current->next = new;
		current->callback(); //call the first time
		return EMBER_SUCCESS;
	}
}
