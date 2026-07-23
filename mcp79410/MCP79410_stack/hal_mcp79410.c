/******************************************************************************
 * File hal_mcp79410.c
 *
 *  Created on: 17 de jul. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "mcp79410_memory_map.h"
#include "mcp79410_config.h"
#include "hal_mcp79410.h"
#include "../GS_stack/gs_hal_i2c.h"
#include "em_i2c.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

//#define DEBUG

#define MCP79410_SRAM_AND_RTCC_REGISTER_I2C_BUS_ADDRESS     (0b01101111 << 1)           /**< I2C bus address*/
#define MCP79410_EEPROM_I2C_BUS_ADDRESS                     (0b01010111 << 1)           /**< I2C bus address*/

/******************************************************************************
 * Macros
 *****************************************************************************/

#define SET_BIT(byte, bit) ((byte) |= (1U << (bit)))
#define CLEAR_BIT(byte, bit) ((byte) &= ~(1U << (bit)))
#define TOGGLE_BIT(byte, bit) ((byte) ^= (1U << (bit)))
#define CHECK_BIT(byte, bit) (((byte) >> (bit)) & 1U)

#define SET_BYTE_WITH_MASK(byte, mask) ((byte) |= (mask))
#define CLEAR_BYTE_WITH_MASK(byte, mask) ((byte) &= ~(mask))
#define TOGGLE_BYTE_WITH_MASK(byte, mask) ((byte) ^= (mask))

#define CLEAR_BYTE_UPTO_BIT(byte, bit) ((byte) = (((byte) >> (bit)) << (bit)))    // Clear the least significant piece from the byte
#define CLEAR_BYTE_DOWN_TO_BIT(byte, bit) ((byte) = ((uint8_t) ((byte) << (7 - bit)) >> (7 - bit))) // Clear the most significant piece from the byte
#define CLEAR_BYTE(byte) ((byte) = 0U)

/******************************************************************************
 * Data types
 *****************************************************************************/

typedef struct {
  // Timekeeping Registers
  struct timekeeping_registers_t {
    uint8_t rtcsec_data;
    uint8_t rtcmin_data;
    uint8_t rtchour_data;
    uint8_t rtcwkday_data;
    uint8_t rtcdate_data;
    uint8_t rtcmth_data;
    uint8_t rtcyear_data;
    uint8_t control_data;
    uint8_t osctrim_data;
    uint8_t eeunlock_data;
  } __attribute__((packed)) timekeeping_registers;

  // Alarms Registers
  struct alarm_registers_t {
    uint8_t almsec_data;
    uint8_t almmin_data;
    uint8_t almhour_data;
    uint8_t almwkday_data;
    uint8_t almdate_data;
    uint8_t almmth_data;
  } __attribute__((packed)) alarm_registers[2];

  // Power-Fail Time-Stamp Registers
  struct power_fail_registers_t {
    uint8_t pwrdnmin_data;
    uint8_t pwrdnhour_data;
    uint8_t pwrdndate_data;
    uint8_t pwrdnmth_data;
    uint8_t pwrupmin_data;
    uint8_t pwruphour_data;
    uint8_t pwrupdate_data;
    uint8_t pwrupmth_data;
  } __attribute__((packed)) power_fail_registers;

} __attribute__((packed)) st_mcp79410_registers_t;

/******************************************************************************
 * Static Variables
 *****************************************************************************/

static st_mcp79410_registers_t mcp79410_registers;

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static I2C_TransferReturn_TypeDef write_to_register   (uint8_t register_address, const uint8_t* data_array, uint16_t data_size);
static I2C_TransferReturn_TypeDef read_from_register  (uint8_t register_address, uint8_t* read_data, uint16_t* read_data_size);
static void set_external_crystal                      (bool enable_external_crystal, st_mcp79410_registers_t *registers);
static void set_external_clock                        (bool enable_external_clock, st_mcp79410_registers_t *registers);
static void set_24hr_mode                             (bool enable_24hr_mode, st_mcp79410_registers_t *registers);
static void set_battery_mode                          (bool enable_battery, st_mcp79410_registers_t *registers);
static void set_timedate_on_registers_struct              (st_timedate_t timedate_data, st_mcp79410_registers_t *registers);
static void set_timedate_on_mcp79410_from_register_struct (const st_mcp79410_registers_t *registers);
static uint8_t binary_to_decimal                      (uint8_t binary);
static uint8_t decimal_to_binary                      (uint8_t decimal);
static void get_timedate_from_registers_struct            (st_timedate_t *timedate_data, const st_mcp79410_registers_t *registers);
static void get_timedate_from_mcp79410_on_register_struct (st_mcp79410_registers_t *registers);

