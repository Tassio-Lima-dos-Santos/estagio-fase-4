/******************************************************************************
 * File IADC.c
 *
 *  Created on: 19 de jun. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_iadc.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

// Frequências de clock
#define CLK_SRC_ADC_FREQ    1000000   // CLK_SRC_ADC: 1 MHz - 20 MHz na stack
#define CLK_ADC_FREQ        10000     // CLK_ADC: 10 kHz    - 10 MHz na stack

// Configuração do pino de entrada (ex: PB01)
#define IADC_ENABLE_PORT     gpioPortA
#define IADC_ENABLE_PIN      4

#define IADC_INPUT_PORT_PIN  iadcPosInputPortCPin3
#define IADC_INPUT_BUS       CDBUSALLOC
#define IADC_INPUT_BUSALLOC  GPIO_CDBUSALLOC_CDODD0_ADC0

// Parâmetros do IADC
#define VREF 2400      // 2400 mV
#define ANALOG_GAIN 1  // 1x

/******************************************************************************
 * Data types
 *****************************************************************************/

/******************************************************************************
 * Static Variables
 *****************************************************************************/

/******************************************************************************
 * Extern
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

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
void IADC_NTC_init(void){
  // Declarar estruturas de inicialização
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_DEFAULT;

  // Resetar IADC para garantir configuração limpa
  IADC_reset(IADC0);

  // Seleciona o clock do IADC (FSRCO = 20 MHz)
  CMU_ClockEnable(cmuClock_IADC0, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);

  // Configurar prescaler do clock fonte
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  // Usar referência interna de 1.2V
  initAllConfigs.configs[0].reference = iadcCfgReferenceVddX0P8Buf;
  initAllConfigs.configs[0].vRef = IADC_getReferenceVoltage(iadcCfgReferenceVddX0P8Buf);
  initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain1x;

  // Configurar prescaler do clock ADC
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                             CLK_ADC_FREQ,
                                             0,
                                             iadcCfgModeNormal,
                                             init.srcClkPrescale);

  // Configurar pinos de entrada (ajuste conforme seu hardware)
  initSingleInput.posInput = IADC_INPUT_PORT_PIN;
  initSingleInput.negInput = iadcNegInputGnd; // GND para single-ended

  // Inicializar IADC e canal single
  IADC_init(IADC0, &init, &initAllConfigs);
  IADC_initSingle(IADC0, &initSingle, &initSingleInput);

  GPIO_PinModeSet(IADC_ENABLE_PORT, IADC_ENABLE_PIN, gpioModePushPull, 0);

  // Alocar barramento analógico para o pino de entrada
  GPIO->IADC_INPUT_BUS |= IADC_INPUT_BUSALLOC;
}

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
uint32_t IADC_read_raw(void){
  IADC_Result_t sample;
  uint32_t adc_value;

  GPIO_PinOutSet(IADC_ENABLE_PORT, IADC_ENABLE_PIN);

  // Inicia a conversão
  IADC_command(IADC0, iadcCmdStartSingle);

  // Aguarda a conversão completar (polling)
  // Bits 8 (CONVERTING) e 6 (SINGLEFIFODV) do STATUS
  while ((IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK
                         | _IADC_STATUS_SINGLEFIFODV_MASK))
         != IADC_STATUS_SINGLEFIFODV);

  // Lê o resultado do FIFO
  sample    = IADC_pullSingleFifoResult(IADC0);
  adc_value = sample.data;

  GPIO_PinOutClear(IADC_ENABLE_PORT, IADC_ENABLE_PIN);

  return (adc_value);
}

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
double IADC_convert_raw_to_milivolts(uint32_t raw){
  double mV = (raw * VREF / ANALOG_GAIN) / 0xFFF;
  return (mV);
}

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
double IADC_read_milivolts(void){
  uint32_t raw = IADC_read_raw();

  return (IADC_convert_raw_to_milivolts(raw));
}









