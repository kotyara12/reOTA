/* 
   EN: OTA update in the context of a specially created task
   RU: Обновление OTA в контексте специально созданной задачи
   --------------------------
   (с) 2021 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#ifndef __RE_OTA_H__
#define __RE_OTA_H__

#ifdef __cplusplus
extern "C" {
#endif

void otaStart(char *otaSource);

#ifdef __cplusplus
}
#endif

#endif // __RE_OTA_H__
