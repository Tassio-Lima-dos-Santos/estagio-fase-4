/**
 ******************************************************************************
 * @Company	: Global Sonic
 * @file	: gs_i2c.h
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

#ifndef GS_HAL_I2C_H_
#define GS_HAL_I2C_H_

/* -------------------------- Includes ---------------------------------------*/
#include PLATFORM_HEADER
/* -------------------------- Define -----------------------------------------*/
#define GS_PRIMARY_I2C				I2C0

/**
 * @brief
 *   Indicate plain write sequence: S+ADDR(W)+DATA0+P.
 * @details
 *   @li S - Start
 *   @li ADDR(W) - address with W/R bit cleared
 *   @li DATA0 - Data taken from buffer with index 0
 *   @li P - Stop
 */
#define GS_DRV_I2C_FLAG_WRITE          0x0001

/**
 * @brief
 *   Indicate plain read sequence: S+ADDR(R)+DATA0+P.
 * @details
 *   @li S - Start
 *   @li ADDR(R) - Address with W/R bit set
 *   @li DATA0 - Data read into buffer with index 0
 *   @li P - Stop
 */
#define GS_DRV_I2C_FLAG_READ           0x0002

/**
 * @brief
 *   Indicate combined write/read sequence: S+ADDR(W)+DATA0+Sr+ADDR(R)+DATA1+P.
 * @details
 *   @li S - Start
 *   @li Sr - Repeated start
 *   @li ADDR(W) - Address with W/R bit cleared
 *   @li ADDR(R) - Address with W/R bit set
 *   @li DATAn - Data written from/read into buffer with index n
 *   @li P - Stop
 */
#define GS_DRV_I2C_FLAG_WRITE_READ     0x0004

/**
 * @brief
 *   Indicate write sequence using two buffers: S+ADDR(W)+DATA0+DATA1+P.
 * @details
 *   @li S - Start
 *   @li ADDR(W) - Address with W/R bit cleared
 *   @li DATAn - Data written from buffer with index n
 *   @li P - Stop
 */
#define GS_DRV_I2C_FLAG_WRITE_WRITE    0x0008

/** Use 10 bit address. */
#define GS_DRV_I2C_FLAG_10BIT_ADDR     0x0010
/* -------------------------- Typedef ----------------------------------------*/

typedef void (*i2cInterruptMonitorCallback)(void);

typedef struct {

	//Initialize the I2C
	int8u (*i2cInit)(I2C_TypeDef *i2c,
			int8u sclPort, int8u sclPin,
			int8u sdaPort, int8u sdaPin);

	/**
	 * @brief		: Transfer data via I2C - The GS_PRIMARY_I2C.
	 * @pre-cond.	: The software component I2CSPM must be enable with a instance
	 *  "I2C" created and properly configured.
	 * @param[in]	: flags
	 * 		The flags provide a way to use this function as a Read, Write or both.
	 * 		See the flags definitions above.
	 * @param[in]	: slaveAddress
	 * 		The Slave Address (7 or 10 bits. For 10 bits use the flag to indicate).
	 * @param[out]	: pData0
	 * 		The data0 point.
	 * @param[out]	: pSizeData0
	 * 		Point for data0 size.
	 * @param[out]	: pData1
	 * 		The data1 point.
	 * @param[out]	: pSizeData1
	 * 		Point for data1 size.
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
	 * @return		: Return 0 if all good, error code otherwise.
	 */
	int8u (*i2cSimpleTransfer)(int16u flags, int16u slaveAddress,
			int8u *pData0, int16u *pSizeData0, int8u *pData1, int16u *pSizeData1);


	//Initialize the I2C Interrupt GPIO concentration
	int8u (*interruptGpioMonitorInit)(int8u port, int8u pin);
	//Register callback for interrupts
	int8u (*registerCallBack)(i2cInterruptMonitorCallback cb);
} st_i2c_class_t;

/* -------------------------- Public objects ---------------------------------*/

const st_i2c_class_t *gs_hal_i2c_class(void);

#endif /* GS_HAL_I2C_H_ */
