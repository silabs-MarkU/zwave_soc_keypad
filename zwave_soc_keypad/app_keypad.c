#include "app_keypad.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "AppTimer.h"
#include "CC_Common.h"
#include "FreeRTOS.h"
#include "keyscan_driver.h"
#include "keyscan_driver_config.h"
#include "queue.h"
#include "sl_gpio.h"
#include "sl_power_manager.h"
#include "ZAF_nvm_app.h"
#include "ZW_TransportEndpoint.h"
#include "events.h"
#include "zaf_event_distributor_soc.h"
#include "zpal_log.h"

#define APP_KEYPAD_APPLICATIONDATA_FILE_ID         (0x00000U)
#define APP_KEYPAD_CONFIG_MAGIC                    (0x4B504431UL)
#define APP_KEYPAD_DEFAULT_CACHE_SIZE              (4U)
#define APP_KEYPAD_DEFAULT_CACHE_TIMEOUT_SECONDS   (2U)
#define APP_KEYPAD_MAX_CACHE_SIZE                  (32U)
#define APP_KEYPAD_MAX_CACHE_TIMEOUT_SECONDS       (10U)
#define APP_KEYPAD_ASCII_PADDING_BLOCK_SIZE        (16U)
#define APP_KEYPAD_ASCII_PADDING_VALUE             (0xFFU)
#define APP_KEYPAD_KEY_QUEUE_LENGTH                (16U)
#define APP_KEYPAD_KEY_SUPPORTED_BITMASK_LENGTH    (8U)
#define APP_KEYPAD_DATA_TYPE_MASK_LENGTH           (1U)
#define APP_KEYPAD_EVENT_TYPE_MASK_LENGTH          (4U)
#define APP_KEYPAD_NOTIFICATION_MAX_DATA_LENGTH    (32U)
#define APP_KEYPAD_NOTIFICATION_HEADER_LENGTH      (4U)
#define APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED     (1U)
#define APP_KEYPAD_KEYSCAN_SCAN_ENABLED            (1U)
#define APP_KEYPAD_VERBOSE_LOGGING                 (0U)
#define APP_KEYPAD_ROW_COUNT                       (4U)
#define APP_KEYPAD_COLUMN_COUNT                    (5U)
#define APP_KEYPAD_KEYSCAN_ROUTEEN_MASK            (GPIO_KEYSCAN_ROUTEEN_COLOUT0PEN \
                                                    | GPIO_KEYSCAN_ROUTEEN_COLOUT1PEN \
                                                    | GPIO_KEYSCAN_ROUTEEN_COLOUT2PEN \
                                                    | GPIO_KEYSCAN_ROUTEEN_COLOUT3PEN \
                                                    | GPIO_KEYSCAN_ROUTEEN_COLOUT4PEN)
#define APP_KEYPAD_WAKE_GUARD_TIMEOUT_MS           (1500U)

typedef struct {
  uint32_t magic;
  uint8_t key_cache_size;
  uint8_t key_cache_timeout_seconds;
  uint8_t reserved[2];
} app_keypad_persistent_config_t;

static StaticQueue_t s_key_queue_struct;
static uint8_t s_key_queue_storage[APP_KEYPAD_KEY_QUEUE_LENGTH * sizeof(app_keypad_key_t)];
static QueueHandle_t s_key_queue = NULL;
static SSwTimer s_keypad_timeout_timer;
static SSwTimer s_keypad_wake_guard_timer;

static uint8_t s_cached_ascii[APP_KEYPAD_MAX_CACHE_SIZE];
static uint8_t s_cached_ascii_length = 0;
static uint8_t s_sequence_number = 0;
static app_keypad_persistent_config_t s_config = {
  .magic = APP_KEYPAD_CONFIG_MAGIC,
  .key_cache_size = APP_KEYPAD_DEFAULT_CACHE_SIZE,
  .key_cache_timeout_seconds = APP_KEYPAD_DEFAULT_CACHE_TIMEOUT_SECONDS,
  .reserved = { 0, 0 }
};
static bool s_timer_registered = false;
static bool s_wake_guard_timer_registered = false;
static bool s_initialized = false;
#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
static bool s_keyscan_event_subscribed = false;
static bool s_power_transition_subscribed = false;
static bool s_em2_wake_mode_active = false;
static bool s_wake_guard_em1_requirement_active = false;
static volatile uint8_t s_pending_wake_row_mask = 0U;
static int32_t s_row_interrupt_numbers[APP_KEYPAD_ROW_COUNT] = {
  SL_GPIO_INTERRUPT_UNAVAILABLE,
  SL_GPIO_INTERRUPT_UNAVAILABLE,
  SL_GPIO_INTERRUPT_UNAVAILABLE,
  SL_GPIO_INTERRUPT_UNAVAILABLE
};
#endif

