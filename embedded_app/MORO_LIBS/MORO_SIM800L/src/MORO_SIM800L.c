#include "MORO_SIM800L.h"

// ############################## PRIVATE ##############################
static bool is_inited;

static EventGroupHandle_t event_group = NULL;
static const int CONNECT_BIT		  = BIT0;
static const int GOT_DATA_BIT		  = BIT2;

static gpio_num_t uart_tx;
static gpio_num_t uart_rx;
static uint32_t uart_rx_buffer_size;
static uint32_t uart_tx_buffer_size;
static uint32_t uart_event_queue_size;
static uint32_t uart_event_task_stack_size;
static uint32_t uart_event_task_priority;

static esp_modem_dce_t *dce;

static esp_ip4_addr_t ip;

static void on_ppp_changed_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	ESP_LOGI(MORO_SIM800L_TAG, "PPP state changed event %" PRIu32, event_id);
	if (event_id == NETIF_PPP_ERRORUSER) {
		/* User interrupted event from esp-netif */
		esp_netif_t **p_netif = (esp_netif_t **)event_data;
		ESP_LOGI(MORO_SIM800L_TAG, "User interrupted event from netif:%p", *p_netif);
	}
}

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	ESP_LOGD(MORO_SIM800L_TAG, "IP event! %" PRIu32, event_id);
	if (event_id == IP_EVENT_PPP_GOT_IP) {
		esp_netif_dns_info_t dns_info;

		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		esp_netif_t *netif		 = event->esp_netif;

		ESP_LOGI(MORO_SIM800L_TAG, "Modem Connect to PPP Server");
		ESP_LOGI(MORO_SIM800L_TAG, "~~~~~~~~~~~~~~");
		ESP_LOGI(MORO_SIM800L_TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
		ESP_LOGI(MORO_SIM800L_TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
		ESP_LOGI(MORO_SIM800L_TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));
		esp_netif_get_dns_info(netif, (esp_netif_dns_type_t)0, &dns_info);
		ESP_LOGI(MORO_SIM800L_TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
		esp_netif_get_dns_info(netif, (esp_netif_dns_type_t)1, &dns_info);
		ESP_LOGI(MORO_SIM800L_TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
		ESP_LOGI(MORO_SIM800L_TAG, "~~~~~~~~~~~~~~");

		memcpy(&ip, &event->ip_info.ip, sizeof(esp_ip4_addr_t));

		xEventGroupSetBits(event_group, CONNECT_BIT);

		ESP_LOGI(MORO_SIM800L_TAG, "GOT ip event!!!");
	} else if (event_id == IP_EVENT_PPP_LOST_IP) {
		ESP_LOGI(MORO_SIM800L_TAG, "Modem Disconnect from PPP Server");
	} else if (event_id == IP_EVENT_GOT_IP6) {
		ESP_LOGI(MORO_SIM800L_TAG, "GOT IPv6 event!");

		ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
		ESP_LOGI(MORO_SIM800L_TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
	}
}

// ############################## PUBLIC ##############################

esp_err_t moro_sim800l_init(gpio_num_t inc_uart_tx, gpio_num_t inc_uart_rx, uint32_t inc_uart_rx_buffer_size, uint32_t inc_uart_tx_buffer_size, uint32_t inc_uart_event_queue_size, uint32_t inc_uart_event_task_stack_size, uint32_t inc_uart_event_task_priority) {
	is_inited = false;

	uart_tx					   = inc_uart_tx;
	uart_rx					   = inc_uart_rx;
	uart_rx_buffer_size		   = inc_uart_rx_buffer_size;
	uart_tx_buffer_size		   = inc_uart_tx_buffer_size;
	uart_event_queue_size	   = inc_uart_event_queue_size;
	uart_event_task_stack_size = inc_uart_event_task_stack_size;
	uart_event_task_priority   = inc_uart_event_task_priority;
	esp_err_t ret			   = esp_netif_init();

	if (ret != ESP_OK) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to initialize esp_netif");
		return ret;
	}

	ret = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL);
	if (ret != ESP_OK) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to register IP event handler");
		return ret;
	}

	ret = esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed_event, NULL);
	if (ret != ESP_OK) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to register PPP status event handler");
		return ret;
	}

	/* Configure the PPP netif */
	esp_modem_dce_config_t dce_config	= ESP_MODEM_DCE_DEFAULT_CONFIG("internet");
	esp_netif_config_t netif_ppp_config = ESP_NETIF_DEFAULT_PPP();
	esp_netif_t *esp_netif				= esp_netif_new(&netif_ppp_config);
	if (!esp_netif) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to create PPP netif");
		return ESP_FAIL;
	}

	event_group = xEventGroupCreate();

	esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
	/* setup UART specific configuration based on kconfig options */
	dte_config.uart_config.tx_io_num		= uart_tx;
	dte_config.uart_config.rx_io_num		= uart_rx;
	dte_config.uart_config.flow_control		= ESP_MODEM_FLOW_CONTROL_NONE;
	dte_config.uart_config.rx_buffer_size	= uart_rx_buffer_size;
	dte_config.uart_config.tx_buffer_size	= uart_tx_buffer_size;
	dte_config.uart_config.event_queue_size = uart_event_queue_size;
	dte_config.task_stack_size				= uart_event_task_stack_size;
	dte_config.task_priority				= uart_event_task_priority;
	dte_config.dte_buffer_size				= uart_rx_buffer_size / 2;

	ESP_LOGI(MORO_SIM800L_TAG, "Initializing esp_modem for the SIM800 module...");
	dce = esp_modem_new_dev(ESP_MODEM_DCE_SIM800, &dte_config, &dce_config, esp_netif);
	if (!dce) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to create modem device");
		return ESP_FAIL;
	}

	ESP_LOGI(MORO_SIM800L_TAG, "SIM800L initialized successfully");
	is_inited = true;
	return ret;
}

