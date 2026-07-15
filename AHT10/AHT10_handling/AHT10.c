/******************************************************************************
 * File AHT10.c
 *
 *  Created on: 14 de jul. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include "AHT10.h"
#include "../GS_stack/gs_hal_i2c.h"
#include "em_i2c.h"
#include "AHT10_sensor_config.h"
#include "sl_sleeptimer.h"
#include "zigbee_app_framework_event.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

//#define DEBUG

// AHT10_Config_Settings AHT10 Configuration Settings
#define AHT10_I2C_BUS_ADDRESS           (0x38 << 1)           /**< I2C bus address*/

// Si7021 command macro definitions
#define AHT10_CMD_CALIBRATE   0xE1
#define AHT10_CMD_TRIGGER     0xAC
#define AHT10_CMD_SOFTRESET   0xBA

#define AHT10_BUSY_INDICATION_BITMASK   0b10000000
#define AHT10_MODE_STATUS_1_BITMASK     0b01000000
#define AHT10_MODE_STATUS_0_BITMASK     0b00100000
#define AHT10_CAL_ENABLE_BITMASK        0b00001000

#define DEFAULT_READ_BUFFER_SIZE        32
#define DEFAULT_READ_TIMEOUT            10

#define DEFAULT_WRITE_CMD_SIZE           3
#define DEFAULT_WRITE_TIMEOUT            10

#define TEMPERATURE_POLLING_PERIOD_MS    1000

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/

static const uint8_t calibrate_cmd[DEFAULT_WRITE_CMD_SIZE]    = {AHT10_CMD_CALIBRATE, 0x08, 0x00};
static const uint8_t trigger_measure_cmd[DEFAULT_WRITE_CMD_SIZE]      = {AHT10_CMD_TRIGGER, 0x33, 0x00};
static const uint8_t soft_reset_cmd[DEFAULT_WRITE_CMD_SIZE]   = {AHT10_CMD_SOFTRESET, 0x00, 0x00};

static uint8_t global_read_buffer[DEFAULT_READ_BUFFER_SIZE];
static uint16_t global_read_buffer_size = DEFAULT_READ_BUFFER_SIZE;

static float temperature = 25;
static float relative_humidity = 50;

static sl_zigbee_event_t temperature_polling_event;

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static void calibrate_sensor(void);

static bool is_sensor_busy(const uint8_t* read_register);
static bool is_sensor_calibrated(const uint8_t* read_register);
static const uint8_t* read_from_sensor(void);
static void decode_rh(const uint8_t* read_register, float* humidity);
static void decode_temp(const uint8_t* read_register, float* temperature);
static void decode_temp(const uint8_t* read_register, float* temperature);

static void temperature_polling_event_handler(sl_zigbee_event_t *event);


/******************************************************************************
 * Function Definitions
 *****************************************************************************/

/*******************************************************************************
 * Function name: is_sensor_busy
 *
 * Description  : Function used to decode the array of bytes read from AHT10 and returns if the sensor is busy
 * Parameters   : INPUT read_register - array of bytes read from AHT10
 * Returns      : bool - true if sensor is busy, false if sensor is free
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static bool is_sensor_busy(const uint8_t* read_register)
{
  return (read_register[0] & AHT10_BUSY_INDICATION_BITMASK);
}

/*******************************************************************************
 * Function name: is_sensor_calibrated
 *
 * Description  : Function used to decode the array of bytes read from AHT10 and returns if the sensor is busy
 * Parameters   : INPUT read_register - array of bytes read from AHT10
 * Returns      : bool - true if sensor is calibrated, false if sensor is not calibrated
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static bool is_sensor_calibrated(const uint8_t* read_register)
{
  return (read_register[0] & AHT10_CAL_ENABLE_BITMASK);
}

/*******************************************************************************
 * Function name: read_from_sensor
 *
 * Description  : Function used to read information from AHT10 sensor
 * Parameters   : void
 * Returns      : returns global_read_buffer that is the array where read information is stored
 *
 * Known issues :
 * Note         : Size information is stored in global_read_buffer_size
 ******************************************************************************/
static const uint8_t* read_from_sensor(void)
{
  int read_timeout = DEFAULT_READ_TIMEOUT;
  while(read_timeout--){
    I2C_TransferReturn_TypeDef ret = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_READ,
                                                                           AHT10_I2C_BUS_ADDRESS,
                                                                           global_read_buffer, &global_read_buffer_size,
                                                                           NULL, NULL);
    if (ret == i2cTransferDone) break;
  }

  return (global_read_buffer);
}

/*******************************************************************************
 * Function name: write_to_sensor
 *
 * Description  : Function used to read information from AHT10 sensor
 * Parameters   : void
 * Returns      : returns global_read_buffer that is the array where read information is stored
 *
 * Known issues :
 * Note         : Size information is stored in global_read_buffer_size
 ******************************************************************************/
