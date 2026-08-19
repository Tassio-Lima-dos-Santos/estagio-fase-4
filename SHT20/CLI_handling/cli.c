/***************************************************************************//**
 * @file
 * @brief cli bare metal examples functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <string.h>
#include <stdio.h>
#include "cli.h"
#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"
#include "sl_assert.h"
#include "sl_sleeptimer.h"
#include "em_device.h"

#include "../hal_sht20/hal_sht20.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

void hello_cli_callback                     (sl_cli_command_arg_t *arguments);

void uptime_cli_callback                    (sl_cli_command_arg_t *arguments);
void reset_cli_callback                     (sl_cli_command_arg_t *arguments);

void measure_temperature_cli_callback       (sl_cli_command_arg_t *arguments);
void measure_humidity_cli_callback          (sl_cli_command_arg_t *arguments);
void on_chip_heater_cli_callback            (sl_cli_command_arg_t *arguments);
void OTP_reload_cli_callback                (sl_cli_command_arg_t *arguments);
void check_battery_cli_callback             (sl_cli_command_arg_t *arguments);
void change_measure_resolution_cli_callback (sl_cli_command_arg_t *arguments);
void reset_sht20_cli_callback               (sl_cli_command_arg_t *arguments);

#if SL_SIMPLE_BUTTON_COUNT > 0
void button_callback      (sl_cli_command_arg_t *arguments);
void wait_button_callback (sl_cli_command_arg_t *arguments);
#endif

#if SL_SIMPLE_LED_COUNT > 0
void set_led_cli_callback         (sl_cli_command_arg_t *arguments);
void blink_led_cli_callback   (sl_cli_command_arg_t *arguments);
#endif

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

/***************************************************************************//**
 * Command info for print related commands
 ******************************************************************************/

