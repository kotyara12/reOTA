#include "reOTA.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "mbedtls/ssl.h"

static const char* logTAG = "OTA";
static const char* otaTaskName = "ota";
static TaskHandle_t _otaTask = nullptr;

#ifndef CONFIG_OTA_PEM_STORAGE
  #define CONFIG_OTA_PEM_STORAGE TLS_CERT_BUFFER
#endif // CONFIG_OTA_PEM_STORAGE

#if CONFIG_OTA_PEM_STORAGE == TLS_CERT_BUFFER
  extern const char ota_pem_start[]  asm(CONFIG_OTA_PEM_START);
  extern const char ota_pem_end[]    asm(CONFIG_OTA_PEM_END); 
#endif // CONFIG_OTA_PEM_STORAGE

void otaTaskExec(void *pvParameters)
{
  if (pvParameters) {
    char* otaSource = (char*)pvParameters;

    #if CONFIG_TELEGRAM_ENABLE && CONFIG_NOTIFY_TELEGRAM_OTA
      tgSend(MK_SERVICE, CONFIG_NOTIFY_TELEGRAM_OTA_PRIORITY, CONFIG_NOTIFY_TELEGRAM_ALERT_OTA, CONFIG_TELEGRAM_DEVICE, 
        CONFIG_MESSAGE_TG_OTA, otaSource);
    #endif // CONFIG_TELEGRAM_ENABLE && CONFIG_NOTIFY_TELEGRAM_OTA

    // Notify other tasks to suspend activities
    eventLoopPostSystem(RE_SYS_OTA, RE_SYS_SET);
    vTaskDelay(pdMS_TO_TICKS(CONFIG_OTA_DELAY));

    // Start watchdog timer
    static re_restart_timer_t otaTimer;
    espRestartTimerStartS(&otaTimer, RR_OTA_TIMEOUT, CONFIG_OTA_WATCHDOG, true);

    esp_http_client_config_t cfgOTA;
    uint8_t tryUpdate = 0;
    esp_err_t err = ESP_OK;
    do {
      tryUpdate++;
      rlog_i(logTAG, "Start of firmware upgrade from \"%s\", attempt %d", otaSource, tryUpdate);
      
      memset(&cfgOTA, 0, sizeof(cfgOTA));
      cfgOTA.url = otaSource;
      cfgOTA.skip_cert_common_name_check = false;
      #if CONFIG_OTA_PEM_STORAGE == TLS_CERT_BUFFER
        cfgOTA.use_global_ca_store = false;
        cfgOTA.cert_pem = ota_pem_start;
      #elif CONFIG_OTA_PEM_STORAGE == TLS_CERT_GLOBAL
        cfgOTA.use_global_ca_store = true;
      #elif CONFIG_OTA_PEM_STORAGE == TLS_CERT_BUNDLE
        cfgOTA.use_global_ca_store = false;
        cfgOTA.crt_bundle_attach = esp_crt_bundle_attach;
      #endif // CONFIG_OTA_PEM_STORAGE
      cfgOTA.is_async = false;

      err = esp_https_ota(&cfgOTA);
      if (err == ESP_OK) {
        rlog_i(logTAG, "Firmware upgrade completed!");
      } else {
        rlog_e(logTAG, "Firmware upgrade failed: %d!", err);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_OTA_DELAY));
      };
    } while ((err != ESP_OK) && (tryUpdate < CONFIG_OTA_ATTEMPTS));

    // Notify other tasks to restore activities
    eventLoopPostSystem(RE_SYS_OTA, RE_SYS_CLEAR);

    #if CONFIG_TELEGRAM_ENABLE && CONFIG_NOTIFY_TELEGRAM_OTA
    if (err == ESP_OK) {
      tgSend(MK_SERVICE, CONFIG_NOTIFY_TELEGRAM_OTA_PRIORITY, CONFIG_NOTIFY_TELEGRAM_ALERT_OTA, CONFIG_TELEGRAM_DEVICE, 
        CONFIG_MESSAGE_TG_OTA_OK, err);
    } else {
      tgSend(MK_SERVICE, CONFIG_NOTIFY_TELEGRAM_OTA_PRIORITY, CONFIG_NOTIFY_TELEGRAM_ALERT_OTA, CONFIG_TELEGRAM_DEVICE, 
        CONFIG_MESSAGE_TG_OTA_FAILED, err);
    };
    #endif // CONFIG_TELEGRAM_ENABLE && CONFIG_NOTIFY_TELEGRAM_OTA

    // Free resources
    if (otaSource) free(otaSource);
    _otaTask = nullptr;
    
    // Stop timer
    if (err == ESP_OK) {
      espRestartTimerStart(&otaTimer, RR_OTA, CONFIG_OTA_DELAY, true);
    } else {
      espRestartTimerFree(&otaTimer);
    };
  };

  eventLoopPostSystem(RE_SYS_OTA, RE_SYS_CLEAR);

  vTaskDelete(nullptr);
  rlog_i(logTAG, "Task [ %s ] has been deleted", otaTaskName);
}

void otaStart(char *otaSource)
{
  if (otaSource) {
    if (_otaTask == nullptr) {
      xTaskCreatePinnedToCore(otaTaskExec, otaTaskName, CONFIG_OTA_TASK_STACK_SIZE, (void*)otaSource, CONFIG_TASK_PRIORITY_OTA, &_otaTask, CONFIG_TASK_CORE_OTA);
      if (_otaTask) {
        rloga_i("Task [ %s ] has been successfully created and started", otaTaskName);
      }
      else {
        rloga_e("Failed to create a task for OTA update!");
        free(otaSource);
      };
    } else {
      rloga_e("OTA update has already started!");
      free(otaSource);
    };
  } else {
    rlog_e(logTAG, "Update source not specified");
  };
}