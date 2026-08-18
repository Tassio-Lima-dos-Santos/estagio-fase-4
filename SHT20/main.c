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

#include "zigbee_app_framework_event.h"
#include "em_gpio.h"
#include "hal_sht20/hal_sht20.h"
#include <stdio.h>

#define TEMPERATURE_POLLING_PERIOD_MS 1000

static sl_zigbee_event_t temperature_polling_event;

static void temperature_polling_event_handler(sl_zigbee_event_t *event);

void app_init(void)
{
  GPIO_PinModeSet(gpioPortB, 0, gpioModePushPull, 1);
  SHT20_init(gpioPortC, 2, gpioPortC, 3);
  sl_zigbee_event_init(&temperature_polling_event, temperature_polling_event_handler);
  sl_zigbee_event_set_delay_ms(&temperature_polling_event, TEMPERATURE_POLLING_PERIOD_MS);
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

static void temperature_polling_event_handler(sl_zigbee_event_t *event){
  float temperature = 0;
  float humidity = 0;

  hal_sht20_return_t status = measure_temperature(&temperature);
  status = measure_humidity(&humidity);

  printf("\r\n");
  printf("Relative Humidity = %.2f%%\r\n", humidity);
  printf("Temperature = %.2f C\r\n", temperature);

  sl_zigbee_event_set_delay_ms(event, TEMPERATURE_POLLING_PERIOD_MS);
}



