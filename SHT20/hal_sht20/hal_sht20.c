/******************************************************************************
 * File hal_sht20.c
 *
 *  Created on: 17 de ago. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "hal_sht20.h"
#include "../GS_stack/gs_hal_i2c.h"
#include "em_i2c.h"
#include <string.h>

/******************************************************************************
 * Defines
 *****************************************************************************/

//#define DEBUG
#ifdef DEBUG
#include <stdio.h>
#endif // DEBUG

#define USE_CRC
#ifdef USE_CRC
#define CRC_POLYNOMIAL 0x31
#endif // USE_CRC

#define SHT20_I2C_ADDRESS 0x80

#define TEMP_MEASURE_HOLD_MASTER_INSTRUCTION 0xE3
#define HUMIDITY_MEASURE_HOLD_MASTER_INSTRUCTION 0xE5
#define TEMP_MEASURE_NO_HOLD_MASTER_INSTRUCTION 0xF3
#define HUMIDITY_MEASURE_NO_HOLD_MASTER_INSTRUCTION 0xF5
#define WRITE_REGISTER_INSTRUCTION 0xE6
#define READ_REGISTER_INSTRUCTION 0xE7
#define SOFT_RESET_INSTRUCTION 0xFE

#define OTP_RELOAD_BIT_SHIFT 1
#define ON_CHIP_HEATER_BIT_SHIFT 2
#define END_OF_BAT_BIT_SHIFT 6
#define RESOLUTION_BIT_1_SHIFT 7
#define RESOLUTION_BIT_0_SHIFT 0

#define DEFAULT_WRITE_TIMEOUT 500
#define DEFAULT_WRITE_BUFFER_SIZE 4
#define DEFAULT_READ_BUFFER_SIZE 4

/******************************************************************************
 * Macros
 *****************************************************************************/

#define SET_BIT(byte, bit) ((byte) |= (1UL << (bit)))
#define CLEAR_BIT(byte, bit) ((byte) &= ~(1UL << (bit)))
#define TOGGLE_BIT(byte, bit) ((byte) ^= (1UL << (bit)))
#define CHECK_BIT(byte, bit) (((byte) >> (bit)) & 1U)

#define SET_BYTE_WITH_MASK(byte, mask) ((byte) |= (mask))
#define CLEAR_BYTE_WITH_MASK(byte, mask) ((byte) &= ~(mask))
#define TOGGLE_BYTE_WITH_MASK(byte, mask) ((byte) ^= (mask))

#define CLEAR_BYTE_UPTO_BIT(byte, bit) ((byte) = (((byte) >> (bit)) << (bit)))    // Clear the least significant piece from the byte
#define CLEAR_BYTE_DOWN_TO_BIT(byte, bit) ((byte) = ((uint8_t) ((byte) << (7 - bit)) >> (7 - bit))) // Clear the most significant piece from the byte
#define CLEAR_BYTE(byte) ((byte) = 0UL)

/******************************************************************************
 * Data types
 *****************************************************************************/

typedef struct {
  temperature_and_humidity_res_t resolution;
  bool is_heater_on;
  bool is_OTP_reload_on;
} st_sht20_config_t;

/******************************************************************************
 * Static Variables
 *****************************************************************************/

static st_sht20_config_t sht20_config = {
  .resolution = HUMIDITY_12BITS_TEMPERATURE_14BITS
};

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static hal_sht20_return_t send_command(uint8_t command, const uint8_t* additional_data, size_t data_size);
static hal_sht20_return_t send_command_and_read_response(uint8_t command, uint8_t* response, uint16_t* response_size);
static hal_sht20_return_t read_from_SHT20(uint8_t* read_data, uint16_t* read_data_size);
static hal_sht20_return_t decode_temp(const uint8_t* raw_temperature, float* temperature);
static hal_sht20_return_t decode_rh(const uint8_t* raw_humidity, float* humidity);
#ifdef USE_CRC
static bool is_crc_correct (const uint8_t* data_array, size_t data_size);
#endif // USE_CRC

