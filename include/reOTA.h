/* 
   EN: OTA update in the context of a specially created task
   RU: Обновление OTA в контексте специально созданной задачи
   --------------------------
   (с) 2021 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#ifndef __RE_OTA_H__
#define __RE_OTA_H__

#include <string.h>
#include "project_config.h"
#include "def_consts.h"
#include "rLog.h"
#include "rStrings.h"
#include "reEsp32.h"
#include "reEvents.h"
#if CONFIG_TELEGRAM_ENABLE
#include "reTgSend.h"
#endif // CONFIG_TELEGRAM_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

void otaStart(char *otaSource);

#ifdef __cplusplus
}
#endif

#endif // __RE_OTA_H__
