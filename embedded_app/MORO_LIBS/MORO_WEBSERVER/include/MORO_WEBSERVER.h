#ifndef _MORO_WEBSERVER_H_
#define _MORO_WEBSERVER_H_

#include <algorithm>
#include <list>

#include "MORO_COMMON.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "ping/ping_sock.h"
#include "sdkconfig.h"

/*
 * This function definition is used to define a handler for a httpd server.
 */
typedef esp_err_t (*webserver_httpd_handler_t)(httpd_req_t *req);

typedef int (*webserver_open_fn_t)(void *handle, int sockfd);

typedef void (*webserver_close_fn_t)(httpd_handle_t handle, int sockfd);

//===================== WEBSERVER RELEATED ====================
esp_err_t start_webserver(void);

esp_err_t stop_webserver(void);

esp_err_t reboot_webserver(void);

esp_err_t add_http_endpoint(const char *uri, httpd_method_t method, webserver_httpd_handler_t handler, bool is_websocket, void *user_ctx);

esp_err_t remove_http_endpoint(const char *uri, httpd_method_t method);

esp_err_t add_open_fn(webserver_open_fn_t open_fn);

esp_err_t add_close_fn(webserver_close_fn_t close_fn);
//=============================================================

#endif	// WEBSERVER_H_