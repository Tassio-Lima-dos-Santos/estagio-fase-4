/******************************************************************************
 * File mcp79410_memory_map.h
 *
 *  Created on: 17 de jul. de 2026
 *      Author: Tassio Lima dos Santos
 *      Email: desenvolvimento20@globalsonic.com.br
 *****************************************************************************/


/******************************************************************************
 * Description
 *
 * Usage        :
 * Known Errors :
 * ToDo         :
 *****************************************************************************/

/******************************************************************************
 * Multiple include protection
 *****************************************************************************/
#ifndef MCP79410_STACK_MCP79410_MEMORY_MAP_H_
#define MCP79410_STACK_MCP79410_MEMORY_MAP_H_




/*******************************************************************************
 * RTCC Registers/SRAM Memory Map
 ******************************************************************************/

// Timekeeping
#define MCP79410_RTCC_REGISTER_RTCSEC_ADDRESS               0x00
#define MCP79410_RTCC_REGISTER_RTCMIN_ADDRESS               0x01
#define MCP79410_RTCC_REGISTER_RTCHOUR_ADDRESS              0x02
#define MCP79410_RTCC_REGISTER_RTCWKDAY_ADDRESS             0x03
#define MCP79410_RTCC_REGISTER_RTCDATE_ADDRESS              0x04
#define MCP79410_RTCC_REGISTER_RTCMTH_ADDRESS               0x05
#define MCP79410_RTCC_REGISTER_RTCYEAR_ADDRESS              0x06
#define MCP79410_RTCC_REGISTER_CONTROL_ADDRESS              0x07
#define MCP79410_RTCC_REGISTER_OSCTRIM_ADDRESS              0x08
#define MCP79410_RTCC_REGISTER_EEUNLOCK_ADDRESS             0x09

// Alarm 0
#define MCP79410_RTCC_REGISTER_ALM0SEC_ADDRESS              0x0a
#define MCP79410_RTCC_REGISTER_ALM0MIN_ADDRESS              0x0b
#define MCP79410_RTCC_REGISTER_ALM0HOUR_ADDRESS             0x0c
#define MCP79410_RTCC_REGISTER_ALM0WKDAY_ADDRESS            0x0d
#define MCP79410_RTCC_REGISTER_ALM0DATE_ADDRESS             0x0e
#define MCP79410_RTCC_REGISTER_ALM0MTH_ADDRESS              0x0f

// Alarm 1
#define MCP79410_RTCC_REGISTER_ALM1SEC_ADDRESS              0x11
#define MCP79410_RTCC_REGISTER_ALM1MIN_ADDRESS              0x12
#define MCP79410_RTCC_REGISTER_ALM1HOUR_ADDRESS             0x13
#define MCP79410_RTCC_REGISTER_ALM1WKDAY_ADDRESS            0x14
#define MCP79410_RTCC_REGISTER_ALM1DATE_ADDRESS             0x15
#define MCP79410_RTCC_REGISTER_ALM1MTH_ADDRESS              0x16

// Power-Down Time Stamp
#define MCP79410_RTCC_REGISTER_PWRDNMIN_ADDRESS             0x18
#define MCP79410_RTCC_REGISTER_PWRDNHOUR_ADDRESS            0x19
#define MCP79410_RTCC_REGISTER_PWRDNDATE_ADDRESS            0x1a
#define MCP79410_RTCC_REGISTER_PWRDNMTH_ADDRESS             0x1b

// Power-Up Time Stamp
#define MCP79410_RTCC_REGISTER_PWRUPMIN_ADDRESS             0x1c
#define MCP79410_RTCC_REGISTER_PWRUPHOUR_ADDRESS            0x1d
#define MCP79410_RTCC_REGISTER_PWRUPDATE_ADDRESS            0x1e
#define MCP79410_RTCC_REGISTER_PWRUPMTH_ADDRESS             0x1f

// RTCC Registers Address Limits
#define MCP79410_RTCC_REGISTERS_BEGIN_ADDRESS               MCP79410_RTCC_REGISTER_RTCSEC_ADDRESS
#define MCP79410_RTCC_REGISTERS_END_ADDRESS                 (MCP79410_RTCC_REGISTER_PWRUPMTH_ADDRESS + 1)

// SRAM Address Limits
#define MCP79410_SRAM_BEGIN_ADDRESS                         0x20
#define MCP79410_SRAM_END_ADDRESS                           0x60

