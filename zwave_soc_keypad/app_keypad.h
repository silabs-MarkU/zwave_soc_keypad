#ifndef APP_KEYPAD_H_
#define APP_KEYPAD_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  APP_KEYPAD_KEY_0,
  APP_KEYPAD_KEY_1,
  APP_KEYPAD_KEY_2,
  APP_KEYPAD_KEY_3,
  APP_KEYPAD_KEY_4,
  APP_KEYPAD_KEY_5,
  APP_KEYPAD_KEY_6,
  APP_KEYPAD_KEY_7,
  APP_KEYPAD_KEY_8,
  APP_KEYPAD_KEY_9,
  APP_KEYPAD_KEY_STAR,
  APP_KEYPAD_KEY_HASH,
  APP_KEYPAD_KEY_ENTER,
  APP_KEYPAD_KEY_ESCAPE,
  APP_KEYPAD_KEY_F1,
  APP_KEYPAD_KEY_F2,
  APP_KEYPAD_KEY_UP,
  APP_KEYPAD_KEY_DOWN,
  APP_KEYPAD_KEY_LEFT,
  APP_KEYPAD_KEY_RIGHT
} app_keypad_key_t;

void app_keypad_keyscan_platform_init(void);
void app_keypad_init(void);
bool app_keypad_enqueue_key(app_keypad_key_t key);
bool app_keypad_enqueue_key_from_isr(app_keypad_key_t key);
bool app_keypad_enqueue_text(const char *text);
void app_keypad_process_wake_event(void);
void app_keypad_process_event(void);
void app_keypad_process_timeout_event(void);
const char *app_keypad_key_name(app_keypad_key_t key);

#endif /* APP_KEYPAD_H_ */
