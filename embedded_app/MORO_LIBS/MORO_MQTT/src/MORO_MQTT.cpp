#include "MORO_MQTT.h"
// ################################## PRIVATE ##################################
#define _MQTT_KEEPALIVE_TIMEOUT (120)
#define _MQTT_RECONNECT_TIMEOUT_MS (5000)

#define _MQTT_TASK_PRIORITY (5)
#define _MQTT_TASK_STACK_SIZE (2048 * 20)

#define _MQTT_RX_BUFFER_SIZE (2048)
#define _MQTT_TX_BUFFER_SIZE (2048 * 4)

#define _MQTT_OUTBOX_LIMIT_IN_BYTES (1024 * 1024 * 5)

static esp_mqtt_client_handle_t mqtt_client	  = NULL;
static esp_mqtt_client_config_t client_config = {0};
static char static_client_id[32]			  = {0};

static bool _is_connected = false;

typedef struct {
	const char* topic;
	const char* data;
} mqtt_publish_item_t;

// mqtt tx queue
static QueueHandle_t mqtt_tx_queue = NULL;
#define _MQTT_TX_QUEUE_LENGTH (10)
#define _MQTT_TX_QUEUE_ITEM_SIZE (sizeof(mqtt_publish_item_t))
#define _MQTT_TX_QUEUE_STORAGE_BUFFER_SIZE (_MQTT_TX_QUEUE_LENGTH * _MQTT_TX_QUEUE_ITEM_SIZE)
static uint8_t mqtt_tx_queue_storage_buffer[_MQTT_TX_QUEUE_STORAGE_BUFFER_SIZE];
StaticQueue_t mqtt_tx_queue_static_buffer;

// mqtt tx consumer task
static TaskHandle_t mqtt_tx_consumer_task_handle = NULL;
#define _MQTT_TX_CONSUMER_TASK_STACK_SIZE (2048 * 4)
#define _MQTT_TX_CONSUMER_TASK_PRIORITY (5)
StackType_t mqtt_tx_consumer_task_stack_buffer[_MQTT_TX_CONSUMER_TASK_STACK_SIZE];
StaticTask_t mqtt_tx_consumer_task_buffer;

typedef struct {
	const char* topic;
	mqtt_subscription_callback_t callback;
	bool is_subscribed;
} mqtt_subscription_callback_item_t;

static std::vector<mqtt_subscription_callback_item_t> mqtt_subscription_callback_vector;

static mqtt_event_handler_callback_t mqtt_event_handler_callback = NULL;