esp_err_t moro_sim800l_get_signal_quality(int *rssi, int *ber) {
	if (!is_inited) {
		ESP_LOGE(MORO_SIM800L_TAG, "SIM800L module is not initialized");
		return ESP_FAIL;
	}

	int rssi_val, ber_val;
	esp_err_t err = esp_modem_get_signal_quality(dce, &rssi_val, &ber_val);
	if (err != ESP_OK) {
		ESP_LOGE(MORO_SIM800L_TAG, "esp_modem_get_signal_quality failed with %d %s", err, esp_err_to_name(err));
		return err;
	}

	memcpy(rssi, &rssi_val, sizeof(int));
	memcpy(ber, &ber_val, sizeof(int));

	ESP_LOGI(MORO_SIM800L_TAG, "Signal quality: rssi=%d, ber=%d", rssi_val, ber_val);
	return ESP_OK;
}

esp_err_t moro_sim800l_set_data_mode() {
	if (!is_inited) {
		ESP_LOGE(MORO_SIM800L_TAG, "SIM800L module is not initialized");
		return ESP_FAIL;
	}

	xEventGroupClearBits(event_group, CONNECT_BIT | GOT_DATA_BIT);

	esp_err_t ret = esp_modem_set_mode(dce, ESP_MODEM_MODE_DATA);
	if (ret != ESP_OK) {
		ESP_LOGE(MORO_SIM800L_TAG, "esp_modem_set_mode(ESP_MODEM_MODE_DATA) failed with %d", ret);
		return ret;
	}
	/* Wait for IP address */
	ESP_LOGI(MORO_SIM800L_TAG, "Waiting for IP address");
	xEventGroupWaitBits(event_group, CONNECT_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(30 * 1000));

	if ((CONNECT_BIT) & (!xEventGroupGetBits(event_group))) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to get IP address");
		return ESP_FAIL;
	}

	return ESP_OK;
}

void moro_sim800l_get_connected_ip(esp_ip4_addr_t *inc_ip) {
	// Copy the connected IP address to the provided pointer
	if (!is_inited) {
		ESP_LOGE(MORO_SIM800L_TAG, "SIM800L module is not initialized");
		return;
	}

	memcpy(inc_ip, &ip, sizeof(esp_ip4_addr_t));
}

