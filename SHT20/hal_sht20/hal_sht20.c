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

//#define TEST_CRC
#ifdef TEST_CRC
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#endif // TEST_CRC

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
#define READ_TEMP_BUFFER_SIZE 3
#define READ_HUMIDITY_BUFFER_SIZE 3

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

static uint8_t crc_LUT[] = {
    0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97, 0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E,
    0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4, 0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
    0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11, 0x3F, 0x0E, 0x5D, 0x6C, 0xFB, 0xCA, 0x99, 0xA8,
    0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52, 0x7C, 0x4D, 0x1E, 0x2F, 0xB8, 0x89, 0xDA, 0xEB,
    0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA, 0x84, 0xB5, 0xE6, 0xD7, 0x40, 0x71, 0x22, 0x13,
    0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9, 0xC7, 0xF6, 0xA5, 0x94, 0x03, 0x32, 0x61, 0x50,
    0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C, 0x02, 0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95,
    0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F, 0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6,
    0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC, 0xED, 0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54,
    0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC, 0x9F, 0xAE, 0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17,
    0xFC, 0xCD, 0x9E, 0xAF, 0x38, 0x09, 0x5A, 0x6B, 0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2,
    0xBF, 0x8E, 0xDD, 0xEC, 0x7B, 0x4A, 0x19, 0x28, 0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91,
    0x47, 0x76, 0x25, 0x14, 0x83, 0xB2, 0xE1, 0xD0, 0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69,
    0x04, 0x35, 0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93, 0xBD, 0x8C, 0xDF, 0xEE, 0x79, 0x48, 0x1B, 0x2A,
    0xC1, 0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56, 0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
    0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15, 0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D, 0xAC,
};

static st_sht20_config_t sht20_config = {
  .resolution = HUMIDITY_12BITS_TEMPERATURE_14BITS
};

#ifdef TEST_CRC
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_entropy_context entropy;
#endif // TEST_CRC

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

#ifdef TEST_CRC
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);

  mbedtls_ctr_drbg_seed(&ctr_drbg,
                         mbedtls_entropy_func,
                         &entropy,
                         (const unsigned char *)"MY_SEED", 7);
#endif // TEST_CRC

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
  uint8_t temp_measure_response[READ_TEMP_BUFFER_SIZE];
  uint16_t response_size = sizeof(temp_measure_response);
  if(send_command(TEMP_MEASURE_NO_HOLD_MASTER_INSTRUCTION, NULL, 0) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(read_from_SHT20(temp_measure_response, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

#ifdef TEST_CRC
  uint8_t random_value;
  mbedtls_ctr_drbg_random(&ctr_drbg, &random_value, sizeof(random_value));
  if(!(random_value % 5)) SET_BIT(temp_measure_response[2], 0);
#endif // TEST_CRC

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
  uint8_t humidity_measure_response[READ_HUMIDITY_BUFFER_SIZE];
  uint16_t response_size = sizeof(humidity_measure_response);
  if(send_command(HUMIDITY_MEASURE_NO_HOLD_MASTER_INSTRUCTION, NULL, 0) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);
  if(read_from_SHT20(humidity_measure_response, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

#ifdef TEST_CRC
  uint8_t random_value;
  mbedtls_ctr_drbg_random(&ctr_drbg, &random_value, sizeof(random_value));
  if(!(random_value % 5)) SET_BIT(humidity_measure_response[2], 0);
#endif // TEST_CRC

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
  uint8_t user_register = 0;
  uint16_t response_size = sizeof(user_register);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, &user_register, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Set heater bit
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
  uint8_t user_register = 0;
  uint16_t response_size = sizeof(user_register);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, &user_register, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Clear heater bit
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
  uint8_t user_register = 0;
  uint16_t response_size = sizeof(user_register);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, &user_register, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Clear Disable OTP Reload bit
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
  uint8_t user_register = 0;
  uint16_t response_size = sizeof(user_register);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, &user_register, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Set Disable OTP Reload bit
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
  uint8_t user_register = 0;
  uint16_t response_size = sizeof(user_register);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, &user_register, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

  // Check end of battery bit
  *is_battery_good = !CHECK_BIT(user_register, END_OF_BAT_BIT_SHIFT);

#ifdef DEBUG
  printf("Battery checked!\r\n");
#endif // DEBUG
  return (SHT20_RETURN_DONE);
}

hal_sht20_return_t change_measure_resolution(temperature_and_humidity_res_t resolution)
{
  // Get user register info
  uint8_t user_register = 0;
  uint16_t response_size = sizeof(user_register);
  if(send_command_and_read_response(READ_REGISTER_INSTRUCTION, &user_register, &response_size) != SHT20_RETURN_DONE)
    return (SHT20_RETURN_INTERNAL_ERR);

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
static uint8_t calculate_crc (const uint8_t* data_array, size_t data_size)
{
  uint8_t crc = 0;
  /* handle each bit of input stream by iterating over each bit of each input byte */
  for(size_t i = 0; i < data_size; i++)
  {
    uint8_t byte = data_array[i];

    /* XOR-in next input byte */
    uint8_t data = (uint8_t)(byte ^ crc);
    /* get current CRC value = remainder */
    crc = (uint8_t)(crc_LUT[data]);
  }
  return (crc);
}

static bool is_crc_correct (const uint8_t* data_array, size_t data_size)
{
  uint8_t crc = calculate_crc(data_array, data_size);
  return (crc == 0);
}
#endif // USE_CRC