static void parse_message_and_call_callback(char* topic, char* payload, size_t payload_len) {
	// traverse vector and find the callback
	if (mqtt_subscription_callback_vector.size() == 0) {
		MORO_LOGE("MQTT subscription callback vector is empty");
		return;
	}

	bool match_found = false;

	for (uint8_t i = 0; i < mqtt_subscription_callback_vector.size(); i++) {
		if (mqtt_subscription_callback_vector[i].is_subscribed == false) {
			continue;
		}

		if (wildcard_match(mqtt_subscription_callback_vector[i].topic, topic)) {
			match_found	  = true;
			esp_err_t ret = mqtt_subscription_callback_vector[i].callback(topic, payload, payload_len);
			if (ret != ESP_OK) {
				MORO_LOGE("unable to handle mqtt message for topic [%s]", topic);
			}
		}
	}

	if (!match_found) {
		MORO_LOGE("MQTT topic [%s] doesn't match with any callback", topic);
	}

	return;
}

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
	esp_mqtt_event_handle_t event	= (esp_mqtt_event_handle_t)event_data;
	esp_mqtt_client_handle_t client = event->client;
	switch ((esp_mqtt_event_id_t)event_id) {
		case MQTT_EVENT_BEFORE_CONNECT: {
			break;
		}
		case MQTT_EVENT_CONNECTED: {
			MORO_LOGI("MQTT Connected");
			_is_connected = true;

			int ret = 0;

			for (uint8_t i = 0; i < mqtt_subscription_callback_vector.size(); i++) {
				if (mqtt_subscription_callback_vector[i].topic == NULL || strcmp(mqtt_subscription_callback_vector[i].topic, "") == 0) {
					continue;
				}

				ret = esp_mqtt_client_subscribe(client, mqtt_subscription_callback_vector[i].topic, 0);
				if (ret == -1 || ret == -2) {
					MORO_LOGE("MQTT subscribe failed");
					mqtt_subscription_callback_vector[i].is_subscribed = false;
					MORO_LOGE("MQTT couldn't subscribe to topic [%s]", mqtt_subscription_callback_vector[i].topic);
					break;
				} else {
					mqtt_subscription_callback_vector[i].is_subscribed = true;
					// MORO_LOGI("MQTT subscribed to topic [%s]", mqtt_subscription_callback_vector[i].topic);
				}
			}
			MORO_LOGI("MQTT subscribed to all topics");

			if (mqtt_event_handler_callback != NULL) {
				mqtt_event_t event = _MQTT_EVENT_CONNECTED_;
				mqtt_event_handler_callback(&event);
			}

			break;
		}
		case MQTT_EVENT_DISCONNECTED: {
			MORO_LOGI("MQTT Disconnected");
			_is_connected = false;

			if (mqtt_event_handler_callback != NULL) {
				mqtt_event_t event = _MQTT_EVENT_DISCONNECTED_;
				mqtt_event_handler_callback(&event);
			}

			break;
		}
		case MQTT_EVENT_SUBSCRIBED: {
			break;
		}
		case MQTT_EVENT_UNSUBSCRIBED: {
			MORO_LOGI("MQTT Unsubscribed from topic: [%.*s]", event->topic_len, event->topic);
			break;
		}
		case MQTT_EVENT_PUBLISHED: {
			// MORO_LOGI("MQTT Published to topic: [%.*s]", event->topic_len, event->topic);
			break;
		}
		case MQTT_EVENT_DATA: {
			MORO_LOGI("MQTT Data received from topic [%.*s]", event->topic_len, event->topic);

			// We can't use event->topic directly, because it's not null terminated
			// And we can't use a variable which stands on the stack, because it will
			// be deleted after this function. But we can use a variable which stands on the heap.
			char topic[event->topic_len + 1];
			sprintf(topic, "%.*s", event->topic_len, event->topic);

			char payload[event->data_len + 1];

			sprintf(payload, "%.*s", event->data_len, event->data);

			parse_message_and_call_callback(topic, payload, event->data_len);

			break;
		}
		case MQTT_EVENT_ERROR: {
			if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
				MORO_LOGE("No internet connection");
				// MORO_LOGE("Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
				// MORO_LOGE("Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
				// MORO_LOGE("Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno, strerror(event->error_handle->esp_transport_sock_errno));
			} else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
				MORO_LOGE("Connection refused error: 0x%x", event->error_handle->connect_return_code);
			} else if (event->error_handle->error_type == MQTT_ERROR_TYPE_SUBSCRIBE_FAILED) {
				MORO_LOGE("subscribe failed error: 0x%x", event->error_handle->connect_return_code);
			} else {
				MORO_LOGW("Unknown error from mqtt: 0x%x", event->error_handle->error_type);
			}

			break;
		}
		default: {
			MORO_LOGE("MQTT other event, event id: [%d]", event_id);
			break;
		}
	}
	// MORO_LOGI("MQTT Published to topic: [%.*s]", event->topic_len, event->topic);
}

// mqtt tx consumer task
static void mqtt_tx_consumer_task(void* pvParameters) {
	mqtt_publish_item_t item;
	while (1) {
		if (xQueueReceive(mqtt_tx_queue, &item, portMAX_DELAY) == pdTRUE) {
			// qos = 0, retain = 0

			int message_id = esp_mqtt_client_publish(mqtt_client, item.topic, item.data, strlen(item.data), 0, 0);

			if (message_id == -1) {
				MORO_LOGE("MQTT publish failed");
			}

			// MORO_LOGI("MQTT Published to topic: [%s]", item.topic);
			// now we can free
			free((void*)item.topic);
			free((void*)item.data);
		}
	}
}

// ################################### PUBLIC ##################################

