#ifndef ATTACK_BEACON_SPAM_H
#define ATTACK_BEACON_SPAM_H

#include "attack.h"

/**
 * @brief Starts BLE Beacon Spam attack.
 * 
 * This attack sends BLE advertising packets simulating iBeacon spam
 * for a specified duration.
 * 
 * @param duration_seconds Duration of the attack in seconds.
 */
void attack_beacon_spam_start(uint8_t count)

/**
 * @brief Stops BLE Beacon Spam attack.
 */
void attack_beacon_spam_stop(void);

#endif // ATTACK_BEACON_SPAM_H