esp_err_t moro_sim800l_send_sms(const char *phone_number, const char *message) {
	if (!is_inited) {
		ESP_LOGE(MORO_SIM800L_TAG, "SIM800L module is not initialized");
		return ESP_FAIL;
	}

	if (esp_modem_sms_txt_mode(dce, true) != ESP_OK || esp_modem_sms_character_set(dce) != ESP_OK) {
		ESP_LOGE(MORO_SIM800L_TAG, "Setting text mode or GSM character set failed");
		return ESP_FAIL;
	}

	esp_err_t err = esp_modem_send_sms(dce, phone_number, message);
	if (err != ESP_OK) {
		ESP_LOGE(MORO_SIM800L_TAG, "esp_modem_send_sms() failed with %d", err);
		return err;
	}

	return ESP_OK;
}

esp_err_t moro_sim800l_get_location(float *latitude, float *longitude, char *date_time) {
	if (!is_inited) {
		ESP_LOGE(MORO_SIM800L_TAG, "SIM800L module is not initialized");
		return ESP_FAIL;
	}

	esp_err_t ret = esp_modem_set_mode(dce, ESP_MODEM_MODE_COMMAND);
	if (ret != ESP_OK) {
		ESP_LOGE(MORO_SIM800L_TAG, "esp_modem_set_mode(ESP_MODEM_MODE_COMMAND) failed with %d", ret);
		return ret;
	}

	char response[1024];
	memset(response, 0, sizeof(response));
	ret = esp_modem_at(dce, "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", response, 5000);
	ESP_LOGI(MORO_SIM800L_TAG, "SAPBR 3,1 contype response: %s", response);

	memset(response, 0, sizeof(response));
	ret = esp_modem_at(dce, "AT+SAPBR=3,1,\"APN\",\"internet\"", response, 5000);
	ESP_LOGI(MORO_SIM800L_TAG, "SAPBR 3,1 apn response: %s", response);

	memset(response, 0, sizeof(response));
	ret = esp_modem_at(dce, "AT+SAPBR=1,1", response, 5000);
	ESP_LOGI(MORO_SIM800L_TAG, "SAPBR 1,1 response: %s", response);

	memset(response, 0, sizeof(response));
	ret = esp_modem_at(dce, "AT+SAPBR=2,1", response, 5000);
	ESP_LOGI(MORO_SIM800L_TAG, "SAPBR 2,1 response: %s", response);

	memset(response, 0, sizeof(response));
	ret = esp_modem_at(dce, "AT+CLBS=4,1", response, 5000);
	if (ret != ESP_OK) {
		ESP_LOGE(MORO_SIM800L_TAG, "esp_modem_at(AT+CLBS=1,1) failed with %d", ret);
		return ret;
	}

	ESP_LOGI(MORO_SIM800L_TAG, "Location response: %s", response);

	//  response like  0,28.985866,41.068093,550,24/07/02,02:19:34 we need to parse this
	//  0 is the status, 28.985866 is the latitude, 41.068093 is the longitude, 550 is the altitude, 24/07/02 is the date, 02:19:34 is the time
	char *token = strtok(response, ",");
	if (token == NULL) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to parse location response");
		return ESP_FAIL;
	}

	token = strtok(NULL, ",");
	if (token == NULL) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to parse latitude");
		return ESP_FAIL;
	}
	*longitude = atof(token);

	token = strtok(NULL, ",");
	if (token == NULL) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to parse longitude");
		return ESP_FAIL;
	}
	*latitude = atof(token);

	token = strtok(NULL, ",");
	if (token == NULL) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to parse altitude");
		return ESP_FAIL;
	}

	token = strtok(NULL, ",");
	if (token == NULL) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to parse altitude");
		return ESP_FAIL;
	}
	char date_str[32] = {0};
	strcpy(date_str, token);

	token = strtok(NULL, ",");
	if (token == NULL) {
		ESP_LOGE(MORO_SIM800L_TAG, "Failed to parse date");
		return ESP_FAIL;
	}
	char time_str[32] = {0};
	strcpy(time_str, token);

	sprintf(date_time, "%s,%s", date_str, time_str);

	return ESP_OK;
}