esp_err_t init_mqtt(mqtt_configurations_t* mqtt_configurations) {
	// MORO_LOGI("Initializing MQTT");
	if (mqtt_client != NULL) {
		return ESP_OK;
	}

	char random_str[11];

	for (int i = 0; i < 10; i++) {
		random_str[i] = '0' + (esp_random() % 10);
	}
	random_str[10] = '\0';

	sprintf(static_client_id, "%02X:%02X:%02X:%02X:%02X:%02X-%s", mqtt_configurations->device_mac[0], mqtt_configurations->device_mac[1], mqtt_configurations->device_mac[2], mqtt_configurations->device_mac[3], mqtt_configurations->device_mac[4], mqtt_configurations->device_mac[5], random_str);

	// ------ broker ------ //
	client_config.broker.address.uri = mqtt_configurations->endpoint;

	// ------ client ------ //
	client_config.credentials.client_id			 = strdup(static_client_id);
	client_config.credentials.set_null_client_id = false;
	MORO_LOGD("MQTT Client ID: [%s]", static_client_id);

	/*
	 *  If connection type is BASIC, esp-mqtt wants NULL for cert_pem, client_cert_pem and client_key_pem.
	 *  So we will check the connection type and set the values accordingly.
	 */

	if (strcmp(mqtt_configurations->auth_type, MQTT_AUTH_TYPE_BASIC_AND_CERTIFICATE) == 0 &&
		mqtt_configurations->client_cert != NULL &&
		mqtt_configurations->client_cert_size > 0 &&
		mqtt_configurations->client_cert[0] != '\0' &&

		mqtt_configurations->client_key != NULL &&
		mqtt_configurations->client_key_size > 0 &&
		mqtt_configurations->client_key[0] != '\0' &&

		mqtt_configurations->ca_cert != NULL &&
		mqtt_configurations->ca_cert_size > 0 &&
		mqtt_configurations->ca_cert[0] != '\0' &&

		strcmp(mqtt_configurations->username, " ") != 0 &&
		mqtt_configurations->username[0] != '\0' &&

		strcmp(mqtt_configurations->password, " ") != 0 &&
		mqtt_configurations->password[0] != '\0') {
		client_config.credentials.authentication.certificate	 = strdup(mqtt_configurations->client_cert);
		client_config.credentials.authentication.certificate_len = mqtt_configurations->client_cert_size;

		client_config.credentials.authentication.key	 = strdup(mqtt_configurations->client_key);
		client_config.credentials.authentication.key_len = mqtt_configurations->client_key_size;

		client_config.broker.verification.certificate	  = strdup(mqtt_configurations->ca_cert);
		client_config.broker.verification.certificate_len = mqtt_configurations->ca_cert_size;

		client_config.credentials.username = strdup(mqtt_configurations->username);

		client_config.credentials.authentication.password = strdup(mqtt_configurations->password);

	} else if (strcmp(mqtt_configurations->auth_type, MQTT_AUTH_TYPE_CERTIFICATE) == 0 &&
			   mqtt_configurations->client_cert != NULL &&
			   mqtt_configurations->client_cert_size > 0 &&
			   mqtt_configurations->client_cert[0] != '\0' &&

			   mqtt_configurations->client_key != NULL &&
			   mqtt_configurations->client_key_size > 0 &&
			   mqtt_configurations->client_key[0] != '\0' &&

			   mqtt_configurations->ca_cert != NULL &&
			   mqtt_configurations->ca_cert_size > 0 &&
			   mqtt_configurations->ca_cert[0] != '\0') {
		client_config.credentials.authentication.certificate	 = strdup(mqtt_configurations->client_cert);
		client_config.credentials.authentication.certificate_len = mqtt_configurations->client_cert_size;

		client_config.credentials.authentication.key	 = strdup(mqtt_configurations->client_key);
		client_config.credentials.authentication.key_len = mqtt_configurations->client_key_size;

		client_config.broker.verification.certificate	  = strdup(mqtt_configurations->ca_cert);
		client_config.broker.verification.certificate_len = mqtt_configurations->ca_cert_size;
	} else if (strcmp(mqtt_configurations->auth_type, MQTT_AUTH_TYPE_BASIC) == 0 &&
			   strcmp(mqtt_configurations->username, " ") != 0 &&
			   mqtt_configurations->username[0] != '\0' &&

			   strcmp(mqtt_configurations->password, " ") != 0 &&
			   mqtt_configurations->password[0] != '\0') {
		client_config.credentials.username = strdup(mqtt_configurations->username);

		client_config.credentials.authentication.password = strdup(mqtt_configurations->password);

		client_config.credentials.authentication.certificate	 = NULL;
		client_config.credentials.authentication.certificate_len = 0;

		client_config.credentials.authentication.key	 = NULL;
		client_config.credentials.authentication.key_len = 0;

		client_config.broker.verification.certificate	  = NULL;
		client_config.broker.verification.certificate_len = 0;

	} else if (strcmp(mqtt_configurations->auth_type, MQTT_AUTH_TYPE_NONE) == 0) {
	}

	// ------ session ------ //
	client_config.session.last_will.topic = mqtt_configurations->last_will_topic;
	client_config.session.last_will.qos	  = 0;
	client_config.session.keepalive		  = _MQTT_KEEPALIVE_TIMEOUT;

	// ------ network ------ //

	client_config.network.reconnect_timeout_ms = _MQTT_RECONNECT_TIMEOUT_MS;
	// client_config.network.transport = //todo if we implement our own transport can we overcome http blocking mqtt connection?

	// ------- task ------- //
	client_config.task.priority	  = _MQTT_TASK_PRIORITY;
	client_config.task.stack_size = _MQTT_TASK_STACK_SIZE;

	// ------- buffers ------- //
	client_config.buffer.size  = (_MQTT_RX_BUFFER_SIZE);
	client_config.outbox.limit = _MQTT_OUTBOX_LIMIT_IN_BYTES;

	mqtt_client = esp_mqtt_client_init(&client_config);
	if (mqtt_client == NULL) {
		MORO_LOGE("MQTT client init failed");
		return ESP_FAIL;
	}
	esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
	esp_mqtt_client_start(mqtt_client);

	// init tx queue in spi ram
	mqtt_tx_queue = xQueueCreateStatic(_MQTT_TX_QUEUE_LENGTH, _MQTT_TX_QUEUE_ITEM_SIZE, mqtt_tx_queue_storage_buffer, &mqtt_tx_queue_static_buffer);

	if (mqtt_tx_queue == NULL) {
		MORO_LOGE("MQTT tx queue creation failed");
		return ESP_FAIL;
	}

	// create tx consumer task
	mqtt_tx_consumer_task_handle = xTaskCreateStatic(mqtt_tx_consumer_task, "mqtt_tx_consumer_task", _MQTT_TX_CONSUMER_TASK_STACK_SIZE, NULL, _MQTT_TX_CONSUMER_TASK_PRIORITY, mqtt_tx_consumer_task_stack_buffer, &mqtt_tx_consumer_task_buffer);

	if (mqtt_tx_consumer_task_handle == NULL) {
		MORO_LOGE("MQTT tx consumer task creation failed");
		// delete queue
		vQueueDelete(mqtt_tx_queue);
		return ESP_FAIL;
	}

	MORO_LOGI("MQTT init done");
	return ESP_OK;
}

