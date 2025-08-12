/**
 * @file webserver.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Provides interface to control and communicate with Webserver component
 */
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(WEBSERVER_EVENTS);
enum {
    WEBSERVER_EVENT_ATTACK_REQUEST,
    WEBSERVER_EVENT_ATTACK_RESET
};

/**
 * @brief Struct to deserialize attack request parameters 
 * 
 */
typedef struct {
    // Replace single ap_record_id with array support
    unsigned ap_count;          // Number of APs in the attack
    unsigned *ap_record_ids;    // Array of AP record IDs
    uint8_t type;               //< Chosen type of attack
    uint8_t method;             //< Chosen method of attack
    uint8_t timeout;            //< Attack timeout in seconds
} attack_request_t;

/**
 * @brief Initializes and starts webserver 
 */
void webserver_run();

#endif
