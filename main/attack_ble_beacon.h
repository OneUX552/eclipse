#ifndef ATTACK_BLE_BEACON_H
#define ATTACK_BLE_BEACON_H

#include <stdint.h>

/**
 * @brief Starts BLE Beacon Spam attack
 * 
 * @param duration_seconds Duration of attack in seconds
 */
void attack_ble_beacon_start(int duration_seconds);

/**
 * @brief Stops BLE Beacon Spam attack
 */
void attack_ble_beacon_stop(void);

#endif // ATTACK_BLE_BEACON_H
