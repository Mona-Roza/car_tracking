#ifndef _MORO_COMMON_H_
#define _MORO_COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <map>
#include <string>
#include <vector>

#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"  // Include the semaphore definitions
#include "freertos/task.h"	  // Include the task definitions
#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"


/*
    If you want to use main in C++ you have to use 
    extern "C" to prevent name mangling.
*/
extern "C" {
void app_main(void);
}

//========================== MORO_LOG ==========================
typedef int (*printf_function_t)(const char*, va_list);
int moro_printf(const char* format, ...);
int moro_log_write(uint8_t log_level_macro, const char* format, ...);
void moro_printf_list(const uint8_t* c, int len, const char* format, const char* start, const char* sep, const char* end);

void moro_log_set_level(uint8_t log_level_macro);
uint8_t moro_log_get_level();
void forward_esp_log_to_moro_printf();

printf_function_t change_printf_function(printf_function_t ptr);
void reset_printf_function();
bool add_printf_function(printf_function_t ptr);
bool remove_printf_function(printf_function_t ptr);
void reset_esp_log_printf();

#define ANSI_COLOR_BLACK "\033[1;30m"	 // 000
#define ANSI_COLOR_RED "\033[1;31m"		 // 100
#define ANSI_COLOR_GREEN "\033[1;32m"	 // 010
#define ANSI_COLOR_YELLOW "\033[1;33m"	 // 011
#define ANSI_COLOR_BLUE "\033[1;34m"	 // 001
#define ANSI_COLOR_MAGENTA "\033[1;35m"	 // 101
#define ANSI_COLOR_CYAN "\033[1;36m"	 // 011
#define ANSI_COLOR_WHITE "\033[1;37m"	 // 111
#define ANSI_COLOR_RESET "\033[0m"		 // 0

#define MORO_LOG_LEVEL_NONE 0
#define MORO_LOG_LEVEL_ERROR 1
#define MORO_LOG_LEVEL_WARNING 2
#define MORO_LOG_LEVEL_INFO 3
#define MORO_LOG_LEVEL_DEBUG 4
#define MORO_LOG_LEVEL_VERBOSE 5

#define MORO_LOG_NO_COLOR 0
#define MORO_LOG_COLORED 1

#ifdef CONFIG_MORO_LOG_LEVEL
#define MORO_LOG_LEVEL CONFIG_MORO_LOG_LEVEL
#else
#define MORO_LOG_LEVEL MORO_LOG_LEVEL_NONE
#endif

#ifdef CONFIG_MORO_LOG_COLOR_LEVEL
#define MORO_LOG_COLOR_LEVEL CONFIG_MORO_LOG_COLOR_LEVEL
#else
#define MORO_LOG_COLOR_LEVEL MORO_LOG_NO_COLOR
#endif

#if MORO_LOG_COLOR_LEVEL >= MORO_LOG_COLORED
#define MORO_LOGFORMATDETAILED(color, logchar, tmstamp, format) color "[%c:%u][%s:%u %s()]:" format "\n" ANSI_COLOR_RESET, logchar, tmstamp, __FILE__, __LINE__, __FUNCTION__

#define MORO_LOGFORMATSIMPLE(color, logchar, tmstamp, format) color "[%c:%u]" format "\n" ANSI_COLOR_RESET, logchar, tmstamp
#else
#define MORO_LOGFORMATDETAILED(color, logchar, tmstamp, format) "[%c:%u][%s:%u %s()]:" format "\n", logchar, tmstamp, __FILE__, __LINE__, __FUNCTION__
#define MORO_LOGFORMATSIMPLE(color, logchar, tmstamp, format) "[%c:%u]" format "\n", logchar, tmstamp
#endif

#if MORO_LOG_LEVEL >= MORO_LOG_LEVEL_ERROR
#define MORO_LOGE(format, ...) moro_log_write(MORO_LOG_LEVEL_ERROR, MORO_LOGFORMATDETAILED(ANSI_COLOR_RED, 'E', esp_log_timestamp(), format), ##__VA_ARGS__)
#else
#define MORO_LOGE(format, ...)
#endif

#if MORO_LOG_LEVEL >= MORO_LOG_LEVEL_WARNING
#define MORO_LOGW(format, ...) moro_log_write(MORO_LOG_LEVEL_WARNING, MORO_LOGFORMATDETAILED(ANSI_COLOR_YELLOW, 'W', esp_log_timestamp(), format), ##__VA_ARGS__)
#else
#define MORO_LOGW(format, ...)
#endif

#if MORO_LOG_LEVEL >= MORO_LOG_LEVEL_INFO
#define MORO_LOGI(format, ...) moro_log_write(MORO_LOG_LEVEL_INFO, MORO_LOGFORMATDETAILED(ANSI_COLOR_GREEN, 'I', esp_log_timestamp(), format), ##__VA_ARGS__)
#else
#define MORO_LOGI(format, ...)
#endif

#if MORO_LOG_LEVEL >= MORO_LOG_LEVEL_DEBUG
#define MORO_LOGD(format, ...) moro_log_write(MORO_LOG_LEVEL_DEBUG, MORO_LOGFORMATDETAILED(ANSI_COLOR_MAGENTA, 'D', esp_log_timestamp(), format), ##__VA_ARGS__)
#else
#define MORO_LOGD(format, ...)
#define _MORO_LOGD(format, ...)
#endif

#if MORO_LOG_LEVEL >= MORO_LOG_LEVEL_VERBOSE
#define MORO_LOGV(format, ...) moro_log_write(MORO_LOG_LEVEL_VERBOSE, MORO_LOGFORMATDETAILED(ANSI_COLOR_WHITE, 'V', esp_log_timestamp(), format), ##__VA_ARGS__)
#else
#define MORO_LOGV(format, ...)
#endif
//=============================================================

// ========================== UTILS ===========================

std::vector<std::string> split(const char* str, char delimiter);

bool wildcard_match(const char* pattern, const char* value);

//=============================================================

#endif /* _MORO_COMMON_H_ */