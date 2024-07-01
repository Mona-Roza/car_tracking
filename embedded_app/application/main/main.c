
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
	char last_will_topic[128] = {0};

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

	sprintf(last_will_topic, "%s/%s", MORO_MQTT_BASE_TOPIC, mqtt_configurations.device_mac);

	strcpy(mqtt_configurations.last_will_topic, last_will_topic);

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

	ret = moro_mqtt_subscribe_and_set_callback("monaroza/basak", cb);
	if (ret != ESP_OK) {
		ESP_LOGE(MAIN_TAG, "Failed to subscribe to topic");
		return;
	}

	// while (1) {
	// 	ret = moro_mqtt_publish("monaroza/basak", "Hello World", 0, 0, false);
	// 	if (ret != ESP_OK) {
	// 		ESP_LOGE(MAIN_TAG, "Failed to publish message");
	// 	} else {
	// 		ESP_LOGI(MAIN_TAG, "Message published successfully");
	// 	}
	// 	vTaskDelay(pdMS_TO_TICKS(1000));
	// }
}
