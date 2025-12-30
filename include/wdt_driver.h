#pragma once

#include "sam3x8e.h"

// Watchdog Control Register Key (Magic Number required to write to WDT)
#define WDT_KEY (0xA5000000)

/**
 * @brief Reset the Watchdog Timer ("Feed/Kick the dog").
 * Must be called periodically before the timeout expires.
 */
static inline void wdt_reset(void) {
    WDT->WDT_CR = WDT_KEY | WDT_CR_WDRSTT;
}

void wdt_enable(uint32_t ms);