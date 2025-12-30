#include "wdt_driver.h"


/**
 * @brief Enable the Watchdog Timer with a specific timeout.
 * * @note On the SAM3X, the WDT_MR register can only be written ONCE 
 * after a processor reset. Ensure it is not disabled in SystemInit().
 * * @param ms Timeout in milliseconds (max approx. 16000 ms)
 */
void wdt_enable(uint32_t ms) {
    // Limits of SAM3X WDT:
    // The counter is 12-bit (0..4095).
    // The clock is SLCK (32.768 kHz) / 128 = 256 Hz.
    // Max timeout is approx. 16 seconds (4095 / 256).
    
    if (ms > 16000) ms = 16000;
    if (ms < 1) ms = 1;

    // Calculate WDV (Watchdog Value)
    // Formula: WDV = (ms * 256) / 1000
    uint32_t wdv = (ms * 256) / 1000;

    // Configure WDT_MR (Mode Register)
    // WDRSTEN: Reset Enable (Trigger a processor reset on timeout)
    // WDDIS:   0 (We want to enable it, so this bit must be 0)
    // WDV:     Set the counter value for the timeout
    // WDD:     Delta Value (Write Disable Delta). 
    //          Must be >= WDV to allow resetting at any time. 
    //          We set it to WDV (or 0xFFF) to allow immediate resets.
    WDT->WDT_MR = WDT_MR_WDRSTEN | WDT_MR_WDV(wdv) | WDT_MR_WDD(wdv);
}

