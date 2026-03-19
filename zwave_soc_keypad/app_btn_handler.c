/***************************************************************************//**
 * @file
 * @brief app_btn_handler.c
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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
#include <events.h>
#include <zaf_event_distributor_soc.h>
#include "app_button_handler.h"
#include "app_button_handler.h"
#include "zpal_log.h"

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
//                          Public Functions Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

/**
 * @brief Handles button 0 press events.
 *
 * This function is called when button 0 is pressed. It determines the duration
 * of the press and enqueues the corresponding application event or command class event.
 *
 * @param duration The duration of the button press. Possible values are:
 * - `APP_BUTTON_PRESS_DURATION_SHORT`: Short press duration.
 * - `APP_BUTTON_PRESS_DURATION_MEDIUM`: Medium press duration.
 * - `APP_BUTTON_PRESS_DURATION_LONG`: Long press duration.
 *
 * The function enqueues the following events based on the duration:
 * - `EVENT_APP_BATTERY_REPORT` for medium press.
 * - `EVENT_APP_DOORHANDLE_ACTIVATED` or `EVENT_APP_DOORHANDLE_DEACTIVATED` for long press based on the state of `doorhandle_status`.
 *
 * For short press, it simulates entering a user code on a key pad and enqueues a command class event:
 * - `COMMAND_CLASS_USER_CODE` with `CC_USER_CODE_EVENT_VALIDATE`.
 *
 * If the duration does not match any of the predefined values, no event is enqueued.
 */
void app_button_press_btn_0_handler(uint8_t duration)
{
  uint8_t app_event = EVENT_EMPTY;
  uint16_t command_class = COMMAND_CLASS_NO_OPERATION;
  uint8_t cc_event;
  void *cc_data;

  switch (duration) {
    case APP_BUTTON_PRESS_DURATION_SHORT:
      break;
    case APP_BUTTON_PRESS_DURATION_MEDIUM:
      app_event = EVENT_APP_BATTERY_REPORT;
      break;
    case APP_BUTTON_PRESS_DURATION_LONG:
      break;
    default:
      break;
  }

  if (app_event != EVENT_EMPTY) {
    app_event_enqueue(app_event);
  }

  if (command_class != COMMAND_CLASS_NO_OPERATION) {
    app_cc_event_enqueue(command_class, cc_event, cc_data);
  }
}

/**
 * @brief Handles button 1 press events.
 *
 * This function is called when button 1 is pressed. It determines the duration
 * of the press and enqueues the corresponding application event.
 *
 * @param duration The duration of the button press. Possible values are:
 * - `APP_BUTTON_PRESS_DURATION_SHORT`: Short press duration.
 * - `APP_BUTTON_PRESS_DURATION_VERYLONG`: Very long press duration.
 *
 * The function enqueues the following events based on the duration:
 * - `EVENT_SYSTEM_LEARNMODE_TOGGLE` for short press.
 * - `EVENT_SYSTEM_RESET` for very long press.
 *
 * If the duration does not match any of the predefined values, no event is enqueued.
 */
void app_button_press_btn_1_handler(uint8_t duration)
{
  uint8_t app_event = EVENT_EMPTY;

  switch (duration) {
    case APP_BUTTON_PRESS_DURATION_SHORT:
      app_event = EVENT_SYSTEM_LEARNMODE_TOGGLE;
      break;
    case APP_BUTTON_PRESS_DURATION_VERYLONG:
      app_event = EVENT_SYSTEM_RESET;
      break;
    default:
      break;
  }

  if (app_event != EVENT_EMPTY) {
    app_event_enqueue(app_event);
  }
}
