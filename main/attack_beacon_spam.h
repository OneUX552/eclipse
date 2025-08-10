#ifndef ATTACK_BEACON_SPAM_H
#define ATTACK_BEACON_SPAM_H

#include "attack.h"

/**
 * @brief Starts Wi-Fi Beacon Spam attack.
 * 
 * This attack sends Wi-Fi beacon frames simulating multiple fake APs.
 * 
 * @param count Number of beacon frames to send per timer tick.
 */
void attack_beacon_spam_start(uint8_t count);

/**
 * @brief Stops Wi-Fi Beacon Spam attack.
 */
void attack_beacon_spam_stop(void);

#endif // ATTACK_BEACON_SPAM_H
