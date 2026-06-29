/***************************************************************************//**
 * @file
 * @brief NVM3 examples functions
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
#include <stdio.h>
#include <string.h>

#include "nvm3_app.h"
#include "nvm3_default.h"
#include "nvm3_default_config.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
// Max and min keys for data objects
#define DATA_KEY  NVM3_KEY_MIN

// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE nvm3_defaultHandle

/*******************************************************************************
 **************************   LOCAL VARIABLES   ********************************
 ******************************************************************************/

// Buffer for reading from NVM3
// static char buffer[NVM3_DEFAULT_MAX_OBJECT_SIZE];

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Write data to NVM3
 *
 * This function implements the CLI command 'write' (see nvm3_app.slcp)
 * It stores string data at the selected NVM3 key.
 ******************************************************************************/
void save_state_to_flash(void)
{

  if (ECODE_NVM3_OK == nvm3_writeData(NVM3_DEFAULT_HANDLE,
                                      DATA_KEY,
                                      (unsigned char *)&state_variables,
                                      sizeof(state_variables) )) {
    printf("Current state saved!\r\n");
  } else {
    printf("Error saving state\r\n");
  }
}

/***************************************************************************//**
 * Delete data in NVM3.
 *
 * This function implements the CLI command 'delete' (see nvm3_app.slcp)
 * It deletes the data object stored at the selected NVM3 key.
 ******************************************************************************/
void erase_state_in_flash(void)
{

  if (ECODE_NVM3_OK == nvm3_deleteObject(NVM3_DEFAULT_HANDLE, DATA_KEY)) {
    printf("Saved state erased!\r\n");
  } else {
    printf("Error erasing data\r\n");
  }
}

/***************************************************************************//**
 * Read data from NVM3.
 *
 * This function implements the CLI command 'read' (see nvm3_app.slcp)
 * It reads the data object stored at the selected NVM3 key.
 ******************************************************************************/
void load_memory_state_from_flash(void)
{
  uint32_t type;
  size_t len;
  Ecode_t err;

  err = nvm3_getObjectInfo(NVM3_DEFAULT_HANDLE, DATA_KEY, &type, &len);
  if (err != NVM3_OBJECTTYPE_DATA || type != NVM3_OBJECTTYPE_DATA) {
    printf("Incorrect data saved!\r\n");
    return;
  }

  err = nvm3_readData(NVM3_DEFAULT_HANDLE, DATA_KEY, &state_variables, len);
  if (ECODE_NVM3_OK == err) {
    printf("State loaded from memory!\r\n");
  } else {
    printf("Error loading state\r\n");
  }
}

/***************************************************************************//**
 * Initialize NVM3 example.
 ******************************************************************************/
void nvm3_app_init(void)
{
  Ecode_t err;

  // This will call nvm3_open() with default parameters for
  // memory base address and size, cache size, etc.
  err = nvm3_initDefault();
  EFM_ASSERT(err == ECODE_NVM3_OK);
}

/***************************************************************************//**
 * NVM3 ticking function.
 ******************************************************************************/
void nvm3_app_process_action(void)
{
  // Check if NVM3 controller can release any out-of-date objects
  // to free up memory.
  // This may take more than one call to nvm3_repack()
  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE)) {
    printf("Repacking NVM...\r\n");
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }
}
