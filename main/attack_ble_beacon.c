#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <string.h>
#include "esp_bt_main.h"

static const char *TAG = "ble_beacon_spam";

#define BEACON_ADV_INTERVAL 0x20 // 20ms units (0x20 = 32 * 0.625ms = 20ms)

static uint8_t vendorList[] = {
    0x4C, 0x00, // Apple (LSB first for manufacturer id 0x004C)
    0x50, 0x00, // Samsung
    0x60, 0x00, // Google
    0x75, 0x00, // Microsoft
    0xA0, 0x00, // Sony
    0xE0, 0x00  // LG
};
#define VENDOR_COUNT (sizeof(vendorList)/2)

static void generate_random_uuid(uint8_t *uuid) {
    for (int i = 0; i < 16; i++) {
        uuid[i] = esp_random() & 0xFF;
    }
    // Set UUID version and variant bits (version 4 UUID)
    uuid[6] = (uuid[6] & 0x0F) | 0x40; // Version 4
    uuid[8] = (uuid[8] & 0x3F) | 0x80; // Variant bits
}

static void prepare_beacon_data(uint8_t *adv_data, uint8_t *length) {
    // Apple iBeacon format
    // Flags
    uint8_t flags[] = {
        0x02, 0x01, 0x06
    };

    // Length of Manufacturer data (0x1A), Type (0xFF), Manufacturer ID (2 bytes),
    // iBeacon type (0x02), Length (0x15), UUID(16 bytes), Major(2 bytes), Minor(2 bytes), Tx Power(1 byte)
    uint8_t mfg_data[25];

    uint16_t manuf_id_idx = (esp_random() % VENDOR_COUNT) * 2;
    mfg_data[0] = vendorList[manuf_id_idx];     // Manufacturer ID LSB
    mfg_data[1] = vendorList[manuf_id_idx + 1]; // Manufacturer ID MSB

    mfg_data[2] = 0x02;  // iBeacon type
    mfg_data[3] = 0x15;  // iBeacon data length

    generate_random_uuid(&mfg_data[4]);

    // Random Major and Minor
    mfg_data[20] = (esp_random() & 0xFF);
    mfg_data[21] = (esp_random() & 0xFF);
    mfg_data[22] = (esp_random() & 0xFF);
    mfg_data[23] = (esp_random() & 0xFF);

    mfg_data[24] = 0xC5; // Measured Power (Tx Power)

    // Now prepare the advertising data buffer
    int pos = 0;

    // Copy flags
    memcpy(adv_data + pos, flags, sizeof(flags));
    pos += sizeof(flags);

    // Manufacturer data AD structure
    adv_data[pos++] = 0x1A; // Length of this AD structure (26 bytes)
    adv_data[pos++] = 0xFF; // Manufacturer specific data type

    memcpy(adv_data + pos, mfg_data, sizeof(mfg_data));
    pos += sizeof(mfg_data);

    *length = pos;
}

void ble_beacon_spam_task(void *param) {
    int duration = (int)param;

    esp_err_t ret;

    // Init BLE controller and Bluedroid stack
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Bluetooth controller initialize failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid stack init failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    // Prepare advertising data
    uint8_t adv_data[31];
    uint8_t adv_data_len = 0;
    prepare_beacon_data(adv_data, &adv_data_len);

    esp_ble_adv_data_t ble_adv_data = {
        .set_scan_rsp = false,
        .include_name = false,
        .include_txpower = false,
        .min_interval = 0x0006, 
        .max_interval = 0x0010,
        .appearance = 0x00,
        .manufacturer_len = adv_data_len - 3, // exclude flags (3 bytes)
        .p_manufacturer_data = &adv_data[3], // manufacturer data starts after flags
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 0,
        .p_service_uuid = NULL,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };

    ret = esp_ble_gap_config_adv_data(&ble_adv_data);
    if (ret) {
        ESP_LOGE(TAG, "Config adv data failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    esp_ble_adv_params_t adv_params = {
        .adv_int_min = BEACON_ADV_INTERVAL,
        .adv_int_max = BEACON_ADV_INTERVAL,
        .adv_type = ADV_TYPE_NONCONN_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    ret = esp_ble_gap_start_advertising(&adv_params);
    if (ret) {
        ESP_LOGE(TAG, "Start advertising failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "BLE Beacon spam started for %d seconds", duration);

    vTaskDelay(duration * 1000 / portTICK_PERIOD_MS);

    esp_ble_gap_stop_advertising();

    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    ESP_LOGI(TAG, "BLE Beacon spam finished");

    vTaskDelete(NULL);
}

void attack_ble_beacon_start(int duration_seconds) {
    xTaskCreate(ble_beacon_spam_task, "ble_beacon_spam", 4096, (void*)duration_seconds, 5, NULL);
}