static const sl_cli_command_info_t cmd__hello = \
  SL_CLI_COMMAND(hello_cli_callback,
                 "Print \"Hello, world!\" to the terminal",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t print_table[] = {
  { "hello", &cmd__hello, false },

  { NULL, NULL, false },
};

static const sl_cli_command_info_t cmd_group__print_table = \
  SL_CLI_COMMAND_GROUP(print_table, "Print related commands");

/***************************************************************************//**
 * Command info for system related commands
 ******************************************************************************/

static const sl_cli_command_info_t cmd__uptime = \
  SL_CLI_COMMAND(uptime_cli_callback,
                 "Shows for how long the microcontroller has been running",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__reset = \
  SL_CLI_COMMAND(reset_cli_callback,
                 "Reset the microcontroller (Doesn't save current state if you don't specify it)",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t system_table[] = {
  { "uptime", &cmd__uptime, false },
  { "reset", &cmd__reset, false },

  { NULL, NULL, false },
};

static const sl_cli_command_info_t cmd_group__system_table = \
  SL_CLI_COMMAND_GROUP(system_table, "System related commands");

/***************************************************************************//**
 * Command info for sht20 related commands
 ******************************************************************************/

static const sl_cli_command_info_t cmd__measure_temperature = \
  SL_CLI_COMMAND(measure_temperature_cli_callback,
                 "Measure temperature through SHT20 sensor",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__measure_humidity = \
  SL_CLI_COMMAND(measure_humidity_cli_callback,
                 "Measure humidity through SHT20 sensor",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__on_chip_heater = \
  SL_CLI_COMMAND(on_chip_heater_cli_callback,
                 "Enable or disable the SHT20 on-chip heater",
                 "Enable or disable: <1|0>",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__OTP_reload = \
  SL_CLI_COMMAND(OTP_reload_cli_callback,
                 "Enable or disable the SHT20 OTP reload feature",
                 "Enable or disable: <1|0>",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__check_battery = \
  SL_CLI_COMMAND(check_battery_cli_callback,
                 "Check battery through SHT20",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__change_measure_resolution = \
  SL_CLI_COMMAND(change_measure_resolution_cli_callback,
                 "Change SHT20 measure resolution",
                 "Humidity 12 bits and temperature 14 bits: 0\r\n"
                 "Humidity 8 bits and temperature 12 bits: 1\r\n"
                 "Humidity 10 bits and temperature 13 bits: 2\r\n"
                 "Humidity 11 bits and temperature 11 bits: 3",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__reset_sht20 = \
  SL_CLI_COMMAND(reset_sht20_cli_callback,
                 "Resets SHT20 sensor",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t sht20_table[] = {
  { "measure_temperature", &cmd__measure_temperature, false },
  { "measure_humidity", &cmd__measure_humidity, false },
  { "on_chip_heater", &cmd__on_chip_heater, false },
  { "OTP_reload", &cmd__OTP_reload, false },
  { "check_battery", &cmd__check_battery, false },
  { "change_measure_resolution", &cmd__change_measure_resolution, false },
  { "reset", &cmd__reset_sht20, false },

  { NULL, NULL, false },
};

static const sl_cli_command_info_t cmd_group__sht20_table = \
  SL_CLI_COMMAND_GROUP(sht20_table, "SHT20 related commands");

static sl_cli_command_entry_t main_table[] = {
  { "print_group", &cmd_group__print_table, false },
  { "system", &cmd_group__system_table, false },
  { "sht20", &cmd_group__sht20_table, false },

  { NULL, NULL, false },
};

static sl_cli_command_group_t main_group = {
  { NULL },
  false,
  main_table
};

/*******************************************************************************
 *************************  EXPORTED VARIABLES   *******************************
 ******************************************************************************/

sl_cli_command_group_t *command_group = &main_group;

/*******************************************************************************
 ***************************   LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Callback definitions for generic commands
 ******************************************************************************/

/***************************************************************************//**
 * Callback for hello
 *
 * This function is used as a callback when the hello command is called
 * in the cli. It simply echoes back "Hello, world!".
 ******************************************************************************/
void hello_cli_callback(sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  printf("Hello, world!\r\n");
}

/***************************************************************************//**
 * Callback for uptime
 *
 * This function is used as a callback when the uptime command is called
 * in the cli. It simply shows the MCU uptime.
 ******************************************************************************/
void uptime_cli_callback(sl_cli_command_arg_t *arguments){
  (void) arguments;

  uint32_t ticks = sl_sleeptimer_get_tick_count();
  uint32_t uptime_ms = sl_sleeptimer_tick_to_ms(ticks);

  uint32_t uptime_sec  = (uptime_ms / 1000);
  uint8_t uptime_min   = (uptime_sec / 60) % 60;
  uint32_t uptime_hour = (uptime_sec / 3600);
  uptime_sec %= 60;
  uptime_ms  %= 1000;

  printf("Uptime: %lu:%u:%lu:%lu\r\n", uptime_hour, uptime_min, uptime_sec, uptime_ms);
}

/***************************************************************************//**
 * Callback for reset
 *
 * This function is used as a callback when the reset command is called
 * in the cli. It resets the MCU. You can save the current state by passing save as an argument.
 ******************************************************************************/
void reset_cli_callback(sl_cli_command_arg_t *arguments){
  printf("Resetting system!\r\n");
  sl_sleeptimer_delay_millisecond(1000);
  NVIC_SystemReset();
}

void measure_temperature_cli_callback       (sl_cli_command_arg_t *arguments)
{
  (void) arguments;

  float temperature = 0;

  hal_sht20_return_t status;
  status = measure_temperature(&temperature);
  switch (status) {
    case SHT20_RETURN_DONE:
      printf("Temperature = %.2f C\r\n", temperature);
      break;
    case SHT20_RETURN_CORRUPT_DATA:
      printf("Temperature data corrupted!\r\n");
      reset_SHT20();
      break;
    default:
      printf("Error reading temperature\r\n");
      reset_SHT20();
      break;
  }
}

void measure_humidity_cli_callback          (sl_cli_command_arg_t *arguments)
{
  (void) arguments;

  float humidity = 0;

  hal_sht20_return_t status;
  status = measure_humidity(&humidity);
  switch (status) {
    case SHT20_RETURN_DONE:
      printf("Relative Humidity = %.2f%%\r\n", humidity);
      break;
    case SHT20_RETURN_CORRUPT_DATA:
      printf("Humidity data corrupted!\r\n");
      reset_SHT20();
      break;
    default:
      printf("Error reading humidity\r\n");
      reset_SHT20();
      break;
  }
}

void on_chip_heater_cli_callback            (sl_cli_command_arg_t *arguments)
{
  bool enable = sl_cli_get_argument_uint8(arguments, 0);
  hal_sht20_return_t status;
  if(enable) status = enable_on_chip_heater();
  else status = disable_on_chip_heater();

  if(status != SHT20_RETURN_DONE) printf("Error!\r\n");
  else if(enable) printf("On-chip heater turned on!\r\n");
  else printf("On-chip heater turned off!\r\n");
}

void OTP_reload_cli_callback                (sl_cli_command_arg_t *arguments)
{
  bool enable = sl_cli_get_argument_uint8(arguments, 0);
  hal_sht20_return_t status;
  if(enable) status = enable_OTP_reload();
  else status = disable_OTP_reload();

  if(status != SHT20_RETURN_DONE) printf("Error!\r\n");
  else if(enable) printf("OTP reload feature enabled!\r\n");
  else printf("OTP reload feature disabled!\r\n");
}

void check_battery_cli_callback             (sl_cli_command_arg_t *arguments)
{
  (void) arguments;

  bool is_battery_ok = false;
  hal_sht20_return_t status;
  status = check_battery(&is_battery_ok);
  switch (status) {
    case SHT20_RETURN_DONE:
      if(is_battery_ok) printf("Battery is ok!\r\n");
      else printf("Battery is not good!\r\n");
      break;
    default:
      printf("Error reading battery status\r\n");
      reset_SHT20();
      break;
  }
}

void change_measure_resolution_cli_callback (sl_cli_command_arg_t *arguments)
{
  temperature_and_humidity_res_t resolution = sl_cli_get_argument_uint8(arguments, 0);
  hal_sht20_return_t status;
  status = change_measure_resolution(resolution);

  if(status == SHT20_RETURN_INVALID_ARG) printf("Invalid Argument!\r\n");
  else if(status == SHT20_RETURN_DONE) printf("Measure resolution changed!\r\n");
  else printf("Error!\r\n");
}

void reset_sht20_cli_callback (sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  hal_sht20_return_t status;
  status = reset_SHT20();

  if(status == SHT20_RETURN_DONE) printf("SHT20 reset!\r\n");
  else printf("Error!\r\n");
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/*******************************************************************************
 * Initialize cli example.
 ******************************************************************************/
void cli_app_init(void)
{
  bool status;

  status = sl_cli_command_add_command_group(sl_cli_example_handle, command_group);
  EFM_ASSERT(status);

  printf("\r\nStarted CLI Bare-metal\r\n\r\n");
}

/***************************************************************************//**
 * Ticking function
 ******************************************************************************/
void cli_app_process_action(void)
{
}
