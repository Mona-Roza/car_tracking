#include "MORO_COMMON.h"

#define MAX_NUMBER_OF_MORO_PRINTF_SOCKETS 3
#define _semaphore_wait_tick (__pdMS_TO_TICKS(120 * 1000))

volatile static int number_of_printf_sockets = 1;  /// include vprintf
volatile static uint8_t dynamic_log_macro	 = MORO_LOG_LEVEL;

// We do not have a initializer, so initialize here
static SemaphoreHandle_t moro_log_mutex = NULL;

// volatile static printf_function_t moro_printf_ptr = &vprintf;

volatile static printf_function_t moro_printf_ptr_list[MAX_NUMBER_OF_MORO_PRINTF_SOCKETS] = {
	&vprintf,
};

// ############################## PRIVATE FUNCTIONS ##############################

static bool lock_moro_log() {
	if (moro_log_mutex == NULL) {
		moro_log_mutex = xSemaphoreCreateMutex();
		if (moro_log_mutex == NULL) return false;
	}
	return xSemaphoreTake(moro_log_mutex, pdMS_TO_TICKS(60 * 1000)) == pdPASS;
}

static void unlock_moro_log() {
	xSemaphoreGive(moro_log_mutex);
}

static int moro_printf_internal(const char* format, va_list args) {
	int ret = -1;
	if (lock_moro_log()) {
		for (uint8_t i = 0; i < number_of_printf_sockets; ++i) {
			printf_function_t moro_printf_ptr = moro_printf_ptr_list[i];
			ret								  = (*moro_printf_ptr)(format, args);
		}
		unlock_moro_log();
	}
	return ret;
}

// ############################## PUBLIC FUNCTIONS ##############################

//========================== MORO_LOG ==========================
int moro_printf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	int ret = moro_printf_internal(format, args);
	va_end(args);
	return ret;
}

int moro_log_write(uint8_t log_level_macro, const char* format, ...) {
	if (dynamic_log_macro >= log_level_macro) {
		va_list args;
		va_start(args, format);
		int ret = moro_printf_internal(format, args);
		va_end(args);
		return ret;
	}
	return -0x8;
}

void moro_printf_list(const uint8_t* c, int len, const char* format, const char* start, const char* sep, const char* end) {
	moro_printf("%s", start);
	while (len--) {
		moro_printf(format, *c++);
		if (len) {
			moro_printf("%s", sep);
		}
	}
	moro_printf("%s", end);
}

// UNIMPLEMENTED
void moro_log_set_level(uint8_t log_level_macro) {
	return;
	// Disable for bug, might be fixed in future
	// if(lock_moro_log()){
	// 	if(log_level_macro < MORO_LOG_LEVEL_NONE) dynamic_log_macro = MORO_LOG_LEVEL_NONE;
	// 	else if(log_level_macro > MORO_LOG_LEVEL_VERBOSE) dynamic_log_macro = MORO_LOG_LEVEL_VERBOSE;
	// 	else dynamic_log_macro = log_level_macro;
	// 	unlock_moro_log();
	// }

	if (!lock_moro_log()) {
		return;
	}

	if (log_level_macro < MORO_LOG_LEVEL_NONE)
		dynamic_log_macro = MORO_LOG_LEVEL_NONE;
	else if (log_level_macro > MORO_LOG_LEVEL_VERBOSE)
		dynamic_log_macro = MORO_LOG_LEVEL_VERBOSE;
	else
		dynamic_log_macro = log_level_macro;

	unlock_moro_log();
}

uint8_t moro_log_get_level() {
	uint8_t log_level_macro = MORO_LOG_LEVEL_NONE;
	if (lock_moro_log()) {
		log_level_macro = dynamic_log_macro;
		unlock_moro_log();
	}
	return log_level_macro;
}

void forward_esp_log_to_moro_printf() {
	esp_log_set_vprintf(moro_printf_internal);
}

/*
	Closes printf, only given ptr is available!
*/
printf_function_t change_printf_function(printf_function_t ptr) {
	if (ptr == NULL) return NULL;

	printf_function_t old_ptr = NULL;
	if (lock_moro_log()) {
		old_ptr					= moro_printf_ptr_list[0];
		moro_printf_ptr_list[0] = ptr;
		unlock_moro_log();
	}
	return old_ptr;
}

void reset_printf_function() {
	if (lock_moro_log()) {
		moro_printf_ptr_list[0] = &vprintf;
		unlock_moro_log();
	}
}

/*
	Adds another socket as printf and they work simultaneously
*/
bool add_printf_function(printf_function_t ptr) {
	if (number_of_printf_sockets > MAX_NUMBER_OF_MORO_PRINTF_SOCKETS || ptr == NULL) return false;

	if (lock_moro_log()) {
		moro_printf_ptr_list[number_of_printf_sockets++] = ptr;
		unlock_moro_log();
	}
	return true;
}

bool remove_printf_function(printf_function_t ptr) {
	// at least one field is required
	if (number_of_printf_sockets == 1 || ptr == NULL) return false;

	if (lock_moro_log()) {
		// start from 1, try to change additionals not the first
		for (uint8_t i = 1; i < number_of_printf_sockets; ++i) {
			if (ptr == moro_printf_ptr_list[i]) {
				moro_printf_ptr_list[i] = NULL;
				number_of_printf_sockets--;
			}
		}

		// Now swap if necessary
		for (int i = 0; i < MAX_NUMBER_OF_MORO_PRINTF_SOCKETS; ++i) {
			printf_function_t temp;
			if (i != MAX_NUMBER_OF_MORO_PRINTF_SOCKETS - 1 && moro_printf_ptr_list[i] == NULL && moro_printf_ptr_list[i + 1] != NULL) {
				// Swap
				moro_printf_ptr_list[i]		= moro_printf_ptr_list[i + 1];
				moro_printf_ptr_list[i + 1] = NULL;
			}
		}
	}
	unlock_moro_log();
	return true;
}

void reset_esp_log_printf() {
	esp_log_set_vprintf(vprintf);
}

//=============================================================

// ========================== UTILS ===========================

std::vector<std::string> split(const char* str, char delimiter) {
	std::vector<std::string> result;
	do {
		const char* begin = str;
		while (*str != delimiter && *str)
			str++;
		result.push_back(std::string(begin, str));
	} while (0 != *str++);
	return result;
}

bool wildcard_match(const char* pattern, const char* value) {
	if (*pattern == '\0' && *value == '\0') return true;
	if (*pattern == '#') {
		if (*(pattern + 1) != '\0' && *value == '\0') return false;
		return wildcard_match(pattern + 1, value) || wildcard_match(pattern, value + 1);
	} else if (*pattern == '+') {
		while (*value != '\0' && *value != '/') {
			if (wildcard_match(pattern + 1, value + 1)) return true;
			value++;
		}
		return false;
	} else if (*pattern == *value)
		return wildcard_match(pattern + 1, value + 1);
	return false;
}

// ============================================================