
#include "MORO_COMMON.h"
#include "MORO_MQTT.h"
#include "MORO_SIM800L.h"

#define UART_RX_PIN GPIO_NUM_23
#define UART_TX_PIN GPIO_NUM_22

#define UART_RX_BUFFER_SIZE 1024
#define UART_TX_BUFFER_SIZE 512

#define UART_EVENT_QUEUE_SIZE 30
#define UART_EVENT_TASK_STACK_SIZE 2048
#define UART_EVENT_TASK_PRIORITY 5

#define SPIFFS_BASE_PATH "/files"
#define SPIFFS_PARTITION_LABEL "files"

#define MQTT_CONFIGURATIONS_FILE_PATH "/files/mqtt_configurations.json"

// define TAG for logging
static const char* MAIN_TAG = "main";

mqtt_configurations_t mqtt_configurations;

#define MORO_MQTT_BASE_TOPIC "monaroza"

moro_mac_t moro_mac;

void prepare_mqtt_configurations() {
	char last_will_topic[128] = {0};

	//--------------------------- MQTT Configurations --------------------------
	/*
	 * mqtt_configurations.json file content:
	 *   {
	 *        "endpoint":     " ",
	 *        "auth_type":    "certificate",
	 *        "username":     " ",
	 *        "password":     " ",
	 *        "ca_cert":      null,
	 *        "ca_cert_size": 0,
	 *        "client_cert":  null,
	 *        "client_cert_size":     0,
	 *        "client_key":   null,
	 *        "client_key_size":      0,
	 *        "device_mac":   "B4:8A:0A:62:A7:44",
	 *        "last_will_topic":      "platform/B4:8A:0A:62:A7:44/last_will_topic",
	 *   }
	 */
	const char* mqtt_configurations_file_path = MQTT_CONFIGURATIONS_FILE_PATH;
	struct stat mqtt_file_stats;

	if (stat(mqtt_configurations_file_path, &mqtt_file_stats) == 0) {
		ESP_LOGI(MAIN_TAG, "mqtt_configurations.json file found.");

		// read the file content
		int fd = open(mqtt_configurations_file_path, O_RDONLY);
		moro_abort_if_true((fd < 0), "Failed to open mqtt_configurations.json file. [%d] Aborting.", fd);

		char* buffer = (char*)malloc(mqtt_file_stats.st_size);
		read(fd, buffer, mqtt_file_stats.st_size);

		// parse the file content
		cJSON* mqtt_configurations_json = cJSON_Parse(buffer);

		sprintf(mqtt_configurations.endpoint, "%s", cJSON_GetObjectItem(mqtt_configurations_json, "endpoint")->valuestring);

		sprintf(mqtt_configurations.auth_type, "%s", cJSON_GetObjectItem(mqtt_configurations_json, "auth_type")->valuestring);

		sprintf(mqtt_configurations.username, "%s", cJSON_GetObjectItem(mqtt_configurations_json, "username")->valuestring);

		sprintf(mqtt_configurations.password, "%s", cJSON_GetObjectItem(mqtt_configurations_json, "password")->valuestring);

		mqtt_configurations.ca_cert_size = cJSON_GetObjectItem(mqtt_configurations_json, "ca_cert_size")->valueint;

		if (mqtt_configurations.ca_cert_size > 0) {
			mqtt_configurations.ca_cert = (char*)malloc(mqtt_configurations.ca_cert_size);
			sprintf(mqtt_configurations.ca_cert, "%s", cJSON_GetObjectItem(mqtt_configurations_json, "ca_cert")->valuestring);
		} else {
			mqtt_configurations.ca_cert = NULL;
		}

		mqtt_configurations.client_cert_size = cJSON_GetObjectItem(mqtt_configurations_json, "client_cert_size")->valueint;

		if (mqtt_configurations.client_cert_size > 0) {
			mqtt_configurations.client_cert = (char*)malloc(mqtt_configurations.client_cert_size);
			sprintf(mqtt_configurations.client_cert, "%s", cJSON_GetObjectItem(mqtt_configurations_json, "client_cert")->valuestring);
		} else {
			mqtt_configurations.client_cert = NULL;
		}

		mqtt_configurations.client_key_size = cJSON_GetObjectItem(mqtt_configurations_json, "client_key_size")->valueint;

		if (mqtt_configurations.client_key_size > 0) {
			mqtt_configurations.client_key = (char*)malloc(mqtt_configurations.client_key_size);
			sprintf(mqtt_configurations.client_key, "%s", cJSON_GetObjectItem(mqtt_configurations_json, "client_key")->valuestring);
		} else {
			mqtt_configurations.client_key = NULL;
		}

		sprintf(mqtt_configurations.last_will_topic, "%s", cJSON_GetObjectItem(mqtt_configurations_json, "last_will_topic")->valuestring);

		sprintf(mqtt_configurations.device_mac, "%s", moro_mac.mac_str);

		// ESP_LOGI(MAIN_TAG, "MQTT Configurations: [%s]", cJSON_Print(mqtt_configurations_json));

		free(buffer);
		cJSON_Delete(mqtt_configurations_json);
		close(fd);
	} else {
		ESP_LOGI(MAIN_TAG, "mqtt_configurations.json file didn't found. Preparing...");

		cJSON* mqtt_configurations_json = cJSON_CreateObject();

		cJSON_AddStringToObject(mqtt_configurations_json, "endpoint", "mqtt://46.2.23.249:1883");
		memset(mqtt_configurations.endpoint, 0, sizeof(mqtt_configurations.endpoint));

		cJSON_AddStringToObject(mqtt_configurations_json, "auth_type", "none");
		sprintf(mqtt_configurations.auth_type, "none");

		cJSON_AddStringToObject(mqtt_configurations_json, "username", "");
		memset(mqtt_configurations.username, 0, sizeof(mqtt_configurations.username));

		cJSON_AddStringToObject(mqtt_configurations_json, "password", "");
		memset(mqtt_configurations.password, 0, sizeof(mqtt_configurations.password));

		cJSON_AddNullToObject(mqtt_configurations_json, "ca_cert");
		mqtt_configurations.ca_cert = NULL;

		cJSON_AddNumberToObject(mqtt_configurations_json, "ca_cert_size", 0);
		mqtt_configurations.ca_cert_size = 0;

		cJSON_AddNullToObject(mqtt_configurations_json, "client_cert");
		mqtt_configurations.client_cert = NULL;

		cJSON_AddNumberToObject(mqtt_configurations_json, "client_cert_size", 0);
		mqtt_configurations.client_cert_size = 0;

		cJSON_AddNullToObject(mqtt_configurations_json, "client_key");
		mqtt_configurations.client_key = NULL;

		cJSON_AddNumberToObject(mqtt_configurations_json, "client_key_size", 0);
		mqtt_configurations.client_key_size = 0;

		sprintf(mqtt_configurations.device_mac, "%s", moro_mac.mac_str);
		cJSON_AddStringToObject(mqtt_configurations_json, "device_mac", moro_mac.mac_str);

		sprintf(last_will_topic, "%s/%s/die", MORO_MQTT_BASE_TOPIC, mqtt_configurations.device_mac);

		cJSON_AddStringToObject(mqtt_configurations_json, "last_will_topic", last_will_topic);
		sprintf(mqtt_configurations.last_will_topic, last_will_topic);

		// create the file
		int fd = open(mqtt_configurations_file_path, O_EXCL | O_CREAT);
		moro_abort_if_true((fd < 0), "Failed to create mqtt_configurations.json file. Aborting.");
		close(fd);

		// ESP_LOGI(MAIN_TAG, "MQTT Configurations: [%s]", cJSON_Print(mqtt_configurations_json));

		fd = open(mqtt_configurations_file_path, O_RDWR);
		// write the file content
		char* mqtt_configurations_json_string = cJSON_Print(mqtt_configurations_json);

		moro_abort_if_true((write(fd, mqtt_configurations_json_string, strlen(mqtt_configurations_json_string)) < 0), "Failed to write mqtt_configurations.json file. Aborting.");

		cJSON_Delete(mqtt_configurations_json);
		free(mqtt_configurations_json_string);
		close(fd);
	}
}

