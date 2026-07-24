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
#include "../MCP79410_stack/hal_mcp79410.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

void hello_cli_callback            (sl_cli_command_arg_t *arguments);

void uptime_cli_callback        (sl_cli_command_arg_t *arguments);
void reset_cli_callback        (sl_cli_command_arg_t *arguments);

void set_datetime_cli_callback(sl_cli_command_arg_t *arguments);
void set_time_cli_callback(sl_cli_command_arg_t *arguments);
void set_date_cli_callback(sl_cli_command_arg_t *arguments);
void get_datetime_cli_callback(sl_cli_command_arg_t *arguments);
void get_time_cli_callback(sl_cli_command_arg_t *arguments);
void get_date_cli_callback(sl_cli_command_arg_t *arguments);

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
 * Command info for real time related commands
 ******************************************************************************/

static const sl_cli_command_info_t cmd__real_time_set_datetime = \
  SL_CLI_COMMAND(set_datetime_cli_callback,
                 "Set the datetime in the RTC",
                 "Date"SL_CLI_UNIT_SEPARATOR"Month"SL_CLI_UNIT_SEPARATOR"Year (Only the 2 last digits, i.e. 2026 is 26)"SL_CLI_UNIT_SEPARATOR"Hours"SL_CLI_UNIT_SEPARATOR"Minutes"SL_CLI_UNIT_SEPARATOR"Seconds",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__real_time_set_time = \
  SL_CLI_COMMAND(set_time_cli_callback,
                 "Set the time in the RTC",
                 "Hours"SL_CLI_UNIT_SEPARATOR"Minutes"SL_CLI_UNIT_SEPARATOR"Seconds",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__real_time_set_date = \
  SL_CLI_COMMAND(set_date_cli_callback,
                 "Set the time in the RTC",
                 "Date"SL_CLI_UNIT_SEPARATOR"Month"SL_CLI_UNIT_SEPARATOR"Year (Only the 2 last digits, i.e. 2026 is 26)",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__real_time_get_datetime = \
  SL_CLI_COMMAND(get_datetime_cli_callback,
                 "Gets the datetime from RTC",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__real_time_get_time = \
  SL_CLI_COMMAND(get_time_cli_callback,
                 "Gets the time from RTC",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__real_time_get_date = \
  SL_CLI_COMMAND(get_date_cli_callback,
                 "Gets the date from RTC",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t real_time_table[] = {
  { "set_datetime", &cmd__real_time_set_datetime, false },
  { "set_time", &cmd__real_time_set_time, false },
  { "set_date", &cmd__real_time_set_date, false },
  { "get_datetime", &cmd__real_time_get_datetime, false },
  { "get_time", &cmd__real_time_get_time, false },
  { "get_date", &cmd__real_time_get_date, false },

  { NULL, NULL, false },
};

static const sl_cli_command_info_t cmd_group__real_time_table = \
  SL_CLI_COMMAND_GROUP(real_time_table, "Real time related commands");

static sl_cli_command_entry_t main_table[] = {
  { "print_group", &cmd_group__print_table, false },
  { "system", &cmd_group__system_table, false },
  { "real_time", &cmd_group__real_time_table, false },

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

/***************************************************************************//**
 * Callback for set_datetime
 *
 * This function is used as a callback when the set_time command is called
 * in the cli.
 ******************************************************************************/
void set_datetime_cli_callback(sl_cli_command_arg_t *arguments){
  st_datetime_t time_argument;

  time_argument.date = sl_cli_get_argument_uint8(arguments, 0);
  time_argument.month = sl_cli_get_argument_uint8(arguments, 1);
  time_argument.year = sl_cli_get_argument_uint8(arguments, 2);
  time_argument.hours = sl_cli_get_argument_uint8(arguments, 3);
  time_argument.minutes = sl_cli_get_argument_uint8(arguments, 4);
  time_argument.seconds = sl_cli_get_argument_uint8(arguments, 5);
  time_argument.is_pm = false;
  time_argument.is_24hr_mode = true;
  time_argument.is_leap_year = true;
  time_argument.weekday = 1;

  if(!is_datetime_valid(time_argument)){
    printf("Invalid time\r\n");
    return;
  }

  set_datetime(time_argument);

  printf("Datetime set!");
}

/***************************************************************************//**
 * Callback for set_time
 *
 * This function is used as a callback when the set_time command is called
 * in the cli.
 ******************************************************************************/
void set_time_cli_callback(sl_cli_command_arg_t *arguments){
  st_datetime_t time_argument;

  time_argument.date = 0;
  time_argument.month = 0;
  time_argument.year = 0;
  time_argument.hours = sl_cli_get_argument_uint8(arguments, 0);
  time_argument.minutes = sl_cli_get_argument_uint8(arguments, 1);
  time_argument.seconds = sl_cli_get_argument_uint8(arguments, 2);
  time_argument.is_pm = false;
  time_argument.is_24hr_mode = true;
  time_argument.is_leap_year = true;
  time_argument.weekday = 1;

  if(!is_datetime_valid(time_argument)){
    printf("Invalid time\r\n");
    return;
  }

  set_time(time_argument);

  printf("Time set!");
}

/***************************************************************************//**
 * Callback for set_date
 *
 * This function is used as a callback when the set_date command is called
 * in the cli.
 ******************************************************************************/
void set_date_cli_callback(sl_cli_command_arg_t *arguments){
  st_datetime_t time_argument;

  time_argument.date = sl_cli_get_argument_uint8(arguments, 0);
  time_argument.month = sl_cli_get_argument_uint8(arguments, 1);
  time_argument.year = sl_cli_get_argument_uint8(arguments, 2);
  time_argument.hours = 0;
  time_argument.minutes = 0;
  time_argument.seconds = 0;
  time_argument.is_pm = false;
  time_argument.is_24hr_mode = true;
  time_argument.is_leap_year = true;
  time_argument.weekday = 1;

  if(!is_datetime_valid(time_argument)){
    printf("Invalid time\r\n");
    return;
  }

  set_date(time_argument);

  printf("Date set!");
}

/***************************************************************************//**
 * Callback for get_datetime
 *
 * This function is used as a callback when the get_datetime command is called
 * in the cli.
 ******************************************************************************/
void get_datetime_cli_callback(sl_cli_command_arg_t *arguments){
  st_datetime_t current_time;
  get_datetime(&current_time);

  printf("Current time - %02d:%02d:%02d\r\n"
         "Current date - %02d/%02d/20%02d\r\n",
         current_time.hours,
         current_time.minutes,
         current_time.seconds,
         current_time.date,
         current_time.month,
         current_time.year
         );
}

/***************************************************************************//**
 * Callback for get_time
 *
 * This function is used as a callback when the get_time command is called
 * in the cli.
 ******************************************************************************/
void get_time_cli_callback(sl_cli_command_arg_t *arguments){
  st_datetime_t current_time;
  get_datetime(&current_time);

  printf("Current time - %02d:%02d:%02d\r\n",
         current_time.hours,
         current_time.minutes,
         current_time.seconds
         );
}

/***************************************************************************//**
 * Callback for get_date
 *
 * This function is used as a callback when the get_date command is called
 * in the cli.
 ******************************************************************************/
void get_date_cli_callback(sl_cli_command_arg_t *arguments){
  st_datetime_t current_time;
  get_datetime(&current_time);

  printf("Current date - %02d/%02d/20%02d\r\n",
         current_time.date,
         current_time.month,
         current_time.year
         );
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