static const char *const s_key_names[] = {
  "0",
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",
  "*",
  "#",
  "enter",
  "escape",
  "f1",
  "f2",
  "up",
  "down",
  "left",
  "right"
};

static const app_keypad_key_t s_keyscan_key_map[APP_KEYPAD_ROW_COUNT][APP_KEYPAD_COLUMN_COUNT] = {
  { APP_KEYPAD_KEY_LEFT,   APP_KEYPAD_KEY_7,      APP_KEYPAD_KEY_4,    APP_KEYPAD_KEY_1,    APP_KEYPAD_KEY_F1 },
  { APP_KEYPAD_KEY_0,      APP_KEYPAD_KEY_8,      APP_KEYPAD_KEY_5,    APP_KEYPAD_KEY_2,    APP_KEYPAD_KEY_F2 },
  { APP_KEYPAD_KEY_RIGHT,  APP_KEYPAD_KEY_9,      APP_KEYPAD_KEY_6,    APP_KEYPAD_KEY_3,    APP_KEYPAD_KEY_HASH },
  { APP_KEYPAD_KEY_ENTER,  APP_KEYPAD_KEY_ESCAPE, APP_KEYPAD_KEY_DOWN, APP_KEYPAD_KEY_UP,   APP_KEYPAD_KEY_STAR }
};

#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
static const sl_gpio_t s_keyscan_column_gpios[APP_KEYPAD_COLUMN_COUNT] = {
  { SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_0_PORT, SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_0_PIN },
  { SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_1_PORT, SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_1_PIN },
  { SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_2_PORT, SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_2_PIN },
  { SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_3_PORT, SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_3_PIN },
  { SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_4_PORT, SL_KEYSCAN_DRIVER_KEYSCAN_COL_OUT_4_PIN }
};

static const sl_gpio_t s_keyscan_row_gpios[APP_KEYPAD_ROW_COUNT] = {
  { SL_KEYSCAN_DRIVER_KEYSCAN_ROW_SENSE_0_PORT, SL_KEYSCAN_DRIVER_KEYSCAN_ROW_SENSE_0_PIN },
  { SL_KEYSCAN_DRIVER_KEYSCAN_ROW_SENSE_1_PORT, SL_KEYSCAN_DRIVER_KEYSCAN_ROW_SENSE_1_PIN },
  { SL_KEYSCAN_DRIVER_KEYSCAN_ROW_SENSE_2_PORT, SL_KEYSCAN_DRIVER_KEYSCAN_ROW_SENSE_2_PIN },
  { SL_KEYSCAN_DRIVER_KEYSCAN_ROW_SENSE_3_PORT, SL_KEYSCAN_DRIVER_KEYSCAN_ROW_SENSE_3_PIN }
};
#endif

static void app_keypad_apply_default_config(void);
static void app_keypad_clear_cached_ascii(void);
static void app_keypad_store_config(void);
static void app_keypad_timeout_timer_callback(SSwTimer *pTimer);
static uint8_t app_keypad_ascii_padded_length(uint8_t ascii_length);
static bool app_keypad_key_to_ascii(app_keypad_key_t key, uint8_t *ascii);
static void app_keypad_process_key(app_keypad_key_t key);
static void app_keypad_process_ascii_key(uint8_t ascii);
static void app_keypad_process_command_key(uint8_t event_type);
#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
static void app_keypad_power_transition_callback(sl_power_manager_em_t from,
                                                 sl_power_manager_em_t to);
static void app_keypad_enter_em2_wake_mode(void);
static void app_keypad_leave_em2_wake_mode(void);
static void app_keypad_disable_keyscan_column_routes(void);
static void app_keypad_enable_keyscan_column_routes(void);
static void app_keypad_start_wake_guard(uint32_t timeout_ms);
static void app_keypad_stop_wake_guard(void);
static void app_keypad_wake_guard_timer_callback(SSwTimer *pTimer);
static void app_keypad_row_wake_isr(uint8_t interrupt_number, void *context);
static bool app_keypad_try_decode_matrix_key(const uint8_t *p_keyscan_matrix,
                                             app_keypad_key_t *p_key);
static void app_keypad_keyscan_event_callback(uint8_t *p_keyscan_matrix,
                                              sl_keyscan_driver_status_t status);
static const sl_power_manager_em_transition_event_info_t s_power_transition_event_info = {
  .event_mask = SL_POWER_MANAGER_EVENT_TRANSITION_ENTERING_EM2
                | SL_POWER_MANAGER_EVENT_TRANSITION_LEAVING_EM2,
  .on_event = app_keypad_power_transition_callback
};
#endif
static JOB_STATUS app_keypad_send_notification(uint8_t event_type,
                                               const uint8_t *event_data,
                                               uint8_t event_data_length,
                                               bool use_ascii_encoding);
static void app_keypad_notification_callback(TRANSMISSION_RESULT *pTransmissionResult);
static uint8_t app_keypad_lifeline_reporting(ccc_pair_t *p_ccc_pair);
static received_frame_status_t app_keypad_command_class_handler(cc_handler_input_t *input,
                                                                cc_handler_output_t *output);
