
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

// define TAG for logging
static const char* MAIN_TAG = "main";

mqtt_configurations_t mqtt_configurations;

#define MORO_MQTT_BASE_TOPIC "monaroza"

moro_mac_t moro_mac;
/*
 EMBED_TXTFILES
						"../embedded_certs/ca.pem"
						"../embedded_certs/cert.crt"
						"../embedded_certs/private.key"
*/

extern const uint8_t ca_pem_start[] asm("_binary_ca_pem_start");
extern const uint8_t ca_pem_end[] asm("_binary_ca_pem_end");

extern const uint8_t cert_crt_start[] asm("_binary_cert_crt_start");
extern const uint8_t cert_crt_end[] asm("_binary_cert_crt_end");

extern const uint8_t private_key_start[] asm("_binary_private_key_start");
extern const uint8_t private_key_end[] asm("_binary_private_key_end");

void prepare_mqtt_configurations() {
	//--------------------------- MQTT Configurations --------------------------
	/*
		mqtt_configurations
		typedef struct {
			char endpoint[128];
			char auth_type[25];	 // can be basic, certificate or basic_and_certificate
			char username[128];
			char password[128];

			char* ca_cert;
			size_t ca_cert_size;

			char* client_cert;
			size_t client_cert_size;

			char* client_key;
			size_t client_key_size;

			char device_mac[18];
			char last_will_topic[128];
		} mqtt_configurations_t;
	 */

	strcpy(mqtt_configurations.endpoint, "mqtts://an726pjx0w8v9-ats.iot.eu-north-1.amazonaws.com:8883");
	strcpy(mqtt_configurations.auth_type, MQTT_AUTH_TYPE_CERTIFICATE);

	// set ca certificate
	mqtt_configurations.ca_cert		 = (char*)ca_pem_start;
	mqtt_configurations.ca_cert_size = ca_pem_end - ca_pem_start;

	// set client certificate
	mqtt_configurations.client_cert		 = (char*)cert_crt_start;
	mqtt_configurations.client_cert_size = cert_crt_end - cert_crt_start;

	// set client key
	mqtt_configurations.client_key		= (char*)private_key_start;
	mqtt_configurations.client_key_size = private_key_end - private_key_start;

	strcpy(mqtt_configurations.device_mac, moro_mac.mac_str);

	//--------------------------------------------------------------------------
}

esp_err_t cb(const char* topic, const char* payload, size_t payload_len) {
	ESP_LOGI(MAIN_TAG, "Received message on topic: %s, payload: %s", topic, payload);
	return ESP_OK;
}

void app_main(void) {
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_LOGI(MAIN_TAG, "Preparing MAC address");
	ESP_ERROR_CHECK(read_mac(&moro_mac));

	prepare_mqtt_configurations();

	esp_err_t ret = moro_sim800l_init(UART_TX_PIN, UART_RX_PIN, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, UART_EVENT_QUEUE_SIZE, UART_EVENT_TASK_STACK_SIZE, UART_EVENT_TASK_PRIORITY);
	if (ret != ESP_OK) {
		ESP_LOGE(MAIN_TAG, "Failed to initialize SIM800L module");
		return;
	}

	ret = moro_sim800l_set_data_mode();
	if (ret != ESP_OK) {
		ESP_LOGE(MAIN_TAG, "Failed to set data mode");
	}

	ret = moro_mqtt_init(&mqtt_configurations);
	if (ret != ESP_OK) {
		ESP_LOGE(MAIN_TAG, "Failed to initialize MQTT");
		return;
	}

	while (1) {
		ESP_LOGW(MAIN_TAG, "==============================");
		float latitude = 0, longitude = 0;
		char date_time[32] = {0};
		ret				   = moro_sim800l_get_location(&latitude, &longitude, date_time);
		vTaskDelay(pdMS_TO_TICKS(2500));
		ret = moro_sim800l_set_data_mode();
		vTaskDelay(pdMS_TO_TICKS(10000));
		if (ret == ESP_OK) {
			char payload[128] = {0};
			sprintf(payload, "{\"latitude\": %f, \"longitude\": %f, \"date_time\": \"%s\"}", latitude, longitude, date_time);

			ret = moro_mqtt_publish("location/service", payload, 0, 0, 1);
			if (ret != ESP_OK) {
				ESP_LOGE(MAIN_TAG, "Failed to publish message");
			} else {
				ESP_LOGI(MAIN_TAG, "Message published successfully");
			}
		}
		ESP_LOGW(MAIN_TAG, "==============================");
		vTaskDelay(pdMS_TO_TICKS(2500));
	}
}
