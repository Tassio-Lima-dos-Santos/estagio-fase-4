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

#define CHECK_BIT(byte, bit) (((byte) >> (bit)) & 1U)

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

void hello_cli_callback            (sl_cli_command_arg_t *arguments);

void uptime_cli_callback        (sl_cli_command_arg_t *arguments);
void reset_cli_callback        (sl_cli_command_arg_t *arguments);

void set_datetime_cli_callback(sl_cli_command_arg_t *arguments);
void set_time_cli_callback(sl_cli_command_arg_t *arguments);
void set_date_cli_callback(sl_cli_command_arg_t *arguments);
void set_trimming_cli_callback(sl_cli_command_arg_t *arguments);
void get_datetime_cli_callback(sl_cli_command_arg_t *arguments);
void get_time_cli_callback(sl_cli_command_arg_t *arguments);
void get_date_cli_callback(sl_cli_command_arg_t *arguments);
void get_trimming_cli_callback(sl_cli_command_arg_t *arguments);
void get_all_registers_cli_callback(sl_cli_command_arg_t *arguments);

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

static const sl_cli_command_info_t cmd__real_time_set_trimming = \
  SL_CLI_COMMAND(set_trimming_cli_callback,
                 "Set the trimming in the RTC",
                 "Sign (add is 1, subtraction is 0)"SL_CLI_UNIT_SEPARATOR"Enable coarse trim"SL_CLI_UNIT_SEPARATOR"Pulses",
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

static const sl_cli_command_info_t cmd__real_time_get_trimming = \
  SL_CLI_COMMAND(get_trimming_cli_callback,
                 "Gets trimming info from RTC",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__real_time_get_all_registers = \
  SL_CLI_COMMAND(get_all_registers_cli_callback,
                 "Gets trimming info from RTC",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t real_time_table[] = {
  { "set_datetime", &cmd__real_time_set_datetime, false },
  { "set_time", &cmd__real_time_set_time, false },
  { "set_date", &cmd__real_time_set_date, false },
  { "set_trimming", &cmd__real_time_set_trimming, false },
  { "get_datetime", &cmd__real_time_get_datetime, false },
  { "get_time", &cmd__real_time_get_time, false },
  { "get_date", &cmd__real_time_get_date, false },
  { "get_trimming", &cmd__real_time_get_trimming, false },
  { "get_all_registers", &cmd__real_time_get_all_registers, false },

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
 * Callback for set_trimming
 *
 * This function is used as a callback when the set_date command is called
 * in the cli.
 ******************************************************************************/
void set_trimming_cli_callback(sl_cli_command_arg_t *arguments)
{
  bool sign = sl_cli_get_argument_uint8(arguments, 0);
  bool coarse_trim = sl_cli_get_argument_uint8(arguments, 1);
  uint8_t pulses = sl_cli_get_argument_uint8(arguments, 2);

  set_trimming(sign, coarse_trim, pulses);

  printf("Trimming set!\r\n");
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

/***************************************************************************//**
 * Callback for get_date
 *
 * This function is used as a callback when the get_date command is called
 * in the cli.
 ******************************************************************************/
void get_trimming_cli_callback(sl_cli_command_arg_t *arguments){
  uint8_t osctrim_reg = get_trimming();

  bool is_add = CHECK_BIT(osctrim_reg, 7);
  uint8_t pulses = osctrim_reg << 1;

  if(is_add) printf("Trim adds ");
  else printf("Trim subtracts ");
  printf("%u clock cycles\r\n", pulses);
}

void get_all_registers_cli_callback(sl_cli_command_arg_t *arguments)
{
  uint8_t reg_array[64];
  get_all_register(reg_array);

  printf("\r\nRTCSEC    : %08b", reg_array[0]);
  printf("\r\nRTCMIN    : %08b", reg_array[1]);
  printf("\r\nRTCHOUR   : %08b", reg_array[2]);
  printf("\r\nRTCWKDAY  : %08b", reg_array[3]);
  printf("\r\nRTCDATE   : %08b", reg_array[4]);
  printf("\r\nRTCMTH    : %08b", reg_array[5]);
  printf("\r\nRTCYEAR   : %08b", reg_array[6]);
  printf("\r\nCONTROL   : %08b", reg_array[7]);
  printf("\r\nOSCTRIM   : %08b", reg_array[8]);
  printf("\r\nEEUNLOCK  : %08b", reg_array[9]);

  printf("\r\nALM0SEC   : %08b", reg_array[10]);
  printf("\r\nALM0MIN   : %08b", reg_array[11]);
  printf("\r\nALM0HOUR  : %08b", reg_array[12]);
  printf("\r\nALM0WKDAY : %08b", reg_array[13]);
  printf("\r\nALM0DATE  : %08b", reg_array[14]);
  printf("\r\nALM0MTH   : %08b", reg_array[15]);

  printf("\r\nALM1SEC   : %08b", reg_array[17]);
  printf("\r\nALM1MIN   : %08b", reg_array[18]);
  printf("\r\nALM1HOUR  : %08b", reg_array[19]);
  printf("\r\nALM1WKDAY : %08b", reg_array[20]);
  printf("\r\nALM1DATE  : %08b", reg_array[21]);
  printf("\r\nALM1MTH   : %08b", reg_array[22]);

  printf("\r\nPWRDNMIN  : %08b", reg_array[24]);
  printf("\r\nPWRDNHOUR : %08b", reg_array[25]);
  printf("\r\nPWRDNDATE : %08b", reg_array[26]);
  printf("\r\nPWRDNMTH  : %08b", reg_array[27]);

  printf("\r\nPWRUPMIN  : %08b", reg_array[28]);
  printf("\r\nPWRUPHOUR : %08b", reg_array[29]);
  printf("\r\nPWRUPDATE : %08b", reg_array[30]);
  printf("\r\nPWRUPMTH  : %08b", reg_array[31]);

  printf("\r\n");
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