static void app_keypad_reset(void);
#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
static sl_keyscan_driver_process_keyscan_event_handle_t s_keyscan_event_handle = {
  .on_event = app_keypad_keyscan_event_callback
};
static sl_power_manager_em_transition_event_handle_t s_power_transition_event_handle;
#endif

static void
app_keypad_apply_default_config(void)
{
  s_config.magic = APP_KEYPAD_CONFIG_MAGIC;
  s_config.key_cache_size = APP_KEYPAD_DEFAULT_CACHE_SIZE;
  s_config.key_cache_timeout_seconds = APP_KEYPAD_DEFAULT_CACHE_TIMEOUT_SECONDS;
  s_config.reserved[0] = 0U;
  s_config.reserved[1] = 0U;
}

static void
app_keypad_load_config(void)
{
  app_keypad_persistent_config_t stored_config = { 0 };

  if ((ZPAL_STATUS_OK != ZAF_nvm_app_read(APP_KEYPAD_APPLICATIONDATA_FILE_ID,
                                          &stored_config,
                                          sizeof(stored_config)))
      || (stored_config.magic != APP_KEYPAD_CONFIG_MAGIC)
      || (stored_config.key_cache_size < 1U)
      || (stored_config.key_cache_size > APP_KEYPAD_MAX_CACHE_SIZE)
      || (stored_config.key_cache_timeout_seconds < 1U)
      || (stored_config.key_cache_timeout_seconds > APP_KEYPAD_MAX_CACHE_TIMEOUT_SECONDS)) {
    app_keypad_apply_default_config();
    app_keypad_store_config();
    return;
  }

  s_config = stored_config;
}

static void
app_keypad_store_config(void)
{
  zpal_status_t status = ZAF_nvm_app_write(APP_KEYPAD_APPLICATIONDATA_FILE_ID,
                                           &s_config,
                                           sizeof(s_config));

  if (ZPAL_STATUS_OK != status) {
    ZPAL_LOG_ERROR(ZPAL_LOG_APP,
                   "Failed to store keypad config, status=%d\n",
                   (int)status);
  }
}

static void
app_keypad_clear_cached_ascii(void)
{
  (void)TimerStop(&s_keypad_timeout_timer);
  memset(s_cached_ascii, 0, sizeof(s_cached_ascii));
  s_cached_ascii_length = 0U;
}

static uint8_t
app_keypad_ascii_padded_length(uint8_t ascii_length)
{
  uint8_t padded_length;

  if (0U == ascii_length) {
    return 0U;
  }

  padded_length = (uint8_t)(((ascii_length + (APP_KEYPAD_ASCII_PADDING_BLOCK_SIZE - 1U))
                             / APP_KEYPAD_ASCII_PADDING_BLOCK_SIZE)
                            * APP_KEYPAD_ASCII_PADDING_BLOCK_SIZE);

  return padded_length;
}

static bool
app_keypad_key_to_ascii(app_keypad_key_t key, uint8_t *ascii)
{
  static const uint8_t ascii_map[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '#'
  };

  if ((key > APP_KEYPAD_KEY_HASH) || (NULL == ascii)) {
    return false;
  }

  *ascii = ascii_map[key];
  return true;
}

#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
static void
app_keypad_disable_keyscan_column_routes(void)
{
  GPIO->KEYSCANROUTE.ROUTEEN &= ~APP_KEYPAD_KEYSCAN_ROUTEEN_MASK;
}

static void
app_keypad_enable_keyscan_column_routes(void)
{
  GPIO->KEYSCANROUTE.ROUTEEN |= APP_KEYPAD_KEYSCAN_ROUTEEN_MASK;
}

static void
app_keypad_enter_em2_wake_mode(void)
{
  if (s_em2_wake_mode_active) {
    return;
  }

  (void)sl_keyscan_driver_stop_scan();
  app_keypad_disable_keyscan_column_routes();

  for (uint8_t column = 0U; column < APP_KEYPAD_COLUMN_COUNT; column++) {
    sl_gpio_set_pin_mode(&s_keyscan_column_gpios[column], SL_GPIO_MODE_PUSH_PULL, 0);
  }

  for (uint8_t row = 0U; row < APP_KEYPAD_ROW_COUNT; row++) {
    int32_t interrupt_number = SL_GPIO_INTERRUPT_UNAVAILABLE;

    sl_gpio_set_pin_mode(&s_keyscan_row_gpios[row], SL_GPIO_MODE_INPUT_PULL, 1);
    if (SL_STATUS_OK
        != sl_gpio_configure_external_interrupt(&s_keyscan_row_gpios[row],
                                                &interrupt_number,
                                                SL_GPIO_INTERRUPT_FALLING_EDGE,
                                                app_keypad_row_wake_isr,
                                                (void *)(uintptr_t)row)) {
      ZPAL_LOG_ERROR(ZPAL_LOG_APP,
                     "Failed to arm keypad wake interrupt for row %u\n",
                     row);
      continue;
    }

    s_row_interrupt_numbers[row] = interrupt_number;
  }

  s_em2_wake_mode_active = true;
}

