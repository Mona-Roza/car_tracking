#ifndef _MORO_SIM800L_H_
#define _MORO_SIM800L_H_

#include "MORO_COMMON.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"

class MORO_SIM800L {
   public:
	static esp_err_t init(gpio_num_t tx, gpio_num_t rx, int buff_size, int baud_rate, char *pin = NULL);

	static esp_err_t deinit();

	// Change uint8_t to uint64_t
	static esp_err_t register_sim_card(const char *pin, uint8_t timeout_ms = 1000, uint8_t retry = 1);

	static esp_err_t send_sms(const char *phone_number, const char *message, const char *callback = NULL, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t read_sms(uint8_t index, char *phone_number, char *message, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t delete_sms(uint8_t index, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t call(const char *phone_number, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t hang_up(uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t get_signal_quality(uint8_t *rssi, uint8_t *ber, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t get_operator_name(char *operator_name, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t get_battery_voltage(uint16_t *voltage, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t get_location(char *latitude, char *longitude, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t get_time(char *time, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t get_network_status(uint8_t *status, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t get_network_time_zone(char *time_zone, uint8_t timeout_ms = 0, uint8_t retry = 1);

	static esp_err_t get_network_time(char *time, uint8_t timeout_ms = 0, uint8_t retry = 1);

   private:
	static esp_err_t send_command(const char *command, const char *callback, int timeout_ms = 0, uint8_t retry = 1, char *response = NULL, uint8_t response_size = 0, uint8_t *response_len = NULL, uint8_t *response_index = NULL);
};

extern MORO_SIM800L *MORO_SIM800L;

#endif /* _MORO_SIM800L_H_ */