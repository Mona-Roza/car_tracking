#include "MORO_SIM800L.h"

#define BUF_SIZE (1024)
// ############################## PRIVATE ##############################

esp_err_t MORO_SIM800L::send_command(const char *command, const char *callback, int timeout_ms, uint8_t retry, char *response, uint8_t response_size, uint8_t *response_len, uint8_t *response_index) {
	// We need to send command with \r\n
	char cmd[32];
	sprintf(cmd, "%s\r\n", command);

	// Send command
	int ret = uart_write_bytes(UART_NUM_1, cmd, strlen(cmd));

	if (ret < 0) {
		MORO_LOGE("UART write failed");
		vTaskDelay(pdMS_TO_TICKS(100));	 // wait for a while
		return ESP_FAIL;
	}

	// Wait for response
	char *data = (char *)malloc(BUF_SIZE);
	if (data == NULL) {
		MORO_LOGE("Memory allocation failed");
		vTaskDelay(pdMS_TO_TICKS(100));	 // wait for a while
		return ESP_FAIL;
	}

	memset(data, 0, BUF_SIZE);

	esp_err_t err = uart_wait_tx_done(UART_NUM_1, timeout_ms);
	if (err != ESP_OK) {
		MORO_LOGE("UART wait tx done failed");
		free(data);
		vTaskDelay(pdMS_TO_TICKS(100));	 // wait for a while
		return err;
	}

	// read until timeout
	int len = uart_read_bytes(UART_NUM_1, (uint8_t *)data, BUF_SIZE, pdMS_TO_TICKS(timeout_ms));
	if (len < 0) {
		MORO_LOGE("UART read failed");
		free(data);
		vTaskDelay(pdMS_TO_TICKS(100));	 // wait for a while
		return ESP_FAIL;
	}

	// check if response is expected
	if (callback != NULL) {
		if (strstr(data, callback) == NULL) {
			MORO_LOGE("Response not found");
			free(data);
			vTaskDelay(pdMS_TO_TICKS(100));	 // wait for a while
			return ESP_FAIL;
		}
	}

	// copy response
	if (response != NULL) {
		if (response_len != NULL) {
			*response_len = len;
		}

		if (response_index != NULL) {
			*response_index = 0;
		}

		if (response_size > 0) {
			if (len > response_size) {
				len = response_size;
			}
		}

		memcpy(response, data, len);
	}

	free(data);
	vTaskDelay(pdMS_TO_TICKS(100));	 // wait for a while
	return ESP_OK;
}

// ############################## PUBLIC ##############################

