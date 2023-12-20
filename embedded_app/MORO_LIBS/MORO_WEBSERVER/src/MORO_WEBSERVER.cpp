#include "MORO_WEBSERVER.h"

// ############################## PRIVATE ######################################
#define WEBSERVER_TASK_PRIORITY tskIDLE_PRIORITY + 5
#define WEBSERVER_TASK_STACK_SIZE 2 * 4096	// increased for wifi scan results unable to fit into stack
#define WEBSERVER_TASK_CORE_ID tskNO_AFFINITY
#define WEBSERVER_PORT 80
#define WEBSERVER_CTRL_PORT 32768
#define WEBSERVER_MAX_OPEN_SOCKETS 13  // https://esp32.com/viewtopic.php?t=31224 Re: ESP-IDF v5.0 introducing " httpd_accept_conn: error in accept (23)"?
#define WEBSERVER_MAX_URI_HANDLERS 50
#define WEBSERVER_MAX_RESP_HEADERS 20
#define WEBSERVER_BACKLOG_CONN 5
#define WEBSERVER_LRU_PURGE_ENABLE true
#define WEBSERVER_RECV_WAIT_TIMEOUT 5
#define WEBSERVER_SEND_WAIT_TIMEOUT 5

static httpd_handle_t server = NULL;

static std::list<httpd_uri_t> httpd_uri_list;

static std::vector<httpd_open_func_t> httpd_open_func_list;

static std::vector<httpd_close_func_t> httpd_close_func_list;

TaskHandle_t webserver_start_waiter_task_handle = NULL;

static int open_fn(void* hd, int sockfd) {
	int ret = 0;
	for (int i = 0; i < httpd_open_func_list.size(); i++) {
		if (httpd_open_func_list[i] != NULL) {
			ret = httpd_open_func_list[i](hd, sockfd);
			if (ret != 0) {
				return ret;
			}
		}
	}

	return ret;
}

static void close_fn(httpd_handle_t hd, int sockfd) {
	for (int i = 0; i < httpd_close_func_list.size(); i++) {
		if (httpd_close_func_list[i] != NULL) {
			httpd_close_func_list[i](hd, sockfd);
		}
	}
	close(sockfd);
}

static void unregister_all_http_endpoints() {
	for (auto& _httpd_uri : httpd_uri_list) {
		httpd_unregister_uri_handler(server, _httpd_uri.uri, _httpd_uri.method);
	}
	MORO_LOGW("Unregistered all http endpoints");
}

static void register_all_http_endpoints() {
	for (auto& _httpd_uri : httpd_uri_list) {
		httpd_register_uri_handler(server, &_httpd_uri);
	}
	MORO_LOGI("Registered all http endpoints");
}

// ############################## PUBLIC ########################################

esp_err_t start_webserver(void) {
	if (server != NULL) {
		server = NULL;
	}

	httpd_config_t config = {
		.task_priority				  = WEBSERVER_TASK_PRIORITY,
		.stack_size					  = WEBSERVER_TASK_STACK_SIZE,
		.core_id					  = WEBSERVER_TASK_CORE_ID,
		.server_port				  = WEBSERVER_PORT,
		.ctrl_port					  = WEBSERVER_CTRL_PORT,
		.max_open_sockets			  = WEBSERVER_MAX_OPEN_SOCKETS,
		.max_uri_handlers			  = WEBSERVER_MAX_URI_HANDLERS,
		.max_resp_headers			  = WEBSERVER_MAX_RESP_HEADERS,
		.backlog_conn				  = WEBSERVER_BACKLOG_CONN,
		.lru_purge_enable			  = WEBSERVER_LRU_PURGE_ENABLE,	 // https://github.com/espressif/esp-idf/issues/3851#issuecomment-639273670, we could also appy patch
		.recv_wait_timeout			  = WEBSERVER_RECV_WAIT_TIMEOUT,
		.send_wait_timeout			  = WEBSERVER_SEND_WAIT_TIMEOUT,
		.global_user_ctx			  = NULL,
		.global_user_ctx_free_fn	  = NULL,
		.global_transport_ctx		  = NULL,
		.global_transport_ctx_free_fn = NULL,
		.enable_so_linger			  = false,	// https://esp32.com/viewtopic.php?t=31224 Re: ESP-IDF v5.0 introducing " httpd_accept_conn: error in accept (23)"?
		.open_fn					  = &open_fn,
		.close_fn					  = &close_fn,
		.uri_match_fn				  = httpd_uri_match_wildcard,
	};

	// MORO_LOGI("Starting webserver on port [%d]", config.server_port);

	esp_err_t ret = httpd_start(&server, &config);

	if (ret != ESP_OK) {
		MORO_LOGE("Error starting webserver, error [%s]", esp_err_to_name(ret));
		return ret;
	}

	MORO_LOGI("Webserver started on port [%d]", config.server_port);

	return ESP_OK;
}

