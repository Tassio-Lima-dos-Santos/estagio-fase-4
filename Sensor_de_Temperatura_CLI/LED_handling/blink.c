/******************************************************************************
 * File blink.c
 *
 *  Created on: 12 de jun. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include <stdint.h>
#include "blink.h"
#include "../State_handling/LED_state.h"
#include "sl_sleeptimer.h"
#include "zigbee_app_framework_event.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

// #define USE_SLEEPTIMER
#define USE_ZIGBEE_EVENT

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/

#ifdef USE_SLEEPTIMER
static sl_sleeptimer_timer_handle_t timer_blink;
#elif defined(USE_ZIGBEE_EVENT)
static sl_zigbee_event_t blink_event;
static volatile uint16_t global_blink_period;
#endif // defined(USE_ZIGBEE_EVENT)

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

#ifdef USE_SLEEPTIMER
void on_timeout_blink(sl_sleeptimer_timer_handle_t *handle,
                       void *data);
#elif defined(USE_ZIGBEE_EVENT)
static void blink_event_handler(sl_zigbee_event_t *event);
#endif // defined(USE_ZIGBEE_EVENT)

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/*******************************************************************************
 * Function name: start_blink
 *
 * Description  : Start the blinking functionality of the specified LED at a
 *                specified period of time
 * Parameters   : uint8_t led_number
 *                uint16_t period - period of time between each breath
 * Returns      : uint8_t - 0 for success, 1 for failure
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
uint8_t start_blink(uint16_t period){
#ifdef USE_SLEEPTIMER
  // Caso o timer já tiver sido iniciado
  if(timer_blink.callback == on_timeout_blink) stop_blink();

  uint32_t period_tick;
  sl_sleeptimer_ms32_to_tick(period, &period_tick);

  int32_t delay_tick = period_tick / 2;

  if(delay_tick == 0) delay_tick = 1;

  set_led_state(LED_BLINK, period);

  sl_sleeptimer_start_periodic_timer(&timer_blink,
                                     delay_tick,
                                              on_timeout_blink,
                                              NULL,
                                              0,
                                              SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
#elif defined(USE_ZIGBEE_EVENT)
  sl_zigbee_event_init(&blink_event, blink_event_handler);
  // Caso o timer já tiver sido iniciado
  if(sl_zigbee_event_is_scheduled(&blink_event)) stop_blink();

  set_led_state(LED_BLINK, period);

  global_blink_period = period;

  sl_zigbee_event_set_delay_ms(&blink_event, global_blink_period);
#endif // defined(USE_ZIGBEE_EVENT)
  return (0);
}

/*******************************************************************************
 * Function name: stop_blink
 *
 * Description  : Stop the blinking functionality of the specified LED
 * Parameters   : uint8_t led_number
 * Returns      : uint8_t - 0 for success, 1 for failure
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
uint8_t stop_blink(void){
#ifdef USE_SLEEPTIMER
  sl_sleeptimer_stop_timer(&timer_blink);
#elif defined(USE_ZIGBEE_EVENT)
  sl_zigbee_event_set_inactive(&blink_event);
#endif // defined(USE_ZIGBEE_EVENT)

  sl_led_sinalizacao.turn_off(sl_led_sinalizacao.context);
  set_led_state(LED_IDLE, 0);

  return (0);
}

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

#ifdef USE_SLEEPTIMER
/*******************************************************************************
 * Function name: on_timeout_blink
 *
 * Description  : Callback for toggling the LED
 * Parameters   : sl_sleeptimer_timer_handle_t *handle
 *                void *data - Saves which LED is toggling
 * Returns      : void
 *
 * Known issues :
 * Note         :
 ******************************************************************************/
void on_timeout_blink(sl_sleeptimer_timer_handle_t *handle,
                       void *data)
{
  (void) handle;

  (void) data;

  sl_led_sinalizacao.toggle(sl_led_sinalizacao.context);
}
#elif defined(USE_ZIGBEE_EVENT)
static void blink_event_handler(sl_zigbee_event_t *event){
  sl_led_sinalizacao.toggle(sl_led_sinalizacao.context);

  sl_zigbee_event_set_delay_ms(event, global_blink_period);
}
#endif // defined(USE_ZIGBEE_EVENT)