static void
app_keypad_leave_em2_wake_mode(void)
{
  if (!s_em2_wake_mode_active) {
    return;
  }

  for (uint8_t row = 0U; row < APP_KEYPAD_ROW_COUNT; row++) {
    if (SL_GPIO_INTERRUPT_UNAVAILABLE != s_row_interrupt_numbers[row]) {
      (void)sl_gpio_deconfigure_external_interrupt(s_row_interrupt_numbers[row]);
      s_row_interrupt_numbers[row] = SL_GPIO_INTERRUPT_UNAVAILABLE;
    }

    sl_gpio_set_pin_mode(&s_keyscan_row_gpios[row], SL_GPIO_MODE_INPUT_PULL, 1);
  }

  for (uint8_t column = 0U; column < APP_KEYPAD_COLUMN_COUNT; column++) {
    sl_gpio_set_pin_mode(&s_keyscan_column_gpios[column], SL_GPIO_MODE_WIRED_AND, 1);
  }

  app_keypad_enable_keyscan_column_routes();
  s_em2_wake_mode_active = false;

#if APP_KEYPAD_KEYSCAN_SCAN_ENABLED
  (void)sl_keyscan_driver_start_scan();
#endif
}

static void
app_keypad_power_transition_callback(sl_power_manager_em_t from,
                                     sl_power_manager_em_t to)
{
  if ((from != SL_POWER_MANAGER_EM2) && (to == SL_POWER_MANAGER_EM2)) {
    app_keypad_enter_em2_wake_mode();
    return;
  }

  if ((from == SL_POWER_MANAGER_EM2) && (to != SL_POWER_MANAGER_EM2)) {
    app_keypad_leave_em2_wake_mode();
  }
}

static void
app_keypad_row_wake_isr(__attribute__((unused)) uint8_t interrupt_number,
                        void *context)
{
  uintptr_t row = (uintptr_t)context;

  if (row < APP_KEYPAD_ROW_COUNT) {
    s_pending_wake_row_mask |= (uint8_t)(1U << row);
  }

  (void)zaf_event_distributor_enqueue_app_event_from_isr(EVENT_APP_KEYPAD_WAKE);
}

static void
app_keypad_keyscan_event_callback(uint8_t *p_keyscan_matrix,
                                  sl_keyscan_driver_status_t status)
{
  app_keypad_key_t key;

  if (NULL == p_keyscan_matrix) {
    return;
  }

#if SL_KEYSCAN_DRIVER_SINGLEPRESS
  if (SL_KEYSCAN_STATUS_KEYPRESS_VALID != status) {
    return;
  }
#else
  if (SL_KEYSCAN_STATUS_KEYPRESS_RELEASED != status) {
    return;
  }
#endif

  if (!app_keypad_try_decode_matrix_key(p_keyscan_matrix, &key)) {
    return;
  }

  if (!app_keypad_enqueue_key_from_isr(key)) {
    ZPAL_LOG_ERROR(ZPAL_LOG_APP,
                   "Failed to enqueue decoded KEYSCAN key '%s'\n",
                   app_keypad_key_name(key));
  }
}
#endif

#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
static bool
app_keypad_try_decode_matrix_key(const uint8_t *p_keyscan_matrix,
                                 app_keypad_key_t *p_key)
{
  uint8_t active_column = APP_KEYPAD_COLUMN_COUNT;
  uint8_t active_row = 0U;
  const uint8_t row_mask_limit = (uint8_t)((1U << APP_KEYPAD_ROW_COUNT) - 1U);

  if ((NULL == p_keyscan_matrix) || (NULL == p_key)) {
    return false;
  }

  for (uint8_t column = 0U; column < APP_KEYPAD_COLUMN_COUNT; column++) {
    uint8_t row_mask = (uint8_t)(p_keyscan_matrix[column] & row_mask_limit);

    if (0U == row_mask) {
      continue;
    }

    if ((row_mask & (uint8_t)(row_mask - 1U)) != 0U) {
      return false;
    }

    if (active_column < APP_KEYPAD_COLUMN_COUNT) {
      return false;
    }

    active_column = column;
    active_row = 0U;
    while ((row_mask & 0x01U) == 0U) {
      row_mask >>= 1;
      active_row++;
    }
  }

  if ((active_column >= APP_KEYPAD_COLUMN_COUNT) || (active_row >= APP_KEYPAD_ROW_COUNT)) {
    return false;
  }

#if SL_KEYSCAN_DRIVER_SINGLEPRESS
  // Bench testing shows the immediate KEYPRESS_VALID path reports the active
  // scan column one slot early, so advance it before logical key lookup.
  active_column = (uint8_t)((active_column + 1U) % APP_KEYPAD_COLUMN_COUNT);
#endif

  *p_key = s_keyscan_key_map[active_row][active_column];
  return true;
}

