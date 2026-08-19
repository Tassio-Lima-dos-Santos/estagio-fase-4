/******************************************************************************
 * File HDC_1080.c
 *
 *  Created on: 15 de jul. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include "HDC_1080.h"
#include "../GS_stack/gs_hal_i2c.h"
#include "em_i2c.h"
#include "HDC_1080_sensor_config.h"
#include "sl_sleeptimer.h"
#include "zigbee_app_framework_event.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

//#define DEBUG

// HDC_1080_Config_Settings HDC 1080 Configuration Settings
#define HDC_1080_I2C_BUS_ADDRESS           (0x40 << 1)           /**< I2C bus address*/

#define TEMPERATURE_REGISTER_ADDRESS        0x00
#define HUMIDITY_REGISTER_ADDRESS           0x01
#define CONFIGURATION_REGISTER_ADDRESS      0x02

#define TEMPERATURE_REGISTER_RESET_VALUE    0x0000
#define HUMIDITY_REGISTER_RESET_VALUE       0x0000
#define CONFIGURATION_REGISTER_RESET_VALUE  0x1000

#define HDC_1080_BUSY_INDICATION_BITMASK    0b10000000
#define HDC_1080_MODE_STATUS_1_BITMASK      0b01000000
#define HDC_1080_MODE_STATUS_0_BITMASK      0b00100000
#define HDC_1080_CAL_ENABLE_BITMASK         0b00001000

#define HDC_1080_CONFIG_FLAG_RST            0b1000000000000000
#define HDC_1080_CONFIG_FLAG_HEAT           0b0010000000000000
#define HDC_1080_CONFIG_FLAG_MODE_SINGLE    0b0000000000000000
#define HDC_1080_CONFIG_FLAG_MODE_DOUBLE    0b0001000000000000
#define HDC_1080_CONFIG_FLAG_TRES_11b       0b0000010000000000
#define HDC_1080_CONFIG_FLAG_HRES_11b       0b0000000100000000
#define HDC_1080_CONFIG_FLAG_HRES_8b        0b0000001000000000
#define HDC_1080_DEFAULT_CONFIG             0b0001000000000000

#define READ_TEMP_BUFFER_SIZE            32
#define DEFAULT_READ_TIMEOUT                50

#define DEFAULT_WRITE_CMD_SIZE              3
#define DEFAULT_WRITE_TIMEOUT               10

#define TEMPERATURE_POLLING_PERIOD_MS       1000
#define SENSOR_CONVERSION_TIME_MS           7

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/

static uint8_t global_read_buffer[READ_TEMP_BUFFER_SIZE];
static uint16_t global_read_buffer_size = READ_TEMP_BUFFER_SIZE;

static float temperature = 25;
static float relative_humidity = 50;

static sl_zigbee_event_t temperature_polling_event;

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static void configure_sensor(uint16_t configure_data);

static const uint8_t* read_from_register(const uint8_t register_address);
static void write_to_register(const uint8_t register_address, const uint16_t data);
static void raw_humidity_from_byte_stream(const uint8_t* byte_stream, uint16_t* raw_humidity);
static void decode_rh(const uint16_t raw_humidity, float* humidity);
static void raw_temperature_from_byte_stream(const uint8_t* byte_stream, uint16_t* raw_temperature);
static void decode_temp(const uint16_t raw_temperature, float* temperature);

static void temperature_polling_event_handler(sl_zigbee_event_t *event);


/******************************************************************************
 * Function Definitions
 *****************************************************************************/
/*******************************************************************************
 * Function name: read_from_sensor
 *
 * Description  : Function used to read information from HDC 1080 sensor
 * Parameters   : void
 * Returns      : returns global_read_buffer that is the array where read information is stored
 *
 * Known issues :
 * Note         : Size information is stored in global_read_buffer_size
 ******************************************************************************/
