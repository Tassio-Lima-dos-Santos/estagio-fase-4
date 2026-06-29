/******************************************************************************
 * File NTC.c
 *
 *  Created on: 19 de jun. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/

#include "NTC.h"
#include "IADC.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

#define GS_NTC_TEMP {\
    {-30,127500},{-25,97100},{-20,74800},{-15,58080},{-10,45440},{-5,35820},\
    {0,28460},{5,22780},{10,18360},{15,14900},{20,12170},{25,10000},{30,8264},\
    {35,6890},{40,5738},{45,4810},{50,4064},{55,3448},{60,2934},\
    {65,2504},{70,2146},{75,1845},{80,1592},{85,1378},{90,1195},\
    {95,1039}, {100,966}, {105,795}, {110,703}\
}


#define VDD 3000 // 3000 mV
#define R_FIXO 10000 // 10 k ohm

/******************************************************************************
 * Data types
 *****************************************************************************/

typedef struct {
    double iTempCelsius;
    double dRntc;
} st_ntc_temp_t;

/******************************************************************************
 * Static Variables
 *****************************************************************************/

static const st_ntc_temp_t stNtcTempTable[] = GS_NTC_TEMP;

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static float map(float value, float in_min, float in_max, float out_min, float out_max);

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
void NTC_init(void){
  IADC_NTC_init();
}

double NTC_milivoltage_to_resistance(double milivolts_NTC){
  // R_NTC = (V_NTC / (VDD - V_NTC) ) * R_FIXO

  double NTC_resistance = (milivolts_NTC / (VDD - milivolts_NTC) ) * R_FIXO;

  return (NTC_resistance);
}

double NTC_resistance_to_temperature(double resistance_NTC){
  double reference_resistance_below = 0;
  double reference_temperature_below = 0;

  double reference_resistance_above = 0;
  double reference_temperature_above = 0;

  uint8_t LUT_length = sizeof(stNtcTempTable)/sizeof(stNtcTempTable[0]);
  for(int i = 1; i < LUT_length; i++){
    reference_resistance_above = stNtcTempTable[i - 1].dRntc;
    reference_resistance_below = stNtcTempTable[i].dRntc;

    if(reference_resistance_below < resistance_NTC && reference_resistance_above > resistance_NTC){
      reference_temperature_above = stNtcTempTable[i - 1].iTempCelsius;
      reference_temperature_below = stNtcTempTable[i].iTempCelsius;
      break;
    }
  }

  if(reference_temperature_below == 0) return (-2000);

  double temperature = map(resistance_NTC, reference_resistance_below, reference_resistance_above, reference_temperature_below, reference_temperature_above);
  return (temperature);
}

double NTC_milivoltage_to_temperature(double milivolts){
  double resistance = NTC_milivoltage_to_resistance(milivolts);
  double temperature = NTC_resistance_to_temperature(resistance);

  return (temperature);
}

double NTC_read_temperature(void){
  double milivolts = IADC_read_milivolts();
  double temperature = NTC_milivoltage_to_temperature(milivolts);

  return (temperature);
}

static float map(float value, float in_min, float in_max, float out_min, float out_max) {
  float result = (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  return (result);
}
