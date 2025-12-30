#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void usb_cdc_vendor_init(void);

int  cdc_read(uint8_t* dst, int maxlen);
int  cdc_write(const uint8_t* src, int len);

int  vnd_read(uint8_t* dst, int maxlen);
int  vnd_write(const uint8_t* src, int len);

#ifdef __cplusplus
}
#endif
