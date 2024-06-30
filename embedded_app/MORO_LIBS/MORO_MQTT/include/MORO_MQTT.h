#ifndef _MORO_MQTT_H_
#define _MORO_MQTT_H_

#include "MORO_COMMON.h"
#include "mqtt_client.h"

static const char* MORO_MQTT_TAG = "MORO_MQTT";

#define _MQTT_MAX_TOPIC_LEVELS (7)

static const char* MQTT_AUTH_TYPE_NONE					= "none";
static const char* MQTT_AUTH_TYPE_CERTIFICATE			= "certificate";
static const char* MQTT_AUTH_TYPE_BASIC					= "basic";
static const char* MQTT_AUTH_TYPE_BASIC_AND_CERTIFICATE = "basic_certificate";

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

typedef enum {
	_MQTT_EVENT_CONNECTED_,
	_MQTT_EVENT_DISCONNECTED_
} mqtt_event_t;

typedef void (*mqtt_event_handler_callback_t)(mqtt_event_t* event);

typedef esp_err_t (*mqtt_subscription_callback_t)(const char* topic, const char* payload, size_t payload_len);

esp_err_t moro_mqtt_init(mqtt_configurations_t* mqtt_configurations);

esp_err_t moro_mqtt_publish(const char* topic, char* data, int qos, int retain, bool store);

esp_err_t moro_mqtt_subscribe_and_set_callback(const char* topic, mqtt_subscription_callback_t callback);

esp_err_t moro_mqtt_unsubscribe(char* topic);

bool moro_mqtt_is_connected();

esp_err_t moro_mqtt_configurations_changed(mqtt_configurations_t* mqtt_configurations);

esp_err_t moro_mqtt_set_event_handler(mqtt_event_handler_callback_t callback);

#endif	// _MORO_MQTT_H_