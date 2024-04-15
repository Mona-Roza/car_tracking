#include "MORO_COMMON.h"
#include "MORO_SIM800L.h"

#define RX_PIN GPIO_NUM_23
#define TX_PIN GPIO_NUM_22

static void init() {
	char pin[5]	  = "2278";
	esp_err_t err = MORO_SIM800L::init(TX_PIN, RX_PIN, 2048, 9600, pin);
	if (err != ESP_OK) {
		MORO_LOGE("SIM800L init failed");
		return;
	}

	MORO_LOGI("SIM800L init success");
}

void app_main(void) {
	init();

	// MORO_SIM800L::register_sim_card("2278", 10000, 2);

	// uint8_t rssi;
	// uint8_t ber;
	// esp_err_t ret = MORO_SIM800L::get_signal_quality(&rssi, &ber, 100000, 2);

	// MORO_SIM800L::send_sms("+905312167900", "Hello World", NULL, 100000, 2);
}
