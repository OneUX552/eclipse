#ifndef ATTACK_BEACON_SPAM_H
#define ATTACK_BEACON_SPAM_H

#include "attack.h"

/**
 * @brief Starts Beacon Spam attack.
 * 
 * This attack floods the area with fake WiFi networks to disrupt WiFi scanning.
 * 
 * @param beacon_count Number of fake networks to create
 */
void attack_beacon_spam_start(uint8_t beacon_count);

/**
 * @brief Stops Beacon Spam attack.
 */
void attack_beacon_spam_stop();

#endif