static void
app_keypad_start_wake_guard(uint32_t timeout_ms)
{
  if (!s_wake_guard_em1_requirement_active) {
    sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
    s_wake_guard_em1_requirement_active = true;
  }

  (void)TimerStop(&s_keypad_wake_guard_timer);
  if (ESWTIMER_STATUS_FAILED == TimerStart(&s_keypad_wake_guard_timer,
                                           timeout_ms)) {
    ZPAL_LOG_ERROR(ZPAL_LOG_APP,
                   "Failed to start keypad wake guard timer\n");
    app_keypad_stop_wake_guard();
  }
}

static void
app_keypad_stop_wake_guard(void)
{
  (void)TimerStop(&s_keypad_wake_guard_timer);

  if (s_wake_guard_em1_requirement_active) {
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
    s_wake_guard_em1_requirement_active = false;
  }
}

static void
app_keypad_wake_guard_timer_callback(__attribute__((unused)) SSwTimer *pTimer)
{
  app_keypad_stop_wake_guard();
}
#endif

static bool
app_keypad_enqueue_key_internal(app_keypad_key_t key, bool from_isr)
{
  BaseType_t queue_status;
  bool event_status;

  if (NULL == s_key_queue) {
    return false;
  }

  if (from_isr) {
    BaseType_t task_woken = pdFALSE;
    queue_status = xQueueSendToBackFromISR(s_key_queue, &key, &task_woken);
    event_status = zaf_event_distributor_enqueue_app_event_from_isr(EVENT_APP_KEYPAD_PROCESS);
  } else {
    queue_status = xQueueSendToBack(s_key_queue, &key, 0);
    event_status = zaf_event_distributor_enqueue_app_event(EVENT_APP_KEYPAD_PROCESS);
  }

  if ((pdPASS != queue_status) || !event_status) {
    ZPAL_LOG_ERROR(ZPAL_LOG_APP,
                   "Failed to enqueue keypad key '%s'\n",
                   app_keypad_key_name(key));
    return false;
  }

  return true;
}

static void
app_keypad_timeout_timer_callback(__attribute__((unused)) SSwTimer *pTimer)
{
  if (!zaf_event_distributor_enqueue_app_event(EVENT_APP_KEYPAD_TIMEOUT)) {
    ZPAL_LOG_ERROR(ZPAL_LOG_APP, "Failed to enqueue keypad timeout event\n");
  }
}

static void
app_keypad_set_bit(uint8_t *bitmask, uint8_t bit_index)
{
  bitmask[bit_index / 8U] |= (uint8_t)(1U << (bit_index % 8U));
}

static void
app_keypad_build_key_supported_report(cc_handler_output_t *output)
{
  uint8_t key_bitmask[APP_KEYPAD_KEY_SUPPORTED_BITMASK_LENGTH] = { 0U };
  uint8_t *frame = (uint8_t *)output->frame;
  uint8_t ascii_value = 0U;

  for (app_keypad_key_t key = APP_KEYPAD_KEY_0; key <= APP_KEYPAD_KEY_HASH; key++) {
    if (app_keypad_key_to_ascii(key, &ascii_value)) {
      app_keypad_set_bit(key_bitmask, ascii_value);
    }
  }

  frame[0] = COMMAND_CLASS_ENTRY_CONTROL;
  frame[1] = ENTRY_CONTROL_KEY_SUPPORTED_REPORT;
  frame[2] = APP_KEYPAD_KEY_SUPPORTED_BITMASK_LENGTH;
  memcpy(&frame[3], key_bitmask, sizeof(key_bitmask));
  output->length = (uint8_t)(3U + sizeof(key_bitmask));
}

static void
app_keypad_build_event_supported_report(cc_handler_output_t *output)
{
  uint8_t *frame = (uint8_t *)output->frame;

  memset(frame, 0, 13U);

  frame[0] = COMMAND_CLASS_ENTRY_CONTROL;
  frame[1] = ENTRY_CONTROL_EVENT_SUPPORTED_REPORT;
  frame[2] = APP_KEYPAD_DATA_TYPE_MASK_LENGTH;
  frame[3] = (uint8_t)((1U << ENTRY_CONTROL_NOTIFICATION_DATA_TYPE_NA)
                       | (1U << ENTRY_CONTROL_NOTIFICATION_DATA_TYPE_ASCII));
  frame[4] = APP_KEYPAD_EVENT_TYPE_MASK_LENGTH;
  app_keypad_set_bit(&frame[5], ENTRY_CONTROL_NOTIFICATION_CACHING);
  app_keypad_set_bit(&frame[5], ENTRY_CONTROL_NOTIFICATION_CACHED_KEYS);
  app_keypad_set_bit(&frame[5], ENTRY_CONTROL_NOTIFICATION_ENTER);
  app_keypad_set_bit(&frame[5], ENTRY_CONTROL_NOTIFICATION_CANCEL);
  frame[9] = 1U;
  frame[10] = APP_KEYPAD_MAX_CACHE_SIZE;
  frame[11] = 1U;
  frame[12] = APP_KEYPAD_MAX_CACHE_TIMEOUT_SECONDS;

  output->length = 13U;
}

