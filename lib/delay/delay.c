#include "delay.h"

uint32_t ticks_per_us;

void delay_init(void)
{
  COREDEBUG_DEMCR |= COREDEBUG_TRCENA;
  DWT_CYCCNT = 0;
  DWT_CTRL |= DWT_CYCCNTENA;
  ticks_per_us = (SystemCoreClock / 1000000u);
}