esp_err_t MORO_SIM800L::init(gpio_num_t tx, gpio_num_t rx, int buff_size, int baud_rate, char *pin) {
	// =========== initialize UART ===========
	uart_config_t uart_config = {
		.baud_rate			 = baud_rate,
		.data_bits			 = UART_DATA_8_BITS,
		.parity				 = UART_PARITY_DISABLE,
		.stop_bits			 = UART_STOP_BITS_1,
		.flow_ctrl			 = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 0,
	};

	esp_err_t err = uart_param_config(UART_NUM_1, &uart_config);

	if (err != ESP_OK) {
		MORO_LOGE("UART config failed");
		return err;
	}

	err = uart_set_pin(UART_NUM_1, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	if (err != ESP_OK) {
		MORO_LOGE("UART pin config failed");
		return err;
	}

	err = uart_driver_install(UART_NUM_1, buff_size, buff_size, 0, NULL, 0);

	if (err != ESP_OK) {
		MORO_LOGE("UART driver install failed");
		return err;
	}

	// =========== initialize SIM800L ===========
	char response[32];

	// ATI command for finding out module info
	memset(response, 0, sizeof(response));
	esp_err_t ret = send_command("AT", "OK", 2000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return ret;
	}
	MORO_LOGI("SIM800L AT response: %s", response);

	// AT+CPIN? command for finding out sim card pin registration
	memset(response, 0, sizeof(response));
	ret = send_command("AT+CPIN?", "OK", 2000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return ret;
	}
	MORO_LOGI("SIM800L AT+CPIN? response: %s", response);
	// if sim card needs pin then set pin
	if (strstr(response, "+CPIN: SIM PIN") != NULL) {
		if (pin == NULL) {
			MORO_LOGE("SIM800L needs PIN");
			return ESP_FAIL;
		}
		// AT+CPIN=pin command for setting sim card pin
		memset(response, 0, sizeof(response));
		char command[32];
		sprintf(command, "AT+CPIN=%s", pin);
		ret = send_command(command, "OK", 2000, 2, response, sizeof(response));
		if (ret != ESP_OK) {
			MORO_LOGE("SIM800L init failed");
			return ret;
		}
		MORO_LOGI("SIM800L AT+CPIN=pin response: %s", response);
		if (strstr(response, "OK") == NULL) {
			MORO_LOGE("SIM800L init failed");
			return ESP_FAIL;
		}
	}

	// AT+CFUN? command for finding out module functionality
	memset(response, 0, sizeof(response));
	ret = send_command("AT+CFUN?", "OK", 2000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return ret;
	}
	MORO_LOGI("SIM800L AT+CFUN? response: %s", response);

	// AT+CSQ command for finding out signal quality
	memset(response, 0, sizeof(response));
	ret = send_command("AT+CSQ", "OK", 2000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return ret;
	}
	MORO_LOGI("SIM800L AT+CSQ response: %s", response);

	//  AT+COPS=? command for finding out available operators
	memset(response, 0, sizeof(response));
	ret = send_command("AT+COPS=?", "OK", (1000 * 60 * 3), 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return ret;
	}
	MORO_LOGI("SIM800L AT+COPS=? response: %s", response);

	// AT+CGDCONT? command for finding out PDP context
	memset(response, 0, sizeof(response));
	ret = send_command("AT+CGDCONT?", "OK", 2000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return ret;
	}
	MORO_LOGI("SIM800L AT+CGDCONT? response: %s", response);

	// AT+CGACT? command for finding out PDP context activation
	memset(response, 0, sizeof(response));
	ret = send_command("AT+CGACT?", "OK", 2000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return ret;
	}
	MORO_LOGI("SIM800L AT+CGACT? response: %s", response);

	// AT+CREG=2 command for setting network registration
	memset(response, 0, sizeof(response));
	ret = send_command("AT+CREG=2", "OK", 2000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return ret;
	}
	MORO_LOGI("SIM800L AT+CREG=2 response: %s", response);

	// AT&W command for saving settings
	memset(response, 0, sizeof(response));
	ret = send_command("AT&W", "OK", 2000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return ret;
	}
	MORO_LOGI("SIM800L AT&W response: %s", response);

	// Wait for network registration until registered
	while (strstr(response, "+CREG: 2,1") == NULL) {
		// AT+CREG? command for finding out network registration status
		memset(response, 0, sizeof(response));
		ret = send_command("AT+CREG?", "OK", 2000, 2, response, sizeof(response));
		MORO_LOGV("SIM800L AT+CREG? response: %s", response);

		vTaskDelay(pdMS_TO_TICKS(2000));
	}

	MORO_LOGI("SIM800L init success");

	return ESP_OK;
}

esp_err_t MORO_SIM800L::deinit() {
	esp_err_t err = uart_driver_delete(UART_NUM_1);

	if (err != ESP_OK) {
		MORO_LOGE("UART driver delete failed");
		return err;
	}

	return ESP_OK;
}

esp_err_t MORO_SIM800L::get_signal_quality(uint8_t *rssi, uint8_t *ber, uint8_t timeout_ms, uint8_t retry) {
	char response[32];

	memset(response, 0, sizeof(response));
	esp_err_t ret = send_command("AT+CMGF=1", "", 4000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L set text mode failed");
		return ret;
	}
	MORO_LOGI("SIM800L set text mode response: %s", response);

	memset(response, 0, sizeof(response));
	ret = send_command("AT+CSQ", "OK", 2000, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L get signal quality failed");
		return ret;
	}

	char *token = strtok(response, " ,");
	token		= strtok(NULL, " ,");
	*rssi		= atoi(token);

	token = strtok(NULL, " ,");
	*ber  = atoi(token);

	MORO_LOGI("SIM800L signal quality: %d, %d", *rssi, *ber);
	return ESP_OK;
}

esp_err_t MORO_SIM800L::send_sms(const char *phone_number, const char *message, const char *callback, uint8_t timeout_ms, uint8_t retry) {
	char response[32];

	// set text mode
	memset(response, 0, sizeof(response));
	esp_err_t ret = send_command("AT+CMGF=1", "OK", timeout_ms, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L set text mode failed");
		return ret;
	}
	MORO_LOGI("SIM800L set text mode response: %s", response);

	memset(response, 0, sizeof(response));
	ret = send_command("AT+CMGS=?", "OK", timeout_ms, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L Test SMS failed");
		return ret;
	}
	MORO_LOGI("SIM800L Test SMS response: %s", response);

	// set phone number
	memset(response, 0, sizeof(response));
	char command[32];
	sprintf(command, "AT+CMGS=\"%s\"", phone_number);
	ret = send_command(command, ">", timeout_ms, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L set phone number failed");
		return ret;
	}
	MORO_LOGI("SIM800L set phone number response: %s", response);

	// set message
	memset(response, 0, sizeof(response));
	sprintf(command, "%s", message);  // 0x1A is CTRL+Z
	ret = send_command(command, ">", timeout_ms, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L set message failed");
		return ret;
	}
	MORO_LOGI("SIM800L set message response: %s", response);

	// for sending message we need to send CTRL+Z
	memset(response, 0, sizeof(response));
	ret = send_command("26", "OK", timeout_ms, 2, response, sizeof(response));
	if (ret != ESP_OK) {
		MORO_LOGE("SIM800L send message failed");
		return ret;
	}
	MORO_LOGI("SIM800L send message response: %s", response);

	return ESP_OK;
}