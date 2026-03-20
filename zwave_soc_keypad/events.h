/**
 * @file
 *
 * Application-specific event identifiers.
 *
 * @copyright 2019 Silicon Laboratories Inc.
 */
#ifndef ZWAVE_SOC_KEYPAD_EVENTS_H_
#define ZWAVE_SOC_KEYPAD_EVENTS_H_

#include <ev_man.h>

/**
 * Defines application-owned events in the ZAF application event space.
 */
typedef enum {
  EVENT_EMPTY = DEFINE_EVENT_APP_NBR,
  EVENT_APP_PERIODIC_BATTERY_CHECK_TRIGGER,
  EVENT_APP_BATTERY_REPORT,
  EVENT_APP_KEYPAD_PROCESS,
  EVENT_APP_KEYPAD_TIMEOUT
}
EVENT_APP;

#endif /* ZWAVE_SOC_KEYPAD_EVENTS_H_ */