static void
app_keypad_build_configuration_report(cc_handler_output_t *output)
{
  uint8_t *frame = (uint8_t *)output->frame;

  frame[0] = COMMAND_CLASS_ENTRY_CONTROL;
  frame[1] = ENTRY_CONTROL_CONFIGURATION_REPORT;
  frame[2] = s_config.key_cache_size;
  frame[3] = s_config.key_cache_timeout_seconds;
  output->length = sizeof(ZW_ENTRY_CONTROL_CONFIGURATION_REPORT_FRAME);
}

static received_frame_status_t
app_keypad_command_class_handler(cc_handler_input_t *input, cc_handler_output_t *output)
{
  switch (input->frame->ZW_Common.cmd) {
    case ENTRY_CONTROL_KEY_SUPPORTED_GET:
      if (Check_not_legal_response_job(input->rx_options)) {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      app_keypad_build_key_supported_report(output);
      return RECEIVED_FRAME_STATUS_SUCCESS;

    case ENTRY_CONTROL_EVENT_SUPPORTED_GET:
      if (Check_not_legal_response_job(input->rx_options)) {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      app_keypad_build_event_supported_report(output);
      return RECEIVED_FRAME_STATUS_SUCCESS;

    case ENTRY_CONTROL_CONFIGURATION_GET:
      if (Check_not_legal_response_job(input->rx_options)) {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      app_keypad_build_configuration_report(output);
      return RECEIVED_FRAME_STATUS_SUCCESS;

    case ENTRY_CONTROL_CONFIGURATION_SET:
    {
      uint8_t key_cache_size = input->frame->ZW_EntryControlConfigurationSetFrame.keyCacheSize;
      uint8_t key_cache_timeout = input->frame->ZW_EntryControlConfigurationSetFrame.keyCacheTimeout;

      if ((key_cache_size < 1U)
          || (key_cache_size > APP_KEYPAD_MAX_CACHE_SIZE)
          || (key_cache_timeout < 1U)
          || (key_cache_timeout > APP_KEYPAD_MAX_CACHE_TIMEOUT_SECONDS)) {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      s_config.key_cache_size = key_cache_size;
      s_config.key_cache_timeout_seconds = key_cache_timeout;
      app_keypad_store_config();
      app_keypad_clear_cached_ascii();
      return RECEIVED_FRAME_STATUS_SUCCESS;
    }

    default:
      return RECEIVED_FRAME_STATUS_NO_SUPPORT;
  }
}

static void
app_keypad_notification_callback(TRANSMISSION_RESULT *pTransmissionResult)
{
  if ((NULL == pTransmissionResult)
      || (pTransmissionResult->isFinished != TRANSMISSION_RESULT_FINISHED)) {
    return;
  }

  if ((pTransmissionResult->status != TRANSMIT_COMPLETE_OK)
      && (pTransmissionResult->status != TRANSMIT_COMPLETE_VERIFIED)) {
    ZPAL_LOG_WARNING(ZPAL_LOG_APP,
                     "Entry Control notification failed, status=%u\n",
                     pTransmissionResult->status);
  }
}

static JOB_STATUS
app_keypad_send_notification(uint8_t event_type,
                             const uint8_t *event_data,
                             uint8_t event_data_length,
                             bool use_ascii_encoding)
{
  uint8_t payload[APP_KEYPAD_NOTIFICATION_HEADER_LENGTH + APP_KEYPAD_NOTIFICATION_MAX_DATA_LENGTH] = { 0U };
  uint8_t payload_length = APP_KEYPAD_NOTIFICATION_HEADER_LENGTH;
  CMD_CLASS_GRP command = {
    .cmdClass = COMMAND_CLASS_ENTRY_CONTROL,
    .cmd = ENTRY_CONTROL_NOTIFICATION
  };

  payload[0] = s_sequence_number++;
  payload[1] = use_ascii_encoding ? ENTRY_CONTROL_NOTIFICATION_DATA_TYPE_ASCII
                                  : ENTRY_CONTROL_NOTIFICATION_DATA_TYPE_NA;
  payload[2] = event_type;

  if (use_ascii_encoding && (event_data_length > 0U)) {
    uint8_t padded_length = app_keypad_ascii_padded_length(event_data_length);

    memset(&payload[4], APP_KEYPAD_ASCII_PADDING_VALUE, padded_length);
    memcpy(&payload[4], event_data, event_data_length);
    payload[3] = padded_length;
    payload_length = (uint8_t)(payload_length + padded_length);
  } else {
    payload[3] = 0U;
  }

  ZPAL_LOG_DEBUG(ZPAL_LOG_APP,
                 "Entry Control notify event=0x%02x data_len=%u\n",
                 event_type,
                 payload[3]);

  return cc_engine_multicast_request(NULL,
                                     ENDPOINT_ROOT,
                                     &command,
                                     payload,
                                     payload_length,
                                     false,
                                     app_keypad_notification_callback);
}

static void
app_keypad_restart_timeout_timer(void)
{
  uint32_t timeout_ms = (uint32_t)s_config.key_cache_timeout_seconds * 1000UL;

  if (0U == timeout_ms) {
    return;
  }

  if (ESWTIMER_STATUS_FAILED == TimerStart(&s_keypad_timeout_timer, timeout_ms)) {
    ZPAL_LOG_ERROR(ZPAL_LOG_APP, "Failed to start keypad timeout timer\n");
  }
}

static void
app_keypad_process_ascii_key(uint8_t ascii)
{
  if (0U == s_cached_ascii_length && (s_config.key_cache_size > 1U)) {
    (void)app_keypad_send_notification(ENTRY_CONTROL_NOTIFICATION_CACHING,
                                       NULL,
                                       0U,
                                       false);
  }

  if (s_cached_ascii_length >= APP_KEYPAD_MAX_CACHE_SIZE) {
    ZPAL_LOG_WARNING(ZPAL_LOG_APP, "Keypad cache full, dropping ASCII 0x%02x\n", ascii);
    return;
  }

  s_cached_ascii[s_cached_ascii_length++] = ascii;

  if ((1U == s_config.key_cache_size)
      || (s_cached_ascii_length >= s_config.key_cache_size)) {
    (void)app_keypad_send_notification(ENTRY_CONTROL_NOTIFICATION_CACHED_KEYS,
                                       s_cached_ascii,
                                       s_cached_ascii_length,
                                       true);
    app_keypad_clear_cached_ascii();
    return;
  }

  app_keypad_restart_timeout_timer();

#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
  app_keypad_start_wake_guard((uint32_t)s_config.key_cache_timeout_seconds * 1000UL);
#endif
}

static void
app_keypad_process_command_key(uint8_t event_type)
{
  if (s_cached_ascii_length > 0U) {
    (void)app_keypad_send_notification(event_type,
                                       s_cached_ascii,
                                       s_cached_ascii_length,
                                       true);
  } else {
    (void)app_keypad_send_notification(event_type, NULL, 0U, false);
  }

  app_keypad_clear_cached_ascii();
}

static void
app_keypad_process_key(app_keypad_key_t key)
{
  uint8_t ascii = 0U;

#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
  app_keypad_start_wake_guard(APP_KEYPAD_WAKE_GUARD_TIMEOUT_MS);
#endif

  ZPAL_LOG_INFO(ZPAL_LOG_APP, "Key: '%s'\n", app_keypad_key_name(key));

  if (app_keypad_key_to_ascii(key, &ascii)) {
    app_keypad_process_ascii_key(ascii);
    return;
  }

  switch (key) {
    case APP_KEYPAD_KEY_ENTER:
      app_keypad_process_command_key(ENTRY_CONTROL_NOTIFICATION_ENTER);
      break;

    case APP_KEYPAD_KEY_ESCAPE:
      app_keypad_process_command_key(ENTRY_CONTROL_NOTIFICATION_CANCEL);
      break;

    default:
#if APP_KEYPAD_VERBOSE_LOGGING
      ZPAL_LOG_INFO(ZPAL_LOG_APP,
                    "Ignoring reserved keypad key '%s' in phase 1\n",
                    app_keypad_key_name(key));
#endif
      break;
  }
}

static uint8_t
app_keypad_lifeline_reporting(ccc_pair_t *p_ccc_pair)
{
  p_ccc_pair->cmdClass = COMMAND_CLASS_ENTRY_CONTROL;
  p_ccc_pair->cmd = ENTRY_CONTROL_NOTIFICATION;
  return 1U;
}

static void
app_keypad_reset(void)
{
  app_keypad_apply_default_config();
  app_keypad_store_config();

  if (s_initialized) {
    app_keypad_clear_cached_ascii();
  }
}

void
app_keypad_keyscan_platform_init(void)
{
#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
  (void)sl_keyscan_driver_init();
#endif
}

void
app_keypad_init(void)
{
  if (!s_initialized) {
    s_key_queue = xQueueCreateStatic(APP_KEYPAD_KEY_QUEUE_LENGTH,
                                     sizeof(app_keypad_key_t),
                                     s_key_queue_storage,
                                     &s_key_queue_struct);
    assert(NULL != s_key_queue);

    if (!s_timer_registered) {
      s_timer_registered = AppTimerRegister(&s_keypad_timeout_timer,
                                            false,
                                            app_keypad_timeout_timer_callback);
      assert(s_timer_registered);
    }

    if (!s_wake_guard_timer_registered) {
      s_wake_guard_timer_registered = AppTimerRegister(&s_keypad_wake_guard_timer,
                                                       false,
                                                       app_keypad_wake_guard_timer_callback);
      assert(s_wake_guard_timer_registered);
    }

    s_initialized = true;
  }

  app_keypad_load_config();
  app_keypad_clear_cached_ascii();

#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
  if (!s_keyscan_event_subscribed) {
    sl_keyscan_driver_subscribe_event(&s_keyscan_event_handle);
    s_keyscan_event_subscribed = true;
  }

  if (!s_power_transition_subscribed) {
    sl_power_manager_subscribe_em_transition_event(&s_power_transition_event_handle,
                                                   &s_power_transition_event_info);
    s_power_transition_subscribed = true;
  }

#if APP_KEYPAD_KEYSCAN_SCAN_ENABLED
  (void)sl_keyscan_driver_start_scan();
#else
  ZPAL_LOG_INFO(ZPAL_LOG_APP,
                "KEYSCAN driver initialized; scan start is held off until matrix decoding is ready\n");
#endif
#else
  ZPAL_LOG_INFO(ZPAL_LOG_APP,
                "KEYSCAN driver integration is compiled in but init is disabled until pin-tool routing is ready\n");
#endif
}

bool
app_keypad_enqueue_key(app_keypad_key_t key)
{
  return app_keypad_enqueue_key_internal(key, false);
}

bool
app_keypad_enqueue_key_from_isr(app_keypad_key_t key)
{
  return app_keypad_enqueue_key_internal(key, true);
}

bool
app_keypad_enqueue_text(const char *text)
{
  size_t ix;

  if (NULL == text) {
    return false;
  }

  for (ix = 0U; text[ix] != '\0'; ix++) {
    app_keypad_key_t key;

    switch (text[ix]) {
      case '0': key = APP_KEYPAD_KEY_0; break;
      case '1': key = APP_KEYPAD_KEY_1; break;
      case '2': key = APP_KEYPAD_KEY_2; break;
      case '3': key = APP_KEYPAD_KEY_3; break;
      case '4': key = APP_KEYPAD_KEY_4; break;
      case '5': key = APP_KEYPAD_KEY_5; break;
      case '6': key = APP_KEYPAD_KEY_6; break;
      case '7': key = APP_KEYPAD_KEY_7; break;
      case '8': key = APP_KEYPAD_KEY_8; break;
      case '9': key = APP_KEYPAD_KEY_9; break;
      case '*': key = APP_KEYPAD_KEY_STAR; break;
      case '#': key = APP_KEYPAD_KEY_HASH; break;
      default:
        ZPAL_LOG_ERROR(ZPAL_LOG_APP,
                       "Unsupported keypad text character '%c'\n",
                       text[ix]);
        return false;
    }

    if (!app_keypad_enqueue_key(key)) {
      return false;
    }
  }

  return true;
}

void
app_keypad_process_wake_event(void)
{
#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
  uint8_t wake_row_mask;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();
  wake_row_mask = s_pending_wake_row_mask;
  s_pending_wake_row_mask = 0U;
  CORE_EXIT_CRITICAL();

  if (0U == wake_row_mask) {
    return;
  }

  app_keypad_start_wake_guard(APP_KEYPAD_WAKE_GUARD_TIMEOUT_MS);

#if APP_KEYPAD_VERBOSE_LOGGING
  ZPAL_LOG_INFO(ZPAL_LOG_APP,
                "Keypad EM2 wake detected, row mask=0x%02x\n",
                wake_row_mask);
#endif
#endif
}

void
app_keypad_process_event(void)
{
  app_keypad_key_t key;

  while (pdPASS == xQueueReceive(s_key_queue, &key, 0U)) {
    app_keypad_process_key(key);
  }
}

void
app_keypad_process_timeout_event(void)
{
  if (0U == s_cached_ascii_length) {
    return;
  }

  (void)app_keypad_send_notification(ENTRY_CONTROL_NOTIFICATION_CACHED_KEYS,
                                     s_cached_ascii,
                                     s_cached_ascii_length,
                                     true);
  app_keypad_clear_cached_ascii();
}

const char *
app_keypad_key_name(app_keypad_key_t key)
{
  if (key > APP_KEYPAD_KEY_RIGHT) {
    return "unknown";
  }

  return s_key_names[key];
}

REGISTER_CC_V5(COMMAND_CLASS_ENTRY_CONTROL,
               ENTRY_CONTROL_VERSION,
               app_keypad_command_class_handler,
               NULL,
               NULL,
               app_keypad_lifeline_reporting,
               0,
               NULL,
               app_keypad_reset);
