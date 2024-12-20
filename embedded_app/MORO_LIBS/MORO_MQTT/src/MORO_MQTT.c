#include "MORO_MQTT.h"

// ################################## PRIVATE ##################################
#define _MQTT_KEEPALIVE_TIMEOUT (120)
#define _MQTT_RECONNECT_TIMEOUT_MS (5000)

#define _MQTT_TASK_PRIORITY (5)
#define _MQTT_TASK_STACK_SIZE (2048 * 5)

#define _MQTT_RX_BUFFER_SIZE (2048)

#define _MQTT_OUTBOX_LIMIT_IN_BYTES (1024 * 1024 * 5)

static esp_mqtt_client_handle_t mqtt_client	  = NULL;
static esp_mqtt_client_config_t client_config = {0};
static char static_client_id[32]			  = {0};

static bool _is_connected = false;

#define MQTT_DATA_TASK_PRIORITY 5
#define MQTT_DATA_TASK_CORE 0
#define MQTT_DATA_TASK_STACK_SIZE 30000

static mqtt_event_handler_callback_t mqtt_event_handler_callback = NULL;

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
	esp_mqtt_event_handle_t event	= (esp_mqtt_event_handle_t)event_data;
	esp_mqtt_client_handle_t client = event->client;
	switch ((esp_mqtt_event_id_t)event_id) {
		case MQTT_EVENT_BEFORE_CONNECT: {
			break;
		}
		case MQTT_EVENT_CONNECTED: {
			ESP_LOGI(MORO_MQTT_TAG, "MQTT Connected");
			_is_connected = true;

			int ret = 0;

			if (mqtt_event_handler_callback != NULL) {
				mqtt_event_t event = _MQTT_EVENT_CONNECTED_;
				mqtt_event_handler_callback(&event);
			}

			break;
		}
		case MQTT_EVENT_DISCONNECTED: {
			ESP_LOGI(MORO_MQTT_TAG, "MQTT Disconnected");
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
			ESP_LOGI(MORO_MQTT_TAG, "MQTT Unsubscribed from topic: [%.*s]", event->topic_len, event->topic);
			break;
		}
		case MQTT_EVENT_PUBLISHED: {
			ESP_LOGI(MORO_MQTT_TAG, "MQTT Published to topic: [%.*s]", event->topic_len, event->topic);
			break;
		}
		case MQTT_EVENT_DATA: {
			// ESP_LOGI(MORO_MQTT_TAG, "MQTT Data received from topic [%.*s]", event->topic_len, event->topic);

			// We can't use event->topic directly, because it's not null terminated
			// And we can't use a variable which stands on the stack, because it will
			// be deleted after this function. But we can use a variable which stands on the heap.
			char topic[event->topic_len + 1];
			sprintf(topic, "%.*s", event->topic_len, event->topic);

			char payload[event->data_len + 1];
			sprintf(payload, "%.*s", event->data_len, event->data);

			ESP_LOGI(MORO_MQTT_TAG, "MQTT Data received from topic [%s]", topic);
			ESP_LOGI(MORO_MQTT_TAG, "MQTT Data: [%s]", payload);

			break;
		}
		case MQTT_EVENT_ERROR: {
			if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
				ESP_LOGE(MORO_MQTT_TAG, "No internet connection");
				// ESP_LOGE(MORO_MQTT_TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
				// ESP_LOGE(MORO_MQTT_TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
				// ESP_LOGE(MORO_MQTT_TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno, strerror(event->error_handle->esp_transport_sock_errno));
			} else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
				ESP_LOGE(MORO_MQTT_TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
			} else if (event->error_handle->error_type == MQTT_ERROR_TYPE_SUBSCRIBE_FAILED) {
				ESP_LOGE(MORO_MQTT_TAG, "subscribe failed error: 0x%x", event->error_handle->connect_return_code);
			} else {
				ESP_LOGW(MORO_MQTT_TAG, "Unknown error from mqtt: 0x%x", event->error_handle->error_type);
			}

			break;
		}
		default: {
			ESP_LOGE(MORO_MQTT_TAG, "MQTT other event, event id: [ %ld ]", event_id);
			break;
		}
	}
	// ESP_LOGI(MORO_MQTT_TAG, "MQTT Published to topic: [%.*s]", event->topic_len, event->topic);
}

