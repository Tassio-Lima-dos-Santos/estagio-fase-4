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

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

void hello_cli_callback            (sl_cli_command_arg_t *arguments);

void uptime_cli_callback        (sl_cli_command_arg_t *arguments);
void reset_cli_callback        (sl_cli_command_arg_t *arguments);

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

static sl_cli_command_entry_t main_table[] = {
  { "print_group", &cmd_group__print_table, false },
  { "system", &cmd_group__system_table, false },

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
