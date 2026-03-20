#include "app_keypad.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "AppTimer.h"
#include "CC_Common.h"
#include "FreeRTOS.h"
#include "keyscan_driver.h"
#include "queue.h"
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
#define APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED     (0U)
#define APP_KEYPAD_KEYSCAN_SCAN_ENABLED            (0U)

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
static bool s_initialized = false;
#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
static bool s_keyscan_event_subscribed = false;
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
static void app_keypad_keyscan_event_callback(uint8_t *p_keyscan_matrix,
                                              sl_keyscan_driver_status_t status);
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
app_keypad_keyscan_event_callback(__attribute__((unused)) uint8_t *p_keyscan_matrix,
                                  __attribute__((unused)) sl_keyscan_driver_status_t status)
{
  // The driver is wired in for bring-up, but matrix-to-key translation is added later.
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
      ZPAL_LOG_INFO(ZPAL_LOG_APP,
                    "Ignoring reserved keypad key '%s' in phase 1\n",
                    app_keypad_key_name(key));
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

    s_initialized = true;
  }

  app_keypad_load_config();
  app_keypad_clear_cached_ascii();

#if APP_KEYPAD_KEYSCAN_DRIVER_INIT_ENABLED
  if (!s_keyscan_event_subscribed) {
    sl_keyscan_driver_subscribe_event(&s_keyscan_event_handle);
    s_keyscan_event_subscribed = true;
  }

#if APP_KEYPAD_KEYSCAN_SCAN_ENABLED
  (void)sl_keyscan_driver_start_scan();
#else
  ZPAL_LOG_INFO(ZPAL_LOG_APP,
                "KEYSCAN driver initialized but scan start is disabled until matrix routing is ready\n");
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
