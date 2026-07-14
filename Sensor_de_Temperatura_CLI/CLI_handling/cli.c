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
#include "../State_handling/system_state.h"
#include "../State_handling/nvm3_app.h"
#include "../NTC/NTC.h"
#include "../NTC/fire_alarm.h"
#include "../LED_handling/blink.h"
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
 * Command info for fire detection related commands
 ******************************************************************************/

static const sl_cli_command_info_t cmd__get_temperature = \
  SL_CLI_COMMAND(get_temperature_cli_callback,
                 "Returns the current temperature measured by the NTC Sensor",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__loop_temperature = \
  SL_CLI_COMMAND(loop_temperature_cli_callback,
                 "Returns the current temperature measured by the NTC Sensor in a loop",
                 "Enable: <0|1>",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__set_alarm = \
  SL_CLI_COMMAND(set_alarm_cli_callback,
                 "Set an alarm that is triggered based on the temperature, you set the temperature that trigger and the temperature that turns off the alarm",
                 "Dangerous temperature: temperature in Celsius that triggers the alarm"SL_CLI_UNIT_SEPARATOR "Safe temperature: temperature in Celsius that turns off the alarm",
                 { SL_CLI_ARG_INT32, SL_CLI_ARG_INT32, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__disarm_alarm = \
  SL_CLI_COMMAND(disarm_alarm_cli_callback,
                 "Disarm an alarm that was set before",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__show_average_temp_rates = \
  SL_CLI_COMMAND(show_average_temp_rates_cli_callback,
                 "Show the average temperature rates",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__temperature_ramp = \
  SL_CLI_COMMAND(temperature_ramp_cli_callback,
                 "Simulate a temperature ramp from initial temperature to final temperature during a specified period of time",
                 "Initial temperature: temperature in Celsius"SL_CLI_UNIT_SEPARATOR "Final temperature: temperature in Celsius"SL_CLI_UNIT_SEPARATOR "Duration: seconds",
                 { SL_CLI_ARG_INT32, SL_CLI_ARG_INT32, SL_CLI_ARG_UINT32, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__temperature_steps = \
  SL_CLI_COMMAND(temperature_steps_cli_callback,
                 "Simulate a series of temperature steps, you specify the duration of each step and what temperature is set in each step",
                 "Duration of the steps: seconds"SL_CLI_UNIT_SEPARATOR "Temperature in each step: temperature in Celsius",
                 { SL_CLI_ARG_UINT32, SL_CLI_ARG_INT32, SL_CLI_ARG_ADDITIONAL, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__disable_simulation = \
  SL_CLI_COMMAND(disable_simulation_cli_callback,
                 "Disable any simulation running",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__set_temperature = \
  SL_CLI_COMMAND(set_temperature_cli_callback,
                 "Set a simulated temperature",
                 "Temperature: temperature in Celsius",
                 { SL_CLI_ARG_INT32, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__simulated_temperature = \
  SL_CLI_COMMAND(simulated_temperature_cli_callback,
                 "Enable or disable simulated temperature",
                 "Enable: <0|1>",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__filtered_temperature = \
  SL_CLI_COMMAND(filtered_temperature_cli_callback,
                 "Enable or disable filtered temperature",
                 "Enable: <0|1>",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__set_detector_class = \
  SL_CLI_COMMAND(set_detector_class_cli_callback,
                 "Set the detector's class",
                 "Detector class: A1 - 0; A2 - 1; B - 3; ...",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__iso_test_simulation = \
  SL_CLI_COMMAND(iso_test_simulation_cli_callback,
                 "Simula o ensaio de Temperatura de resposta estática da norma ABNT NBR ISO 7240-5",
                 "Detector class: A1 - 0; A2 - 1; B - 3; ..."SL_CLI_UNIT_SEPARATOR "K/min: temperature rate (only those specified in the table)",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static sl_cli_command_entry_t fire_detection_table[] = {
  { "get_temperature", &cmd__get_temperature, false },
  { "loop_temperature", &cmd__loop_temperature, false },
  { "set_alarm", &cmd__set_alarm, false },
  { "disarm_alarm", &cmd__disarm_alarm, false },
  { "show_average_temp_rates", &cmd__show_average_temp_rates, false },
  { "temperature_ramp", &cmd__temperature_ramp, false },
  { "temperature_steps", &cmd__temperature_steps, false },
  { "disable_simulation", &cmd__disable_simulation, false },
  { "set_temperature", &cmd__set_temperature, false },
  { "simulated_temperature", &cmd__simulated_temperature, false },
  { "filtered_temperature", &cmd__filtered_temperature, false },
  { "set_detector_class", &cmd__set_detector_class, false },
  { "iso_test_simulation", &cmd__iso_test_simulation, false },

  { NULL, NULL, false },
};

static const sl_cli_command_info_t cmd_group__fire_detection_table = \
  SL_CLI_COMMAND_GROUP(fire_detection_table, "Fire detection related commands");

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
                 "save: save or no_save",
                 { SL_CLI_ARG_STRINGOPT, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__save = \
  SL_CLI_COMMAND(save_cli_callback,
                 "Save current state",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__load = \
  SL_CLI_COMMAND(load_cli_callback,
                 "Load previous state",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__erase = \
  SL_CLI_COMMAND(erase_cli_callback,
                 "Erase saved state from Flash",
                 "Nothing",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t system_table[] = {
  { "uptime", &cmd__uptime, false },
  { "reset", &cmd__reset, false },
  { "save", &cmd__save, false },
  { "load", &cmd__load, false },
  { "erase", &cmd__erase, false },

  { NULL, NULL, false },
};

static const sl_cli_command_info_t cmd_group__system_table = \
  SL_CLI_COMMAND_GROUP(system_table, "System related commands");

/***************************************************************************//**
 * Command info for input related commands
 ******************************************************************************/
#if SL_SIMPLE_BUTTON_COUNT > 0
static const sl_cli_command_info_t cmd__button = \
  SL_CLI_COMMAND(button_callback,
                 "Show selected button's status",
                 "button number: 0 or 1",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__wait_button = \
  SL_CLI_COMMAND(wait_button_callback,
                 "Wait until selected button is pressed",
                 "button number: 0 or 1",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });
#endif

static sl_cli_command_entry_t input_table[] = {
  #if SL_SIMPLE_BUTTON_COUNT > 0
  { "button", &cmd__button, false },
  { "wait_button", &cmd__wait_button, false },
  #endif

  { NULL, NULL, false },
};

static const sl_cli_command_info_t cmd_group__input_table = \
  SL_CLI_COMMAND_GROUP(input_table, "Input related commands");

/***************************************************************************//**
 * Command info for LED related commands
 ******************************************************************************/

#if SL_SIMPLE_LED_COUNT > 0
static const sl_cli_command_info_t cmd__set = \
  SL_CLI_COMMAND(set_led_cli_callback,
                 "Change the led status",
                 "instruction: on, off, or toggle",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__blink = \
  SL_CLI_COMMAND(blink_led_cli_callback,
                 "Makes the led blink at an specified delay",
                 "enable: start or stop"SL_CLI_UNIT_SEPARATOR "period: miliseconds up to 65535 ms",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_UINT16, SL_CLI_ARG_END, });
#endif // SL_SIMPLE_LED_COUNT

static sl_cli_command_entry_t led_table[] = {
#if SL_SIMPLE_LED_COUNT > 0
  { "set", &cmd__set, false },
  { "blink", &cmd__blink, false },
#endif

  { NULL, NULL, false },
};

static const sl_cli_command_info_t cmd_group__led_table = \
  SL_CLI_COMMAND_GROUP(led_table, "LED related commands");

static sl_cli_command_entry_t main_table[] = {
  { "print_group", &cmd_group__print_table, false },
  { "fire_detection", &cmd_group__fire_detection_table, false },
  { "system", &cmd_group__system_table, false },
  { "input", &cmd_group__input_table, false },
  { "led", &cmd_group__led_table, false },

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
  if(sl_cli_get_argument_count(arguments) > 0){
    char *argument = sl_cli_get_argument_string(arguments, 0);

    if(strcmp(argument, "save") == 0){
      save_state_to_flash();
    }
    else if(strcmp(argument, "no_save") == 0){
      printf("Current state not saved!\r\n");
    }
    else {
      printf("Invalid argument!\r\n");
      printf("Correct format: reset <save|no_save>\r\n");
      return;
    }
  }
  else printf("Current state not saved!\r\n");

  printf("Resetting system!\r\n");
  sl_sleeptimer_delay_millisecond(1000);
  NVIC_SystemReset();
}

/***************************************************************************//**
 * Callback definitions for button related commands
 ******************************************************************************/

#if SL_SIMPLE_BUTTON_COUNT > 0
/***************************************************************************//**
 * Callback for the button
 *
 * This function is used as a callback when the button command is called
 * in the cli. The command shows whether the selected button is pressed or not.
 ******************************************************************************/
void button_callback      (sl_cli_command_arg_t *arguments){
  if (sl_cli_get_argument_count(arguments) != 1) {
    printf("Correct format: button <0|1>\r\n");
    return;
  }

  uint8_t button_number = sl_cli_get_argument_uint8(arguments, 0);

  if(button_number >= SL_SIMPLE_BUTTON_COUNT){
    printf("Invalid button!\r\n");
    printf("Correct format: button <0|1>\r\n");
    return;
  }

  if(sl_simple_button_get_state(sl_simple_button_array[button_number])){
    printf("Button %u is pressed\r\n", button_number);
  }
  else{
    printf("Button %u is not pressed\r\n", button_number);
  }
}

/***************************************************************************//**
 * Callback for the wait_button
 *
 * This function is used as a callback when the wait_button command is called
 * in the cli. Makes the MCU busy-wait for the specified button to be pressed.
 ******************************************************************************/
void wait_button_callback (sl_cli_command_arg_t *arguments){
  if (sl_cli_get_argument_count(arguments) != 1) {
    printf("Correct format: wait_button <0|1>\r\n");
    return;
  }

  uint8_t button_number = sl_cli_get_argument_uint8(arguments, 0);

  if(button_number >= SL_SIMPLE_BUTTON_COUNT){
    printf("Invalid button!\r\n");
    printf("Correct format: wait_button <0|1>\r\n");
    return;
  }

  printf("Waiting for button %u to be pressed...\r\n", button_number);

  while(!sl_simple_button_get_state(sl_simple_button_array[button_number]));

  printf("Button %u was pressed!\r\n", button_number);
}
#endif // if SL_SIMPLE_BUTTON_COUNT > 0

/***************************************************************************//**
 * Callback definitions for LED related commands
 ******************************************************************************/

#if SL_SIMPLE_LED_COUNT > 0
/***************************************************************************//**
 * Callback for the led
 *
 * This function is used as a callback when the led command is called
 * in the cli. The command is used to turn on, turn off and toggle leds.
 ******************************************************************************/
void set_led_cli_callback(sl_cli_command_arg_t *arguments)
{
  char *instruction;

  // Get the instruction provided
  instruction = sl_cli_get_argument_string(arguments, 0);

  if (strcmp(instruction, "on") == 0) {
    stop_blink();

    sl_led_sinalizacao.turn_on(sl_led_sinalizacao.context);
    set_led_state(LED_SET, 1);

    printf("Led turned on!\r\n");

  } else if (strcmp(instruction, "off") == 0) {
    stop_blink();

    sl_led_sinalizacao.turn_off(sl_led_sinalizacao.context);
    set_led_state(LED_SET, 0);

    printf("Led turned off!\r\n");

  } else if (strcmp(instruction, "toggle") == 0) {
    stop_blink();

    sl_led_sinalizacao.toggle(sl_led_sinalizacao.context);

    printf("Led toggled!\r\n");

  } else {
    // led off instruction provided
    printf("Correct format: led set <on|off|toggle>\r\n");
    return;
  }
}

/***************************************************************************//**
 * Callback for the blink_led
 *
 * This function is used as a callback when the blink_led command is called
 * in the cli. The command is used to blink leds at a specified delay in ms.
 ******************************************************************************/
void blink_led_cli_callback(sl_cli_command_arg_t *arguments)
{
  char *enable = sl_cli_get_argument_string(arguments, 0);

  if(strcmp(enable, "start") == 0){
    uint16_t period = sl_cli_get_argument_uint16(arguments, 1);

    start_blink(period);

    printf("Led blinking!\r\n");

  }
  else if(strcmp(enable, "stop") == 0){
    stop_blink();

    printf("Led stopped blinking!\r\n");

  }
  else{
    printf("Correct format: led blink <start|stop> <period : ms>\r\n");
    return;
  }
}
#endif

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

  NTC_init();

  system_state_init();

  printf("\r\nStarted CLI Bare-metal\r\n\r\n");
}

/***************************************************************************//**
 * Ticking function
 ******************************************************************************/
void cli_app_process_action(void)
{
}
