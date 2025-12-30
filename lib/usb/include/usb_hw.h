#pragma once
#include <stdint.h>
#include "sam3x8e.h"
#include "usb_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// UOTGHS DPRAM access (byte-wise)
#define EP_RAM8(ep)   (((volatile uint8_t (*)[0x8000])UOTGHS_RAM_ADDR)[(ep)])

// Hardware API
void usb_enable_clock(void);
void usb_hw_init_core(void);
void usb_handle_end_reset(void);

void ep_write(uint8_t ep, const uint8_t* src, uint16_t len);
void ep_read(uint8_t ep, uint8_t* dst, uint16_t len);

void ep_commit_in(uint8_t ep);
void ep_release_out(uint8_t ep);

uint16_t ep_out_count(uint32_t epsr);

#ifdef __cplusplus
}
#endif