static const uint8_t* read_from_register(const uint8_t register_address)
{
  uint16_t write_size = 1;
  uint8_t  write_buffer[] = {register_address};
  int write_timeout = DEFAULT_READ_TIMEOUT;
  while(write_timeout--){
    I2C_TransferReturn_TypeDef ret = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_WRITE,
                                                                           HDC_1080_I2C_BUS_ADDRESS,
                                                                           write_buffer, &write_size,
                                                                           NULL, NULL);
    if (ret == i2cTransferDone) {
#ifdef DEBUG
      printf("Transfer Done!\r\n");
#endif // DEBUG
      break;
    } else {
#ifdef DEBUG
      printf("Write Error!\r\n");
#endif // DEBUG
    }
  }

  int read_timeout = DEFAULT_READ_TIMEOUT;
  while(read_timeout--){
    I2C_TransferReturn_TypeDef ret = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_READ,
                                                                           HDC_1080_I2C_BUS_ADDRESS,
                                                                           global_read_buffer, &global_read_buffer_size,
                                                                           NULL, NULL);
    if (ret == i2cTransferDone) {
#ifdef DEBUG
      printf("Transfer Done!\r\n");
#endif // DEBUG
      break;
    } else {
#ifdef DEBUG
      printf("Read Error!\r\n");
#endif // DEBUG
      sl_sleeptimer_delay_millisecond(1);
    }
  }

  return (global_read_buffer);
}

/*******************************************************************************
 * Function name: write_to_sensor
 *
 * Description  : Function used to read information from HDC 1080 sensor
 * Parameters   : void
 * Returns      : returns global_read_buffer that is the array where read information is stored
 *
 * Known issues :
 * Note         : Size information is stored in global_read_buffer_size
 ******************************************************************************/
static void write_to_register(const uint8_t register_address, const uint16_t data)
{
  uint16_t write_size = DEFAULT_WRITE_CMD_SIZE;
  uint8_t write_buffer[DEFAULT_WRITE_CMD_SIZE] = {register_address, (const uint8_t) (data >> 8), (const uint8_t) data};
  uint16_t write_timeout = DEFAULT_WRITE_TIMEOUT;
  while(write_timeout--){
    I2C_TransferReturn_TypeDef ret = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_WRITE,
                                                                           HDC_1080_I2C_BUS_ADDRESS,
                                                                           write_buffer, &write_size,
                                                                           NULL, NULL);
    if (ret == i2cTransferDone) {
#ifdef DEBUG
      printf("Transfer Done!\r\n");
#endif // DEBUG
      break;
    }
  }
}

/*******************************************************************************
 * Function name: raw_humidity_from_byte_stream
 *
 * Description  : Function used to convert the array of bytes read from HDC 1080 to raw relative humidity
 * Parameters   : INPUT byte_stream - array of bytes read from HDC 1080
 *                OUTPUT raw_humidity - relative humidity information encoded in a uint16 format
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static void raw_humidity_from_byte_stream(const uint8_t* byte_stream, uint16_t* raw_humidity)
{
  *raw_humidity = ((uint16_t)byte_stream[2] << 8) | ((uint16_t)byte_stream[3]);
}

/*******************************************************************************
 * Function name: decode_rh
 *
 * Description  : Function used to decode the raw relative humidity info and outputs the relative humidity decoded
 * Parameters   : INPUT raw_humidity - relative humidity information encoded in a uint16 format
 *                OUTPUT humidity - relative humidity information decoded
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static void decode_rh(const uint16_t raw_humidity, float* humidity)
{
  *humidity = ((float) raw_humidity / 65536.0f) * 100.0f; // 2^16 = 65536
}

/*******************************************************************************
 * Function name: raw_temperature_from_byte_stream
 *
 * Description  : Function used to convert the array of bytes read from HDC 1080 to raw temperature
 * Parameters   : INPUT read_register - array of bytes read from HDC 1080
 *                OUTPUT raw_temperature - temperature information encoded in a uint16 format
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static void raw_temperature_from_byte_stream(const uint8_t* byte_stream, uint16_t* raw_temperature)
{
  *raw_temperature = ((uint16_t)byte_stream[0] << 8) | (uint16_t)byte_stream[1];
}

/*******************************************************************************
 * Function name: decode_temp
 *
 * Description  : Function used to decode the raw temperature info and outputs the temperature decoded
 * Parameters   : INPUT raw_temperature - temperature information encoded in a uint16 format
 *                OUTPUT temperature - temperature information decoded
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static void decode_temp(const uint16_t raw_temperature, float* temperature)
{
  *temperature = (((float)raw_temperature / 65536.0f) * 165.0f) - 40.0f;
}

/*******************************************************************************
 * Function name: HDC_1080_init
 *
 * Description  : Function used to initialize the HDC 1080 sensor
 * Parameters   : void
 * Returns      : void
 *
 * Known issues :
 * Note         : Initializes a periodic event that prints temperature and relative humidity as well
 ******************************************************************************/