void app_main(void) {
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_LOGI(MAIN_TAG, "Initializing SPIFFS");
	ESP_ERROR_CHECK(init_spiffs(SPIFFS_BASE_PATH, SPIFFS_PARTITION_LABEL));

	ESP_LOGI(MAIN_TAG, "Preparing MAC address");
	ESP_ERROR_CHECK(read_mac(&moro_mac));

	prepare_mqtt_configurations();

	list_all_files(SPIFFS_BASE_PATH);

	esp_err_t ret = moro_sim800l_init(UART_TX_PIN, UART_RX_PIN, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, UART_EVENT_QUEUE_SIZE, UART_EVENT_TASK_STACK_SIZE, UART_EVENT_TASK_PRIORITY);
	if (ret != ESP_OK) {
		ESP_LOGE(MAIN_TAG, "Failed to initialize SIM800L module");
		return;
	}

	ret = moro_sim800l_set_data_mode();
	if (ret != ESP_OK) {
		ESP_LOGE(MAIN_TAG, "Failed to set data mode");
		return;
	}

	esp_ip4_addr_t ip;
	moro_sim800l_get_connected_ip(&ip);
	ESP_LOGI(MAIN_TAG, "Connected IP: " IPSTR, IP2STR(&ip));

	vTaskDelay(pdMS_TO_TICKS(1000));

	ret = moro_mqtt_init(&mqtt_configurations);
	if (ret != ESP_OK) {
		ESP_LOGE(MAIN_TAG, "Failed to initialize MQTT");
		return;
	}

	ret = moro_mqtt_publish("test", "Hello World", 0, 0, false);
}
