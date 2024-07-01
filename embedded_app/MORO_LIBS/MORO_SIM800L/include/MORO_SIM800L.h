#ifndef _MORO_SIM800L_H_
#define _MORO_SIM800L_H_

#include "MORO_COMMON.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_modem_api.h"
#include "esp_modem_c_api_types.h"
#include "esp_modem_config.h"
#include "esp_modem_dce_config.h"
#include "esp_netif.h"
#include "esp_netif_defaults.h"
#include "esp_netif_ppp.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#define MORO_SIM800L_TAG "MORO_SIM800L"

esp_err_t moro_sim800l_init(gpio_num_t uart_tx, gpio_num_t uart_rx, uint32_t uart_rx_buffer_size, uint32_t uart_tx_buffer_size, uint32_t uart_event_queue_size, uint32_t uart_event_task_stack_size, uint32_t uart_event_task_priority);

esp_err_t moro_sim800l_get_signal_quality(int *rssi, int *ber);

esp_err_t moro_sim800l_set_data_mode();

void moro_sim800l_get_connected_ip(esp_ip4_addr_t *ip);

esp_err_t moro_sim800l_send_sms(const char *phone_number, const char *message);

esp_err_t moro_sim800l_get_location(float *latitude, float *longitude, char *date_time);

#endif /* _MORO_SIM800L_H_ */