// ################################### PUBLIC ##################################
esp_err_t moro_mqtt_init(mqtt_configurations_t* mqtt_configurations) {
	// ESP_LOGI(MORO_MQTT_TAG, "Initializing MQTT");
	if (mqtt_client != NULL) {
		ESP_LOGE(MORO_MQTT_TAG, "MQTT client already initialized");
		return ESP_FAIL;
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
	ESP_LOGV(MORO_MQTT_TAG, "MQTT Client ID: [%s]", static_client_id);

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
	client_config.session.last_will.topic		= mqtt_configurations->last_will_topic;
	client_config.session.last_will.qos			= 0;
	client_config.session.keepalive				= _MQTT_KEEPALIVE_TIMEOUT;
	client_config.session.disable_clean_session = true;

	// ------ network ------ //

	client_config.network.reconnect_timeout_ms = _MQTT_RECONNECT_TIMEOUT_MS;
	// client_config.network.transport =
	// ------- task ------- //
	client_config.task.priority	  = _MQTT_TASK_PRIORITY;
	client_config.task.stack_size = _MQTT_TASK_STACK_SIZE;

	// ------- buffers ------- //
	client_config.buffer.size  = (_MQTT_RX_BUFFER_SIZE);
	client_config.outbox.limit = _MQTT_OUTBOX_LIMIT_IN_BYTES;

	mqtt_client = esp_mqtt_client_init(&client_config);
	if (mqtt_client == NULL) {
		ESP_LOGE(MORO_MQTT_TAG, "MQTT client init failed");

		return ESP_FAIL;
	}
	esp_err_t ret = esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
	if (ret != ESP_OK) {
		ESP_LOGE(MORO_MQTT_TAG, "MQTT client event handler registration failed");

		// deinit mqtt client
		esp_mqtt_client_destroy(mqtt_client);

		return ret;
	}

	ret = esp_mqtt_client_start(mqtt_client);
	if (ret != ESP_OK) {
		ESP_LOGE(MORO_MQTT_TAG, "MQTT client start failed");

		// deinit mqtt client
		esp_mqtt_client_destroy(mqtt_client);

		return ret;
	}

	ESP_LOGI(MORO_MQTT_TAG, "MQTT init done");
	return ret;
}

esp_err_t moro_mqtt_publish(const char* topic, char* data, int qos, int retain, bool store) {
	if (mqtt_client == NULL) {
		ESP_LOGW(MORO_MQTT_TAG, "MQTT client didn't initialized");
		return ESP_FAIL;
	}

	uint8_t ret = esp_mqtt_client_publish(mqtt_client, topic, data, strlen(data), qos, retain);
	// ESP_LOGI(MORO_MQTT_TAG, "MQTT Publishing to topic: [%s],  ret: %d", topic, ret);

	return ret < 0 ? ESP_FAIL : ESP_OK;
}

bool moro_mqtt_is_connected() {
	return _is_connected;
}

esp_err_t moro_mqtt_configurations_changed(mqtt_configurations_t* mqtt_configurations) {
	if (mqtt_client != NULL) {
		// disconnect unsubscribe and destroy
		esp_mqtt_client_disconnect(mqtt_client);
		esp_mqtt_client_stop(mqtt_client);
		esp_mqtt_client_destroy(mqtt_client);
		// reset mqtt_client
		mqtt_client = NULL;
	}

	return moro_mqtt_init(mqtt_configurations);
}

esp_err_t moro_mqtt_set_event_handler(mqtt_event_handler_callback_t callback) {
	if (callback == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	mqtt_event_handler_callback = callback;

	return ESP_OK;
}