/*******************************************************************************
 * Function name:
 *
 * Description  :
 * Parameters   :
 * Returns      :
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
hal_sht20_return_t SHT20_init(uint8_t scl_port, uint8_t scl_pin, uint8_t sda_port, uint8_t sda_pin)
{
  if(gs_hal_i2c_class()->i2cInit(GS_PRIMARY_I2C, scl_port, scl_pin, sda_port, sda_pin) != 0)
    return (SHT20_RETURN_I2C_ERR);

#ifdef DEBUG
  printf("Driver initialized!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

static hal_sht20_return_t send_command(uint8_t command, const uint8_t* additional_data, size_t data_size)
{
  // Input Validation
  if(additional_data == NULL) data_size = 0;

  // Formatting write array
  uint16_t write_size = 1 + data_size;
  uint8_t write_buffer[write_size];
  write_buffer[0] = command;
  if(data_size != 0){
    memcpy(&(write_buffer[1]), additional_data, data_size);
  }

  // I2C transfer
  I2C_TransferReturn_TypeDef write_status = i2cTransferInProgress; // @suppress("Symbol is not resolved")
  for(int write_timeout = DEFAULT_WRITE_TIMEOUT; write_status != i2cTransferDone; write_timeout--){ // @suppress("Symbol is not resolved")
    write_status = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_WRITE,
                                                         SHT20_I2C_ADDRESS,
                                                         write_buffer, &write_size,
                                                         NULL, NULL);
    if (write_timeout < 1) {
      return (SHT20_RETURN_TIMEOUT);
    }
  }

#ifdef DEBUG
  printf("Command sent!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

static hal_sht20_return_t send_command_and_read_response(uint8_t command, uint8_t* response, uint16_t* response_size)
{
  // Input Validation
  if(response == NULL || response_size == NULL) return (SHT20_RETURN_INVALID_ARG);
  if(*response_size < 1) return (SHT20_RETURN_INVALID_ARG);

  // Formatting write array
  uint16_t write_size = 1;
  uint8_t write_buffer[1];
  write_buffer[0] = command;

  // I2C transfer
  I2C_TransferReturn_TypeDef transfer_status = i2cTransferInProgress; // @suppress("Symbol is not resolved")
  for(int write_timeout = DEFAULT_WRITE_TIMEOUT; transfer_status != i2cTransferDone; write_timeout--){ // @suppress("Symbol is not resolved")
    transfer_status = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_WRITE_READ,
                                                         SHT20_I2C_ADDRESS,
                                                         write_buffer, &write_size,
                                                         response, response_size);
    if (write_timeout < 1) {
      return (SHT20_RETURN_TIMEOUT);
    }
  }

#ifdef DEBUG
  printf("Command sent and response read!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

static hal_sht20_return_t read_from_SHT20(uint8_t* read_data, uint16_t* read_data_size)
{
  // Input Validation
  if(read_data == NULL || read_data_size == NULL) return (SHT20_RETURN_INVALID_ARG);
  if(*read_data_size < 1) return (SHT20_RETURN_INVALID_ARG);

  // I2C transfer
  I2C_TransferReturn_TypeDef transfer_status = i2cTransferInProgress; // @suppress("Symbol is not resolved")
  for(int write_timeout = DEFAULT_WRITE_TIMEOUT; transfer_status != i2cTransferDone; write_timeout--){ // @suppress("Symbol is not resolved")
    transfer_status = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_READ,
                                                         SHT20_I2C_ADDRESS,
                                                         read_data, read_data_size,
                                                         NULL, NULL);
    if (write_timeout < 1) {
      return (SHT20_RETURN_TIMEOUT);
    }
  }

#ifdef DEBUG
  printf("Response read!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

static hal_sht20_return_t decode_temp(const uint8_t* raw_temperature, float* temperature)
{
  // Input Validation
  if(raw_temperature == NULL || temperature == NULL) return (SHT20_RETURN_INVALID_ARG);

  // Verify status bit
  bool is_status_correct = !CHECK_BIT(raw_temperature[1], 1);
  if(!is_status_correct) return (SHT20_RETURN_CORRUPT_DATA);

  // Merge bytes into a single variable
  uint16_t raw_temp_merged = ((uint16_t)raw_temperature[0] << 8) | (uint16_t)raw_temperature[1];

  // Clear status bits
  CLEAR_BIT(raw_temp_merged, 0);
  CLEAR_BIT(raw_temp_merged, 1);

  // Conversion formula
  *temperature = (((float)raw_temp_merged / 65536.0f) * 175.72f) - 46.85f;

#ifdef DEBUG
  printf("Temperature decoded!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

static hal_sht20_return_t decode_rh(const uint8_t* raw_humidity, float* humidity)
{
  // Input Validation
  if(raw_humidity == NULL || humidity == NULL) return (SHT20_RETURN_INVALID_ARG);

  // Verify status bit
  bool is_status_correct = CHECK_BIT(raw_humidity[1], 1);
  if(!is_status_correct) return (SHT20_RETURN_CORRUPT_DATA);

  // Merge bytes into a single variable
  uint16_t raw_rh_merged = ((uint16_t)raw_humidity[0] << 8) | (uint16_t)raw_humidity[1];

  // Clear status bits
  CLEAR_BIT(raw_rh_merged, 0);
  CLEAR_BIT(raw_rh_merged, 1);

  // Conversion formula
  *humidity = (((float)raw_rh_merged / 65536.0f) * 125.0f) - 6.0f;

#ifdef DEBUG
  printf("Temperature decoded!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t measure_temperature(float *temperature)
{
  // Input validation
  if(temperature == NULL) return (SHT20_RETURN_INVALID_ARG);

  // Get temperature measure info
  uint8_t temp_measure_response[DEFAULT_READ_BUFFER_SIZE];
  uint16_t response_size = sizeof(temp_measure_response);
  if(send_command(TEMP_MEASURE_NO_HOLD_MASTER_INSTRUCTION, NULL, 0) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(read_from_SHT20(temp_measure_response, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Check if data is good through CRC
#ifdef USE_CRC
  if(is_crc_correct(temp_measure_response, response_size) == false) return (SHT20_RETURN_CORRUPT_DATA);
#endif // USE_CRC

  // Decode raw temperature info into float
  if(decode_temp(temp_measure_response, temperature) != SHT20_RETURN_DONE) return (SHT20_RETURN_INTERNAL_ERR);

#ifdef DEBUG
  printf("Temperature measured!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t measure_humidity(float *humidity)
{
  // Input validation
  if(humidity == NULL) return (SHT20_RETURN_INVALID_ARG);

  // Get humidity measure info
  uint8_t humidity_measure_response[DEFAULT_READ_BUFFER_SIZE];
  uint16_t response_size = sizeof(humidity_measure_response);
  if(send_command(HUMIDITY_MEASURE_NO_HOLD_MASTER_INSTRUCTION, NULL, 0) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(read_from_SHT20(humidity_measure_response, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Check if data is good through CRC
#ifdef USE_CRC
  if(is_crc_correct(humidity_measure_response, response_size) == false) return (SHT20_RETURN_CORRUPT_DATA);
#endif // USE_CRC

  // Decode raw humidity info into float
  if(decode_rh(humidity_measure_response, humidity) != SHT20_RETURN_DONE) return (SHT20_RETURN_INTERNAL_ERR);

#ifdef DEBUG
  printf("Humidity measured!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t enable_on_chip_heater(void)
{
  // Get user register info
  uint8_t response_array[DEFAULT_READ_BUFFER_SIZE];
  uint16_t response_size = sizeof(response_array);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, response_array, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(response_size != 1) return (SHT20_RETURN_CORRUPT_DATA);

  // Set heater bit
  uint8_t user_register = response_array[0];
  SET_BIT(user_register, ON_CHIP_HEATER_BIT_SHIFT);

  // Send updated user register
  if(send_command(WRITE_REGISTER_INSTRUCTION, &user_register, 1) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Update global config struct
  sht20_config.is_heater_on = true;

#ifdef DEBUG
  printf("Heater enabled!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t disable_on_chip_heater(void)
{
  // Get user register info
  uint8_t response_array[DEFAULT_READ_BUFFER_SIZE];
  uint16_t response_size = sizeof(response_array);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, response_array, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(response_size != 1) return (SHT20_RETURN_CORRUPT_DATA);

  // Clear heater bit
  uint8_t user_register = response_array[0];
  CLEAR_BIT(user_register, ON_CHIP_HEATER_BIT_SHIFT);

  // Send updated user register
  if(send_command(WRITE_REGISTER_INSTRUCTION, &user_register, 1) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Update global config struct
  sht20_config.is_heater_on = false;

#ifdef DEBUG
  printf("Heater disabled!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t enable_OTP_reload(void)
{
  // Get user register info
  uint8_t response_array[DEFAULT_READ_BUFFER_SIZE];
  uint16_t response_size = sizeof(response_array);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, response_array, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(response_size != 1) return (SHT20_RETURN_CORRUPT_DATA);

  // Clear Disable OTP Reload bit
  uint8_t user_register = response_array[0];
  CLEAR_BIT(user_register, OTP_RELOAD_BIT_SHIFT);

  // Send updated user register
  if(send_command(WRITE_REGISTER_INSTRUCTION, &user_register, 1) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Update global config struct
  sht20_config.is_OTP_reload_on = true;

#ifdef DEBUG
  printf("OTP Reload enabled!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t disable_OTP_reload(void)
{
  // Get user register info
  uint8_t response_array[DEFAULT_READ_BUFFER_SIZE];
  uint16_t response_size = sizeof(response_array);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, response_array, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(response_size != 1) return (SHT20_RETURN_CORRUPT_DATA);

  // Set Disable OTP Reload bit
  uint8_t user_register = response_array[0];
  SET_BIT(user_register, OTP_RELOAD_BIT_SHIFT);

  // Send updated user register
  if(send_command(WRITE_REGISTER_INSTRUCTION, &user_register, 1) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Update global config struct
  sht20_config.is_OTP_reload_on = false;

#ifdef DEBUG
  printf("OTP Reload disabled!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t check_battery(bool *is_battery_good)
{
  // Input validation
  if(is_battery_good == NULL) return (SHT20_RETURN_INVALID_ARG);

  // Get user register info
  uint8_t response_array[DEFAULT_READ_BUFFER_SIZE];
  uint16_t response_size = sizeof(response_array);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, response_array, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(response_size != 1) return (SHT20_RETURN_CORRUPT_DATA);

  // Check end of battery bit
  uint8_t user_register = response_array[0];
  *is_battery_good = CHECK_BIT(user_register, END_OF_BAT_BIT_SHIFT);

#ifdef DEBUG
  printf("Battery checked!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t change_measure_resolution(temperature_and_humidity_res_t resolution)
{
  // Get user register info
  uint8_t response_array[DEFAULT_READ_BUFFER_SIZE];
  uint16_t response_size = sizeof(response_array);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, response_array, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(response_size != 1) return (SHT20_RETURN_CORRUPT_DATA);
  uint8_t user_register = response_array[0];

  switch (resolution) {
    case HUMIDITY_12BITS_TEMPERATURE_14BITS:
      CLEAR_BIT(user_register, RESOLUTION_BIT_0_SHIFT);
      CLEAR_BIT(user_register, RESOLUTION_BIT_1_SHIFT);
      break;
    case HUMIDITY_8BITS_TEMPERATURE_12BITS:
      SET_BIT(user_register, RESOLUTION_BIT_0_SHIFT);
      CLEAR_BIT(user_register, RESOLUTION_BIT_1_SHIFT);
      break;
    case HUMIDITY_10BITS_TEMPERATURE_13BITS:
      CLEAR_BIT(user_register, RESOLUTION_BIT_0_SHIFT);
      SET_BIT(user_register, RESOLUTION_BIT_1_SHIFT);
      break;
    case HUMIDITY_11BITS_TEMPERATURE_11BITS:
      SET_BIT(user_register, RESOLUTION_BIT_0_SHIFT);
      SET_BIT(user_register, RESOLUTION_BIT_1_SHIFT);
      break;
    default:
      return (SHT20_RETURN_INVALID_ARG);
      break;
  }

  // Send updated user register
  if(send_command(WRITE_REGISTER_INSTRUCTION, &user_register, 1) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Update global config struct
  sht20_config.resolution = resolution;

#ifdef DEBUG
  printf("Measure resolution changed!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t reset_SHT20(void)
{
  if(send_command(SOFT_RESET_INSTRUCTION, NULL, 0) != SHT20_RETURN_DONE) return (SHT20_RETURN_INTERNAL_ERR);

#ifdef DEBUG
  printf("SHT20 reset!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

#ifdef USE_CRC
static bool is_crc_correct (const uint8_t* data_array, size_t data_size)
{
  uint8_t crc = 0;
  /* handle each bit of input stream by iterating over each bit of each input byte */
  for(size_t i = 0; i < data_size; i++)
  {
    uint8_t byte = data_array[i];
      for (int j = 7; j >= 0; j--)
      {
          /* check if MSB is set */
          if ((crc & 0x80) != 0)
          {   /* MSB set, shift it out of the register */
              crc = (uint8_t)(crc << 1);
              /* shift in next bit of input stream:
               * If it's 1, set LSB of crc to 1.
               * If it's 0, set LSB of crc to 0. */
              crc = ((uint8_t)(byte & (1 << i)) != 0) ? (uint8_t)(crc | 0x01) : (uint8_t)(crc & 0xFE);
              /* Perform the 'division' by XORing the crc register with the generator polynomial */
              crc = (uint8_t)(crc ^ CRC_POLYNOMIAL);
          }
          else
          {   /* MSB not set, shift it out and shift in next bit of input stream. Same as above, just no division */
              crc = (uint8_t)(crc << 1);
              crc = ((uint8_t)(byte & (1 << i)) != 0) ? (uint8_t)(crc | 0x01) : (uint8_t)(crc & 0xFE);
          }
      }
  }
  return (crc == 0);
}
#endif // USE_CRC
