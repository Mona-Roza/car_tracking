#ifndef _MORO_MQTT_H_
#define _MORO_MQTT_H_

#include <list>

#include "MORO_COMMON.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "mqtt_client.h"
#include "sdkconfig.h"

#define _MQTT_MAX_TOPIC_LEVELS (7)

static const char* MQTT_AUTH_TYPE_NONE					  = "none";
static const char* MQTT_AUTH_TYPE_CERTIFICATE			  = "certificate";
static const char* MQTT_AUTH_TYPE_BASIC					  = "basic";
static const char* MQTT_AUTH_TYPE_BASIC_AND_CERTIFICATE	  = "basic_certificate";
static const esp_transport_handle_t MQTT_CUSTOM_TRANSPORT = NULL;
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

esp_err_t init_mqtt(mqtt_configurations_t* mqtt_configurations);

esp_err_t mqtt_publish(const char* topic, char* data);

esp_err_t mqtt_subscribe_and_set_callback(const char* topic, mqtt_subscription_callback_t callback);

esp_err_t mqtt_unsubscribe(char* topic);

bool mqtt_is_connected();

esp_err_t mqtt_configurations_changed(mqtt_configurations_t* mqtt_configurations);

esp_err_t mqtt_set_event_handler(mqtt_event_handler_callback_t callback);

#endif	// _MORO_MQTT_H_