static void write_to_sensor(const uint8_t* cmd_byte_array)
{
  uint16_t write_cmd_size = DEFAULT_WRITE_CMD_SIZE;
  uint16_t write_timeout = DEFAULT_WRITE_TIMEOUT;
  while(write_timeout--){
    I2C_TransferReturn_TypeDef ret = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_WRITE,
                                                                           AHT10_I2C_BUS_ADDRESS,
                                                                           cmd_byte_array, &write_cmd_size,
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
 * Function name: decode_rh
 *
 * Description  : Function used to decode the array of bytes read from AHT10 and outputs the relative humidity
 * Parameters   : INPUT read_register - array of bytes read from AHT10
 *                OUTPUT humidity - relative humidity information decoded
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static void decode_rh(const uint8_t* read_register, float* humidity)
{
  if(is_sensor_busy(read_register)) return;

  uint32_t raw_humidity = ((uint32_t)read_register[1] << 12) | ((uint32_t)read_register[2] << 4) | ((uint32_t)read_register[3] >> 4);
  *humidity = ((float) raw_humidity / 1048576.0f) * 100.0f; // 2^20 = 1048576
}

/*******************************************************************************
 * Function name: decode_temp
 *
 * Description  : Function used to decode the array of bytes read from AHT10 and outputs the temperature
 * Parameters   : INPUT read_register - array of bytes read from AHT10
 *                OUTPUT temperature - temperature information decoded
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static void decode_temp(const uint8_t* read_register, float* temperature)
{
  if(is_sensor_busy(read_register)) return;

  uint32_t raw_temp = (((uint32_t)read_register[3] & 0x0F) << 16) | ((uint32_t)read_register[4] << 8) | (uint32_t)read_register[5];
  *temperature = (((float)raw_temp / 1048576.0f) * 200.0f) - 50.0f;
}

/*******************************************************************************
 * Function name: AHT10_init
 *
 * Description  : Function used to decode the array of bytes read from AHT10 and outputs the temperature
 * Parameters   : INPUT read_register - array of bytes read from AHT10
 *                OUTPUT temperature - temperature information decoded
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
void AHT10_init(void){
  gs_hal_i2c_class()->i2cInit(GS_PRIMARY_I2C, AHT10_SENSOR_SCL_PORT, AHT10_SENSOR_SCL_PIN, AHT10_SENSOR_SDA_PORT, AHT10_SENSOR_SDA_PIN);

  calibrate_sensor();

  sl_zigbee_event_init(&temperature_polling_event, temperature_polling_event_handler);
  sl_zigbee_event_set_delay_ms(&temperature_polling_event, TEMPERATURE_POLLING_PERIOD_MS);
}

/*******************************************************************************
 * Function name: calibrate_sensor
 *
 * Description  : Function used to calibrate the sensor
 * Parameters   : void
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
static void calibrate_sensor(void){
  write_to_sensor(calibrate_cmd);

  read_from_sensor();

#ifdef DEBUG
  if(is_sensor_calibrated(global_read_buffer)){
    printf("Sensor is calibrated!\r\n");
  }
  else{
    printf("Sensor is not calibrated!\r\n");
  }

  if(is_sensor_busy(global_read_buffer)){
    printf("Sensor is busy!\r\n");
  }
  else{
    printf("Sensor is not busy!\r\n");
  }

  for(int i = 0; i < global_read_buffer_size; i++){
    printf("Byte %d read: %02X\r\n", i, global_read_buffer[i]);
  }
#endif // DEBUG
}

/*******************************************************************************
 * Function name: measure_temperature_and_RH
 *
 * Description  : Function used to measure temperature and relative humidity through AHT10 sensor
 * Parameters   : void
 * Returns      : void
 *
 * Known issues :
 * Note         : Stores temperature and relative humidity in the static variables temperature and relative_humidity
 ******************************************************************************/
void measure_temperature_and_RH(void){
  read_from_sensor();
  if(!is_sensor_calibrated(global_read_buffer)) calibrate_sensor();

  write_to_sensor(trigger_measure_cmd);

  do {
    sl_sleeptimer_delay_millisecond(80);
    read_from_sensor();
  } while (is_sensor_busy(global_read_buffer));

  decode_rh(global_read_buffer, &relative_humidity);
  decode_temp(global_read_buffer, &temperature);
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
  printf("Relative Humidity = %.1f%%\r\n", relative_humidity);
  printf("Temperature = %.1f C\r\n", temperature);

  sl_zigbee_event_set_delay_ms(event, TEMPERATURE_POLLING_PERIOD_MS);
}




