void HDC_1080_init(void)
{
  GPIO_PinModeSet(HDC_1080_SENSOR_ENABLE_PORT, HDC_1080_SENSOR_ENABLE_PIN, gpioModePushPull, 1);

  gs_hal_i2c_class()->i2cInit(GS_PRIMARY_I2C, HDC_1080_SENSOR_SCL_PORT, HDC_1080_SENSOR_SCL_PIN, HDC_1080_SENSOR_SDA_PORT, HDC_1080_SENSOR_SDA_PIN);

  configure_sensor(HDC_1080_DEFAULT_CONFIG);

  sl_zigbee_event_init(&temperature_polling_event, temperature_polling_event_handler);
  sl_zigbee_event_set_delay_ms(&temperature_polling_event, TEMPERATURE_POLLING_PERIOD_MS);
}

/*******************************************************************************
 * Function name: configure_sensor
 *
 * Description  : Function used to calibrate the sensor
 * Parameters   : INPUT configure_data - A 2 bytes data that's written into the HDC 1080's config register
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static void configure_sensor(uint16_t configure_data)
{
  write_to_register(CONFIGURATION_REGISTER_ADDRESS, configure_data);
}

/*******************************************************************************
 * Function name: measure_temperature_and_RH
 *
 * Description  : Function used to measure temperature and relative humidity through HDC 1080 sensor
 * Parameters   : void
 * Returns      : void
 *
 * Known issues :
 * Note         : Stores temperature and relative humidity in the static variables temperature and relative_humidity
 ******************************************************************************/
void measure_temperature_and_RH(void){
  read_from_register(TEMPERATURE_REGISTER_ADDRESS);

  uint16_t raw_temp;
  raw_temperature_from_byte_stream(global_read_buffer, &raw_temp);
  decode_temp(raw_temp, &temperature);

  uint16_t raw_rh;
  raw_humidity_from_byte_stream(global_read_buffer, &raw_rh);
  decode_rh(raw_rh, &relative_humidity);
}

/*******************************************************************************
 * Function name: get_temperature
 *
 * Description  : Returns the temperature information obtained in the last measure
 * Parameters   : void
 * Returns      : float - temperature
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
float get_temperature(void){
  return (temperature);
}

/*******************************************************************************
 * Function name: measure_temperature_and_RH
 *
 * Description  : Returns the relative humidity information obtained in the last measure
 * Parameters   : void
 * Returns      : float - relative humidity
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
float get_relative_humidity(void){
  return (relative_humidity);
}

static void temperature_polling_event_handler(sl_zigbee_event_t *event){
  measure_temperature_and_RH();

  printf("\r\n");
  printf("Relative Humidity = %.2f%%\r\n", get_relative_humidity());
  printf("Temperature = %.2f C\r\n", get_temperature());

  sl_zigbee_event_set_delay_ms(event, TEMPERATURE_POLLING_PERIOD_MS);
}




























