#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Descriptor getter API
const uint8_t* usb_desc_get_device(uint16_t* len);
const uint8_t* usb_desc_get_config(uint16_t* len);
const uint8_t* usb_desc_get_string(uint8_t index, uint16_t* len);
void usb_desc_init_serial(void);

#ifdef __cplusplus
}
#endif
