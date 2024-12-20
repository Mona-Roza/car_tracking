#include "MORO_COMMON.h"

bool wildcard_match(const char *pattern, const char *value) {
	if (*pattern == '\0' && *value == '\0') return true;
	if (*pattern == '#') {
		if (*(pattern + 1) != '\0' && *value == '\0') return false;
		return wildcard_match(pattern + 1, value) || wildcard_match(pattern, value + 1);
	} else if (*pattern == '+') {
		while (*value != '\0' && *value != '/') {
			if (wildcard_match(pattern + 1, value + 1)) return true;
			value++;
		}
		return false;
	} else if (*pattern == *value)
		return wildcard_match(pattern + 1, value + 1);
	return false;
}

void url_encoder(char *buf, const char *input) {
	while (*input) {
		if (*input == '%') {
			char buffer[3] = {input[1], input[2], '\0'};
			*buf++		   = strtol(buffer, NULL, 16);
			input += 3;
		} else {
			*buf++ = *input++;
		}
	}
	*buf = '\0';
}

esp_err_t init_spiffs(const char *base_path, const char *partition_label) {
	esp_vfs_spiffs_conf_t conf = {
		.base_path				= base_path,  // using label as base path
		.partition_label		= partition_label,
		.max_files				= 10,
		.format_if_mount_failed = true,
	};

	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK) {
		return ret;
	}
	return ESP_OK;
}

static const esp_efuse_desc_t MORO_MAC[] = {
	// Mac address, starting from 0, 48bits which is 6bytes
	{EFUSE_BLK3, 0, sizeof(moro_mac_t) * 8}};

static const esp_efuse_desc_t *ESP_EFUSE_MORO_MAC[] = {
	&MORO_MAC[0],
	NULL};

// 1 2 3 4 5 6 7 8 9 A B C D E F
const char *prefix_str = "B0:00:";

static esp_err_t get_efuse_mac_custom(uint8_t *mac) {
#if !CONFIG_IDF_TARGET_ESP32
	size_t size_bits = esp_efuse_get_field_size(ESP_EFUSE_USER_DATA_MAC_CUSTOM);
	assert((size_bits % 8) == 0);
	esp_err_t err = esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_MAC_CUSTOM, mac, size_bits);
	if (err != ESP_OK) {
		return err;
	}
	size_t size = size_bits / 8;
	if (mac[0] == 0 && memcmp(mac, &mac[1], size - 1) == 0) {
		ESP_LOGV("SM_COMMON", "eFuse MAC_CUSTOM is empty");
		return ESP_ERR_INVALID_MAC;
	}
#else
	uint8_t version;
	esp_efuse_read_field_blob(ESP_EFUSE_MAC_CUSTOM_VER, &version, 8);
	if (version != 1) {
		// version 0 means has not been setup
		if (version == 0) {
			ESP_LOGV("MORO_COMMON", "No base MAC address in eFuse (version=0)");
		} else if (version != 1) {
			ESP_LOGV("MORO_COMMON", "Base MAC address version error, version = %d", version);
		}
		return ESP_ERR_INVALID_VERSION;
	}

	uint8_t efuse_crc;
	esp_efuse_read_field_blob(ESP_EFUSE_MAC_CUSTOM, mac, 48);
	esp_efuse_read_field_blob(ESP_EFUSE_MAC_CUSTOM_CRC, &efuse_crc, 8);
	uint8_t calc_crc = esp_rom_efuse_mac_address_crc8(mac, 6);

	if (efuse_crc != calc_crc) {
		ESP_LOGV("MORO_COMMON", "Base MAC address from BLK3 of EFUSE CRC error, efuse_crc = 0x%02x; calc_crc = 0x%02x", efuse_crc, calc_crc);
#ifdef CONFIG_ESP_MAC_IGNORE_MAC_CRC_ERROR
		ESP_LOGV("MORO_COMMON", "Ignore MAC CRC error");
#else
		return ESP_ERR_INVALID_CRC;
#endif
	}
#endif
	return ESP_OK;
}

void list_all_files(const char *file_path) {
	DIR *dir;
	struct dirent *ent;

	cJSON *root = cJSON_CreateObject();

	if ((dir = opendir(file_path)) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			cJSON_AddNumberToObject(root, ent->d_name, ent->d_type);
		}
		closedir(dir);
	} else {
		/* could not open directory */
		ESP_LOGE("MORO_COMMON", "Failed to open [%s]. Aborting.", file_path);
	}

	ESP_LOGV("MORO_COMMON", "Files in [%s]: %s", file_path, cJSON_Print(root));

	cJSON_Delete(root);
}

esp_err_t read_mac(moro_mac_t *smac) {
	uint8_t mac[6];
	esp_err_t ret;

	ret = get_efuse_mac_custom(mac);

	char mac_str[] = "00:00:00:00:00:00";
	sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	ESP_LOGV("MORO_COMMON", "MORO MAC Prefix: %s", prefix_str);
	ESP_LOGV("MORO_COMMON", "MORO MAC CUSTOM mac: %s", mac_str);
	// check mac_str start with prefix
	if (strncmp(mac_str, prefix_str, 8) != 0) {
		ESP_LOGV("MORO_COMMON", "MOROMAC CUSTOM mac not set, reading from legacy mac field");
		ret = esp_efuse_read_field_blob(ESP_EFUSE_MORO_MAC, &mac, sizeof(moro_mac_t) * 8);
		sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		// ESP_LOGV("MORO_COMMON","SMAC Legacy mac: %s", mac_str);
	}

	if (strncmp(mac_str, prefix_str, 8) != 0) {
		ESP_LOGW("MORO_COMMON", "MOROMAC CUSTOM mac not set. Failure mac address activated!");
		uint8_t failure_mac[] = {0xDE, 0xAD, 0xCA, 0xFE, 0xBA, 0xBE};
		memcpy(mac, failure_mac, sizeof(mac));
		ret = ESP_OK;
	}

	memcpy(smac->mac, mac, sizeof(smac->mac));
	sprintf(smac->mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", smac->mac[0], smac->mac[1], smac->mac[2], smac->mac[3], smac->mac[4], smac->mac[5]);
	ESP_LOGD("MORO_COMMON", "MOROMAC: %s", smac->mac_str);
	return ret;
}
