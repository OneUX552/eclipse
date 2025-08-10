#ifndef ATTACK_BLE_BEACON_H
#define ATTACK_BLE_BEACON_H

#include "attack.h"

/**
 * @brief Starts BLE Beacon Spam attack
 * 
 * @param beacon_count Number of unique beacons to generate
 * @param duration Duration of attack in seconds
 */
void attack_ble_beacon_start(int duration_seconds);
void attack_ble_beacon_stop(void);


#endif
