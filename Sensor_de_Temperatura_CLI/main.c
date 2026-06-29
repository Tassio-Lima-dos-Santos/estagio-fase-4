/***************************************************************************//**
 * @file main.c
 * @brief main() function.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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


/******************************************************************************
 * Includes
 *****************************************************************************/

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif
#include "sl_system_init.h"
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif
#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "sl_system_kernel.h"
#else
#include "sl_system_process_action.h"
#endif // SL_CATALOG_KERNEL_PRESENT

#ifdef EMBER_TEST
#define main nodeMain
#endif

#include "em_gpio.h"
#include "sl_sleeptimer.h"
#include <stdio.h>
#include "NTC/IADC.h"
#include "NTC/NTC.h"
#include "zigbee_app_framework_event.h"
#include "CLI_handling/cli.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

#define IADC_READ_PERIOD 1000 // 1000 ms

//#define USE_SLEEPTIMER
#define USE_ZIGBEE_EVENT

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/

#ifdef USE_SLEEPTIMER
static sl_sleeptimer_timer_handle_t timer_temperature_polling;
#elif defined(USE_ZIGBEE_EVENT)
static sl_zigbee_event_t temperature_polling_event;
#endif // defined(USE_ZIGBEE_EVENT)

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

#ifdef USE_SLEEPTIMER
static void timer_temperature_polling_callback(sl_sleeptimer_timer_handle_t *handle, void *data);
#elif defined(USE_ZIGBEE_EVENT)
static void temperature_polling_event_handler(sl_zigbee_event_t *event);
#endif // defined(USE_ZIGBEE_EVENT)

/*******************************************************************************
 **************************   PRIVATE FUNCTIONS   *******************************
 ******************************************************************************/

#ifdef USE_SLEEPTIMER
static void timer_temperature_polling_callback(sl_sleeptimer_timer_handle_t *handle, void *data){
  // GPIO_PinOutToggle(gpioPortD, 1);

  uint32_t raw_iadc = IADC_read_raw();
  printf("Raw IADC: %lu\r\n", raw_iadc);

  double milivolts = IADC_read_milivolts();
  printf("Milivolts: %lf\r\n", milivolts);

  double NTC_resistance = NTC_milivoltage_to_resistance(milivolts);
  printf("NTC Resistance in Ohms: %lf\r\n", NTC_resistance);

  double temperature = NTC_resistance_to_temperature(NTC_resistance);
  printf("Temperature in Celsius: %lf C\r\n", temperature);

  printf("\r\n");
}
#elif defined(USE_ZIGBEE_EVENT)
static void temperature_polling_event_handler(sl_zigbee_event_t *event){
  uint32_t raw_iadc = IADC_read_raw();
  printf("Raw IADC: %lu\r\n", raw_iadc);

  double milivolts = IADC_read_milivolts();
  printf("Milivolts: %lf\r\n", milivolts);

  double NTC_resistance = NTC_milivoltage_to_resistance(milivolts);
  printf("NTC Resistance in Ohms: %lf\r\n", NTC_resistance);

  double temperature = NTC_resistance_to_temperature(NTC_resistance);
  printf("Temperature in Celsius: %lf C\r\n", temperature);

  printf("\r\n");

  sl_zigbee_event_set_delay_ms(event, IADC_READ_PERIOD);
}
#endif // defined(USE_ZIGBEE_EVENT)

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

void app_init(void)
{
  cli_app_init();


#ifdef USE_SLEEPTIMER
  sl_sleeptimer_start_periodic_timer_ms(
      &timer_temperature_polling,
      IADC_READ_PERIOD,
      timer_temperature_polling_callback,
      NULL,
      0,
      SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG
    );
#elif defined(USE_ZIGBEE_EVENT)
  sl_zigbee_event_init(&temperature_polling_event, temperature_polling_event_handler);

  sl_zigbee_event_set_delay_ms(&temperature_polling_event, IADC_READ_PERIOD);
#endif // defined(USE_ZIGBEE_EVENT)
}

void app_process_action(void)
{

}

#if defined(__ICCARM__)
#pragma diag_suppress=Pe111
#endif // defined(__ICCARM__)
int main(void)
{
  // Initialize Silicon Labs device, system, service(s) and protocol stack(s).
  // Note that if the kernel is present, processing task(s) will be created by
  // this call.
  sl_system_init();

  // Initialize the application. For example, create periodic timer(s) or
  // task(s) if the kernel is present.
  app_init();

#if defined(SL_CATALOG_KERNEL_PRESENT)
  // Start the kernel. Task(s) created in app_init() will start running.
  sl_system_kernel_start();
#else // SL_CATALOG_KERNEL_PRESENT
  while (1) {
    // Do not remove this call: Silicon Labs components process action routine
    // must be called from the super loop.
    sl_system_process_action();

    // Application process.
    app_process_action();

    // Let the CPU go to sleep if the system allow it.
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
    sl_power_manager_sleep();
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
  }
#endif // SL_CATALOG_KERNEL_PRESENT

  return 0;
}