esp_err_t stop_webserver(void) {
	unregister_all_http_endpoints();

	if (server == NULL) {
		MORO_LOGW("Webserver already stopped");
		return ESP_OK;
	}

	MORO_LOGI("Stopping webserver");

	esp_err_t ret = httpd_stop(server);

	if (ret != ESP_OK) {
		MORO_LOGE("Error stopping webserver, error [%s]", esp_err_to_name(ret));
		return ret;
	}

	server = NULL;

	MORO_LOGI("Webserver stopped successfully");
	return ESP_OK;
}

esp_err_t reboot_webserver(void) {
	esp_err_t ret = stop_webserver();

	if (ret != ESP_OK) return ret;

	ret = start_webserver();

	if (ret != ESP_OK) return ret;

	return ESP_OK;
}

esp_err_t add_http_endpoint(const char* uri, httpd_method_t method, webserver_httpd_handler_t handler, bool is_websocket, void* user_ctx) {
	esp_err_t ret		   = ESP_OK;
	httpd_uri_t _httpd_uri = {
		.uri				   = uri,
		.method				   = method,
		.handler			   = handler,
		.user_ctx			   = user_ctx,
		.is_websocket		   = is_websocket,
		.supported_subprotocol = NULL,
	};

	if (server == NULL) {
		MORO_LOGE("Webserver not started");
		return ESP_FAIL;
	}
	ret = httpd_register_uri_handler(server, &_httpd_uri);
	if (ret != ESP_OK) {
		MORO_LOGE("Error adding http service [%s], error [%s]", uri, esp_err_to_name(ret));
		return ret;
	}
	// MORO_LOGI("Added http service [%s], [HTTPD_%s]", uri, method == HTTP_GET ? "GET" : "POST");
	httpd_uri_list.push_back(_httpd_uri);
	return ret;
}

esp_err_t remove_http_endpoint(const char* uri, httpd_method_t method) {
	esp_err_t ret = ESP_OK;

	if (server != NULL) {
		MORO_LOGI("Removing http service [%s]", uri);
		ret = httpd_unregister_uri_handler(server, uri, method);
		if (ret != ESP_OK) {
			MORO_LOGE("Error removing http service [%s], error [%s]", uri, esp_err_to_name(ret));
		}
	}

	auto it = std::find_if(httpd_uri_list.begin(), httpd_uri_list.end(), [uri, method](httpd_uri_t& _httpd_uri) {
		return (strcmp(_httpd_uri.uri, uri) == 0) && (_httpd_uri.method == method);
	});

	if (it != httpd_uri_list.end()) {
		httpd_uri_list.erase(it);
	}

	return ret;
}

esp_err_t add_open_fn(webserver_open_fn_t open_fn) {
	httpd_open_func_list.push_back(open_fn);
	return ESP_OK;
}

esp_err_t add_close_fn(webserver_close_fn_t close_fn) {
	httpd_close_func_list.push_back(close_fn);
	return ESP_OK;
}
