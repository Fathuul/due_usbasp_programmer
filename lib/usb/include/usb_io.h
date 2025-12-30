#pragma once
#include <stdint.h>
#include "ringbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

extern rb_t cdc_rx, cdc_tx;
extern rb_t vnd_rx, vnd_tx;

void cdc_tx_kick(void);
void vnd_tx_kick(void);

#ifdef __cplusplus
}
#endif
