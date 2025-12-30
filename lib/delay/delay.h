#pragma once
#include <stdint.h>
#include "sam3x8e.h"

// CoreDebug DEMCR
#define COREDEBUG_DEMCR      (*(volatile uint32_t*)0xE000EDFCu)
#define COREDEBUG_TRCENA     (1u << 24)

// DWT registers
#define DWT_CTRL             (*(volatile uint32_t*)0xE0001000u)
#define DWT_CYCCNT           (*(volatile uint32_t*)0xE0001004u)
#define DWT_CYCCNTENA        (1u << 0)

extern uint32_t ticks_per_us;

void delay_init(void);

// Helper: Convert milliseconds to DWT ticks
// Safe for ms up to ~50 seconds at 84MHz
static inline uint32_t get_ticks_ms(uint32_t ms)
{
    return ms * 1000 * ticks_per_us;
}

static inline void delay_us(uint32_t us)
{
  uint32_t start = DWT_CYCCNT;
  uint32_t ticks = ticks_per_us * us;
  
  // Robust wait using unsigned subtraction difference
  // Works correctly even if DWT_CYCCNT overflows during the wait
  while ((DWT_CYCCNT - start) < ticks) {
      // Busy wait
      __ASM volatile ("nop");
  }
}

static inline void delay_ms(uint32_t ms) {
    // We can just loop delay_us, or implement logic similar to above.
    // Looping is simple enough here.
    for (uint32_t i = 0; i < ms; i++) {
        delay_us(1000);
    }
}