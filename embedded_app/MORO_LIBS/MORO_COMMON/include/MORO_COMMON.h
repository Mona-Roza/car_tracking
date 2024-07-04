#ifndef _MORO_COMMON_H_
#define _MORO_COMMON_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "dirent.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_rom_efuse.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"  // Include the semaphore definitions
#include "freertos/task.h"	  // Include the task definitions
#include "sdkconfig.h"
#ifdef __cplusplus
extern "C" {
#endif

// ================= UTILS =================

bool wildcard_match(const char *pattern, const char *value);

void url_encoder(char *buf, const char *input);

#define moro_abort_if_true(x, reason_str, ...)                      \
	if (x) {                                                        \
		ESP_LOGE("MORO_COMMON", "%s <<<<<%s>>>>>", #x, reason_str); \
		assert(!x);                                                 \
	}

// =========================================

esp_err_t init_spiffs(const char *base_path, const char *partition_label);

void list_all_files(const char *file_path);

typedef struct {
	uint8_t mac[6];
	char mac_str[18];

} moro_mac_t;

esp_err_t read_mac(moro_mac_t *mac);

#ifdef __cplusplus
}
#endif

#endif	// _MORO_COMMON_H_