/*******************************************************************************
 * EEPROM Memory Map
 ******************************************************************************/

// EEPROM Address Limits
#define MCP79410_EEPROM_BEGIN_ADDRESS                       0x00
#define MCP79410_EEPROM_END_ADDRESS                         0x80

// Protected EEPROM Address Limits
#define MCP79410_PROTECTED_EEPROM_BEGIN_ADDRESS             0xF0
#define MCP79410_PROTECTED_EEPROM_END_ADDRESS               0xF8

// STATUS Register Address
#define MCP79410_STATUS_REGISTER_ADDRESS                    0xFF


/*******************************************************************************
 * RTCC Registers Bit Map
 ******************************************************************************/

enum enum_sec_offset{
  SECONE0_OFFSET = 0,
  SECONE1_OFFSET = 1,
  SECONE2_OFFSET = 2,
  SECONE3_OFFSET = 3,
  SECTEN0_OFFSET = 4,
  SECTEN1_OFFSET = 5,
  SECTEN2_OFFSET = 6,
  ST_OFFSET = 7, // Must be set to enable the crystal oscillator circuit
};

enum enum_min_offset{
  MINONE0_OFFSET = 0,
  MINONE1_OFFSET = 1,
  MINONE2_OFFSET = 2,
  MINONE3_OFFSET = 3,
  MINTEN0_OFFSET = 4,
  MINTEN1_OFFSET = 5,
  MINTEN2_OFFSET = 6,
};

enum enum_hour_offset{
  HRONE0_OFFSET = 0,
  HRONE1_OFFSET = 1,
  HRONE2_OFFSET = 2,
  HRONE3_OFFSET = 3,
  HRTEN0_OFFSET = 4,
  HRTEN1_OFFSET = 5,
  BIT_12_24_OFFSET = 6,
};

enum enum_rtcwkday_offset{
  RTCWKDAY0_OFFSET = 0,
  RTCWKDAY1_OFFSET = 1,
  RTCWKDAY2_OFFSET = 2,
  VBATEN_OFFSET = 3,
  PWRFAIL_OFFSET = 4,
  OSCRUN_OFFSET = 5, // Indicates whether the oscillator is running
};

enum enum_date_offset{
  DATEONE0_OFFSET = 0,
  DATEONE1_OFFSET = 1,
  DATEONE2_OFFSET = 2,
  DATEONE3_OFFSET = 3,
  DATETEN0_OFFSET = 4,
  DATETEN1_OFFSET = 5,
};

enum enum_month_offset{
  MTHONE0_OFFSET = 0,
  MTHONE1_OFFSET = 1,
  MTHONE2_OFFSET = 2,
  MTHONE3_OFFSET = 3,
  MTHTEN0_OFFSET = 4,
  LPYR_WKDAY0_OFFSET = 5,
  MTH_WKDAY1_OFFSET = 6,
  MTH_WKDAY2_OFFSET = 7,
};

enum enum_year_offset{
  YRONE0_OFFSET = 0,
  YRONE1_OFFSET = 1,
  YRONE2_OFFSET = 2,
  YRONE3_OFFSET = 3,
  YRTEN0_OFFSET = 4,
  YRTEN1_OFFSET = 5,
  YRTEN2_OFFSET = 6,
  YRTEN3_OFFSET = 7,
};

enum enum_control_offset{
  SQWFS0_OFFSET = 0,
  SQWFS1_OFFSET = 1,
  CRSTRIM_OFFSET = 2,
  EXTOSC_OFFSET = 3, // Must be set to enable an external clock source
  ALM0EN_OFFSET = 4,
  ALM1EN_OFFSET = 5,
  SQWEN_OFFSET = 6,
  OUT_OFFSET = 7,
};

enum enum_osctrim_offset{
  TRIMVAL0_OFFSET = 0,
  TRIMVAL1_OFFSET = 1,
  TRIMVAL2_OFFSET = 2,
  TRIMVAL3_OFFSET = 3,
  TRIMVAL4_OFFSET = 4,
  TRIMVAL5_OFFSET = 5,
  TRIMVAL6_OFFSET = 6,
  SIGN_OFFSET = 7,
};

#endif /* MCP79410_STACK_MCP79410_MEMORY_MAP_H_ */