/*******************************************************************************
 * Function name: MCP79410_init
 *
 * Description  : Initialize the MCP79410 RTCC
 * Parameters   : INPUT bool is_there_external_crystal - tells if there is an external crystal circuit to generate the MCP79410's clock,
 *                                                       if there is not an external crystal, the MCP79410 will expect an external clock
 *                                                       generated by another component
 *                INPUT bool is_24hr_mode              - tells whether the MCP79410 will use 24 or 12 hours
 *                INPUT bool is_battery_enabled        - tells whether the battery is enabled
 *                INPUT st_timedate_t initial_timedate - data of the initial time-date when you initialize the component
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
#ifdef DEBUG
__attribute__((optimize("O0")))
#endif // DEBUG
void MCP79410_init(bool is_there_external_crystal, bool is_24hr_mode, bool is_battery_enabled)
{
#if (defined(MCP79410_ENABLE_PORT) && defined(MCP79410_ENABLE_PIN))
  GPIO_PinModeSet(MCP79410_ENABLE_PORT, MCP79410_ENABLE_PIN, gpioModePushPull, 1);
#endif // (defined(MCP79410_ENABLE_PORT) && defined(MCP79410_ENABLE_PIN))

  gs_hal_i2c_class()->i2cInit(MCP79410_PERIPHERAL, MCP79410_SCL_PORT, MCP79410_SCL_PIN, MCP79410_SDA_PORT, MCP79410_SDA_PIN);

  get_timedate_from_mcp79410_on_register_struct(&mcp79410_registers);

  if(is_there_external_crystal) set_external_crystal(true, &mcp79410_registers);
  else set_external_clock(true, &mcp79410_registers);

  set_24hr_mode(is_24hr_mode, &mcp79410_registers);

  set_battery_mode(is_battery_enabled, &mcp79410_registers);


#ifdef DEBUG
  st_timedate_t read_test;
  get_timedate_from_registers_struct(&read_test, &mcp79410_registers);

  printf("MCP79410 initialized!\r\n");
#endif // DEBUG
}

static I2C_TransferReturn_TypeDef write_to_register(uint8_t register_address, const uint8_t* data_array, uint16_t data_size)
{
  if(data_array == NULL) return (i2cTransferUsageFault); // @suppress("Symbol is not resolved")
  if(register_address >= MCP79410_RTCC_REGISTERS_END_ADDRESS){
    return (i2cTransferUsageFault); // @suppress("Symbol is not resolved")
  }

  uint8_t write_data[data_size + 1];
  write_data[0] = register_address;
  memcpy(write_data + 1, data_array, data_size);
  data_size++;

  I2C_TransferReturn_TypeDef ret = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_WRITE,
                                                                         MCP79410_SRAM_AND_RTCC_REGISTER_I2C_BUS_ADDRESS,
                                                                         write_data, &data_size,
                                                                         NULL, NULL);

  return (ret);
}

static I2C_TransferReturn_TypeDef read_from_register(uint8_t register_address, uint8_t* read_data, uint16_t* read_data_size)
{
  if(read_data == NULL || read_data_size == NULL) return (i2cTransferUsageFault); // @suppress("Symbol is not resolved")
  if(register_address >= MCP79410_RTCC_REGISTERS_END_ADDRESS){
    return (i2cTransferUsageFault); // @suppress("Symbol is not resolved")
  }

  uint16_t write_size = 1;
  uint8_t write_data[1] = {register_address};

  I2C_TransferReturn_TypeDef ret = gs_hal_i2c_class()->i2cSimpleTransfer(I2C_FLAG_WRITE_READ,
                                                                         MCP79410_SRAM_AND_RTCC_REGISTER_I2C_BUS_ADDRESS,
                                                                         write_data, &write_size,
                                                                         read_data, read_data_size);

  return (ret);
}

static void set_external_crystal(bool enable_external_crystal, st_mcp79410_registers_t *registers)
{
  if(enable_external_crystal){
    SET_BIT(registers->timekeeping_registers.rtcsec_data, ST_OFFSET);
    CLEAR_BIT(registers->timekeeping_registers.control_data, EXTOSC_OFFSET);
  }
  else{
    CLEAR_BIT(registers->timekeeping_registers.rtcsec_data, ST_OFFSET);
  }
}

static void set_external_clock(bool enable_external_clock, st_mcp79410_registers_t *registers)
{
  if(enable_external_clock){
    SET_BIT(registers->timekeeping_registers.control_data, EXTOSC_OFFSET);
    CLEAR_BIT(registers->timekeeping_registers.rtcsec_data, ST_OFFSET);
  }
  else{
    CLEAR_BIT(registers->timekeeping_registers.control_data, EXTOSC_OFFSET);
  }
}

static void set_24hr_mode(bool enable_24hr_mode, st_mcp79410_registers_t *registers)
{
  if(enable_24hr_mode){
    CLEAR_BIT(registers->timekeeping_registers.rtchour_data, BIT_12_24_OFFSET);
    CLEAR_BIT(registers->alarm_registers[0].almhour_data, BIT_12_24_OFFSET);
    CLEAR_BIT(registers->alarm_registers[1].almhour_data, BIT_12_24_OFFSET);
    CLEAR_BIT(registers->power_fail_registers.pwrdnhour_data, BIT_12_24_OFFSET);
    CLEAR_BIT(registers->power_fail_registers.pwruphour_data, BIT_12_24_OFFSET);
  }
  else{
    SET_BIT(registers->timekeeping_registers.rtchour_data, BIT_12_24_OFFSET);
    SET_BIT(registers->alarm_registers[0].almhour_data, BIT_12_24_OFFSET);
    SET_BIT(registers->alarm_registers[1].almhour_data, BIT_12_24_OFFSET);
    SET_BIT(registers->power_fail_registers.pwrdnhour_data, BIT_12_24_OFFSET);
    SET_BIT(registers->power_fail_registers.pwruphour_data, BIT_12_24_OFFSET);
  }
}

static void set_battery_mode(bool enable_battery, st_mcp79410_registers_t *registers)
{
  if(enable_battery){
    SET_BIT(registers->timekeeping_registers.rtcwkday_data, VBATEN_OFFSET);
  }
  else{
    CLEAR_BIT(registers->timekeeping_registers.rtcwkday_data, VBATEN_OFFSET);
  }
}

static void set_timedate_on_registers_struct(st_timedate_t timedate_data, st_mcp79410_registers_t *registers)
{
  // Input validation
  if(!is_timedate_valid(timedate_data)) return;
  if(registers == NULL) return;

  struct timekeeping_registers_t *rtc_registers = &registers->timekeeping_registers;

  // Converting binary format to decimal
  uint8_t rtcsec_time_data    = binary_to_decimal(timedate_data.seconds);
  uint8_t rtcmin_time_data    = binary_to_decimal(timedate_data.minutes);
  uint8_t rtchour_time_data   = binary_to_decimal(timedate_data.hours);
  uint8_t rtcwkday_time_data  = binary_to_decimal(timedate_data.weekday);
  uint8_t rtcdate_time_data   = binary_to_decimal(timedate_data.date);
  uint8_t rtcmth_time_data    = binary_to_decimal(timedate_data.month);
  uint8_t rtcyear_time_data   = binary_to_decimal(timedate_data.year % 100);

  // Handling 12-24 hour format
  if(!timedate_data.is_24hr_mode){
    SET_BIT(rtchour_time_data, BIT_12_24_OFFSET);

    if(timedate_data.is_pm) SET_BIT(rtchour_time_data, HRTEN1_OFFSET);
    else CLEAR_BIT(rtchour_time_data, HRTEN1_OFFSET);
  }
  else{
    CLEAR_BIT(rtchour_time_data, BIT_12_24_OFFSET);
  }

  // Handling leap year
  if(timedate_data.is_leap_year) SET_BIT(rtcmth_time_data, LPYR_WKDAY0_OFFSET);
  else CLEAR_BIT(rtcmth_time_data, LPYR_WKDAY0_OFFSET);

  // Clearing previous time data in the registers
  CLEAR_BYTE_WITH_MASK(rtc_registers->rtcsec_data   , MCP79410_RTCSEC_TIME_DATA_MASK);
  CLEAR_BYTE_WITH_MASK(rtc_registers->rtcmin_data   , MCP79410_RTCMIN_TIME_DATA_MASK);
  CLEAR_BYTE_WITH_MASK(rtc_registers->rtchour_data  , MCP79410_RTCHOUR_TIME_DATA_MASK);
  CLEAR_BYTE_WITH_MASK(rtc_registers->rtcwkday_data , MCP79410_RTCWKDAY_TIME_DATA_MASK);
  CLEAR_BYTE_WITH_MASK(rtc_registers->rtcdate_data  , MCP79410_RTCDATE_TIME_DATA_MASK);
  CLEAR_BYTE_WITH_MASK(rtc_registers->rtcmth_data   , MCP79410_RTCMTH_TIME_DATA_MASK);
  CLEAR_BYTE_WITH_MASK(rtc_registers->rtcyear_data  , MCP79410_RTCYEAR_TIME_DATA_MASK);

  // Writing new time data into the registers
  rtc_registers->rtcsec_data    |= rtcsec_time_data;
  rtc_registers->rtcmin_data    |= rtcmin_time_data;
  rtc_registers->rtchour_data   |= rtchour_time_data;
  rtc_registers->rtcwkday_data  |= rtcwkday_time_data;
  rtc_registers->rtcdate_data   |= rtcdate_time_data;
  rtc_registers->rtcmth_data    |= rtcmth_time_data;
  rtc_registers->rtcyear_data   |= rtcyear_time_data;
}

static void set_timedate_on_mcp79410_from_register_struct(const st_mcp79410_registers_t *registers)
{
  // Input validation
  if(registers == NULL) return;

  const struct timekeeping_registers_t *rtc_registers = &registers->timekeeping_registers;
  uint16_t write_size = sizeof(struct timekeeping_registers_t);

  write_to_register(MCP79410_RTCC_REGISTER_RTCSEC_ADDRESS, (const uint8_t *) rtc_registers, write_size);
}

bool is_timedate_valid(st_timedate_t timedate_data)
{
  bool ret = true;
  if(timedate_data.seconds >= 60) ret = false;
  else if(timedate_data.minutes >= 60) ret = false;
  else if(timedate_data.hours >= 12 && timedate_data.is_24hr_mode == false) ret = false;
  else if(timedate_data.hours >= 24 && timedate_data.is_24hr_mode == true) ret = false;
  else if(timedate_data.weekday >= 8) ret = false;
  else if(timedate_data.date >= 32) ret = false;
  else if(timedate_data.month > 12) ret = false;
  else if(timedate_data.year > 99) ret = false;
  else if(timedate_data.is_24hr_mode != true && timedate_data.is_24hr_mode != false) ret = false;
  else if(timedate_data.is_leap_year != true && timedate_data.is_leap_year != false) ret = false;
  else if(timedate_data.is_pm != true && timedate_data.is_pm != false) ret = false;

  return (ret);
}

static uint8_t binary_to_decimal(uint8_t binary)
{
  uint8_t ones_digit = binary % 10;
  uint8_t tens_digit = (binary / 10) % 10;

  uint8_t ret = ((tens_digit << 4) | ones_digit);
  return (ret);
}

static uint8_t decimal_to_binary(uint8_t decimal)
{
  uint8_t ones_digit = (decimal & 0b00001111);
  uint8_t tens_digit = ((decimal & 0b11110000) >> 4);

  uint8_t ret = ((tens_digit * 10) + ones_digit);
  return (ret);
}

static void get_timedate_from_registers_struct (st_timedate_t *timedate_data, const st_mcp79410_registers_t *registers)
{
  // Input validation
  if(timedate_data == NULL || registers == NULL) return;

  const struct timekeeping_registers_t rtc_registers = registers->timekeeping_registers;

  // Retriving raw data from the registers
  uint8_t rtcsec_time_data    = (rtc_registers.rtcsec_data);
  uint8_t rtcmin_time_data    = (rtc_registers.rtcmin_data);
  uint8_t rtchour_time_data   = (rtc_registers.rtchour_data);
  uint8_t rtcwkday_time_data  = (rtc_registers.rtcwkday_data);
  uint8_t rtcdate_time_data   = (rtc_registers.rtcdate_data);
  uint8_t rtcmth_time_data    = (rtc_registers.rtcmth_data);
  uint8_t rtcyear_time_data   = (rtc_registers.rtcyear_data);

  // Clearing all data except the relevant time data
  CLEAR_BYTE_WITH_MASK(rtcsec_time_data   , ~(MCP79410_RTCSEC_TIME_DATA_MASK));
  CLEAR_BYTE_WITH_MASK(rtcmin_time_data   , ~(MCP79410_RTCMIN_TIME_DATA_MASK));
  CLEAR_BYTE_WITH_MASK(rtchour_time_data  , ~(MCP79410_RTCHOUR_TIME_DATA_MASK));
  CLEAR_BYTE_WITH_MASK(rtcwkday_time_data , ~(MCP79410_RTCWKDAY_TIME_DATA_MASK));
  CLEAR_BYTE_WITH_MASK(rtcdate_time_data  , ~(MCP79410_RTCDATE_TIME_DATA_MASK));
  CLEAR_BYTE_WITH_MASK(rtcmth_time_data   , ~(MCP79410_RTCMTH_TIME_DATA_MASK));
  CLEAR_BYTE_WITH_MASK(rtcyear_time_data  , ~(MCP79410_RTCYEAR_TIME_DATA_MASK));

  // Handling the 12-24hr format
  bool is_24hr_format = !(CHECK_BIT(rtchour_time_data, BIT_12_24_OFFSET));
  if(is_24hr_format){
    timedate_data->is_24hr_mode = true;
  }
  else{
    timedate_data->is_24hr_mode = false;
    timedate_data->is_pm = CHECK_BIT(rtchour_time_data, HRTEN1_OFFSET);
    CLEAR_BIT(rtchour_time_data, HRTEN1_OFFSET);
  }
  CLEAR_BIT(rtchour_time_data, BIT_12_24_OFFSET);

  // Handling leap year
  timedate_data->is_leap_year = CHECK_BIT(rtcmth_time_data, LPYR_WKDAY0_OFFSET);
  CLEAR_BIT(rtcmth_time_data, LPYR_WKDAY0_OFFSET);

  // Converting decimal format to binary
  timedate_data->seconds  = decimal_to_binary(rtcsec_time_data);
  timedate_data->minutes  = decimal_to_binary(rtcmin_time_data);
  timedate_data->hours    = decimal_to_binary(rtchour_time_data);
  timedate_data->weekday  = decimal_to_binary(rtcwkday_time_data);
  timedate_data->date     = decimal_to_binary(rtcdate_time_data);
  timedate_data->month    = decimal_to_binary(rtcmth_time_data);
  timedate_data->year     = decimal_to_binary(rtcyear_time_data);
}

static void get_timedate_from_mcp79410_on_register_struct(st_mcp79410_registers_t *registers)
{
  // Input validation
  if(registers == NULL) return;

  struct timekeeping_registers_t *rtc_registers = &registers->timekeeping_registers;
  uint16_t read_size = sizeof(struct timekeeping_registers_t);

  read_from_register(MCP79410_RTCC_REGISTER_RTCSEC_ADDRESS, (uint8_t *) rtc_registers, &read_size);
}

void set_timedate(st_timedate_t time){
  set_timedate_on_registers_struct(time, &mcp79410_registers);
  set_timedate_on_mcp79410_from_register_struct(&mcp79410_registers);
}

void set_time(st_timedate_t time)
{

}

void set_date(st_timedate_t time)
{

}

void get_timedate(st_timedate_t *time){
  get_timedate_from_mcp79410_on_register_struct(&mcp79410_registers);
  get_timedate_from_registers_struct(time, &mcp79410_registers);
}


