esp_err_t mqtt_publish(const char* topic, char* data) {
	if (mqtt_client == NULL) {
		MORO_LOGW("MQTT client didn't initialized");
		return ESP_FAIL;
	}

	// MORO_LOGI("MQTT Publishing to topic: [%s]", topic);
	// copy topic and data to new variables
	mqtt_publish_item_t item;
	item.topic = strdup(topic);
	item.data  = strdup(data);

	if (xQueueSend(mqtt_tx_queue, &item, 0) != pdTRUE) {
		MORO_LOGE("MQTT tx queue is full");
		return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t mqtt_subscribe_and_set_callback(const char* topic, mqtt_subscription_callback_t callback) {
	// qos = 0

	char* topic_copy = (char*)malloc(strlen(topic) + 1);
	strcpy(topic_copy, topic);

	mqtt_subscription_callback_item_t item;
	item.topic	  = strdup(topic);
	item.callback = callback;
	mqtt_subscription_callback_vector.push_back(item);

	if (mqtt_client != NULL && _is_connected) {
		esp_mqtt_client_subscribe(mqtt_client, topic, 0);
	}

	return ESP_OK;
}

esp_err_t mqtt_unsubscribe(char* topic) {
	if (mqtt_client == NULL) {
		MORO_LOGE("MQTT client is null");
		return ESP_FAIL;
	}

	esp_err_t ret = esp_mqtt_client_unsubscribe(mqtt_client, topic);
	if (ret != ESP_OK) {
		MORO_LOGE("MQTT unsubscribe failed");
		return ESP_FAIL;
	}

	for (uint8_t i = 0; i < mqtt_subscription_callback_vector.size(); i++) {
		if (strcmp(mqtt_subscription_callback_vector[i].topic, topic) == 0) {
			mqtt_subscription_callback_vector.erase(mqtt_subscription_callback_vector.begin() + i);
			break;
		}
	}

	return ESP_OK;
}

bool mqtt_is_connected() {
	return _is_connected;
}

esp_err_t mqtt_configurations_changed(mqtt_configurations_t* mqtt_configurations) {
	if (mqtt_client != NULL) {
		// disconnect unsubscribe and destroy
		esp_mqtt_client_disconnect(mqtt_client);
		esp_mqtt_client_stop(mqtt_client);
		esp_mqtt_client_destroy(mqtt_client);
		// delete task, queue and buffers
		if (mqtt_tx_consumer_task_handle != NULL) {
			vTaskDelete(mqtt_tx_consumer_task_handle);
		}

		if (mqtt_tx_queue != NULL) {
			vQueueDelete(mqtt_tx_queue);
		}

		vQueueDelete(mqtt_tx_queue);
		// reset mqtt_client
		mqtt_client = NULL;
	}

	return init_mqtt(mqtt_configurations);
}

esp_err_t mqtt_set_event_handler(mqtt_event_handler_callback_t callback) {
	if (callback == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	mqtt_event_handler_callback = callback;

	return ESP_OK;
}
