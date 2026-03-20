/***************************************************************************//**
 * @file
 * @brief cli_keypad.c
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#if defined(SL_COMPONENT_CATALOG_PRESENT)
#include "sl_component_catalog.h"
#endif

#ifdef SL_CATALOG_ZW_CLI_COMMON_PRESENT

#include <ctype.h>
#include <string.h>
#include "sl_cli.h"
#include "app_log.h"
#include "events.h"
#include "zaf_event_distributor_soc.h"
#include "app_keypad.h"
// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

/******************************************************************************
 * CLI - battery_report: Send Battery Report
 *****************************************************************************/
void cli_battery_report(sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  app_log_info("Send Battery Report\r\n");
  zaf_event_distributor_enqueue_app_event(EVENT_APP_BATTERY_REPORT);
}

static bool cli_parse_key_token(const char *token, app_keypad_key_t *key)
{
  char normalized[16] = { 0 };
  size_t ix;

  if ((NULL == token) || (NULL == key)) {
    return false;
  }

  for (ix = 0; (ix < (sizeof(normalized) - 1U)) && (token[ix] != '\0'); ix++) {
    normalized[ix] = (char)tolower((unsigned char)token[ix]);
  }

  if ((strlen(normalized) == 1U) && (normalized[0] >= '0') && (normalized[0] <= '9')) {
    *key = (app_keypad_key_t)(APP_KEYPAD_KEY_0 + (normalized[0] - '0'));
    return true;
  }

  if ((0 == strcmp(normalized, "*")) || (0 == strcmp(normalized, "star"))) {
    *key = APP_KEYPAD_KEY_STAR;
    return true;
  }

  if ((0 == strcmp(normalized, "#")) || (0 == strcmp(normalized, "hash"))) {
    *key = APP_KEYPAD_KEY_HASH;
    return true;
  }

  if ((0 == strcmp(normalized, "ent")) || (0 == strcmp(normalized, "enter"))) {
    *key = APP_KEYPAD_KEY_ENTER;
    return true;
  }

  if ((0 == strcmp(normalized, "esc")) || (0 == strcmp(normalized, "escape"))
      || (0 == strcmp(normalized, "cancel"))) {
    *key = APP_KEYPAD_KEY_ESCAPE;
    return true;
  }

  if (0 == strcmp(normalized, "f1")) {
    *key = APP_KEYPAD_KEY_F1;
    return true;
  }

  if (0 == strcmp(normalized, "f2")) {
    *key = APP_KEYPAD_KEY_F2;
    return true;
  }

  if (0 == strcmp(normalized, "up")) {
    *key = APP_KEYPAD_KEY_UP;
    return true;
  }

  if (0 == strcmp(normalized, "down")) {
    *key = APP_KEYPAD_KEY_DOWN;
    return true;
  }

  if (0 == strcmp(normalized, "left")) {
    *key = APP_KEYPAD_KEY_LEFT;
    return true;
  }

  if (0 == strcmp(normalized, "right")) {
    *key = APP_KEYPAD_KEY_RIGHT;
    return true;
  }

  return false;
}

/******************************************************************************
 * CLI - keypad_key: Queue one keypad key
 *****************************************************************************/
void cli_keypad_key(sl_cli_command_arg_t *arguments)
{
  const char *token = sl_cli_get_argument_string(arguments, 0);
  app_keypad_key_t key;

  if (!cli_parse_key_token(token, &key)) {
    app_log_error("Unsupported keypad token: %s\r\n", token);
    return;
  }

  if (app_keypad_enqueue_key(key)) {
    app_log_info("Queued keypad key: %s\r\n", app_keypad_key_name(key));
  } else {
    app_log_error("Failed to queue keypad key: %s\r\n", app_keypad_key_name(key));
  }
}

/******************************************************************************
 * CLI - keypad_text: Queue ASCII keypad text
 *****************************************************************************/
void cli_keypad_text(sl_cli_command_arg_t *arguments)
{
  const char *text = sl_cli_get_argument_string(arguments, 0);

  if (app_keypad_enqueue_text(text)) {
    app_log_info("Queued keypad text: %s\r\n", text);
  } else {
    app_log_error("Failed to queue keypad text: %s\r\n", text);
  }
}

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------

#endif // SL_CATALOG_ZW_CLI_COMMON_PRESENT
