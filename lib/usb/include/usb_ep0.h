#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ep0_handle_setup(const uint8_t setup[8]);
void ep0_on_txini(void);
void ep0_on_rxouti(uint32_t eps);
void ep0_reset_state(void);

// Send data to host (Control IN)
void ep0_reply_data(const uint8_t* data, uint16_t len);

// Send Zero Length Packet (Status Stage ACK)
void ep0_reply_zlp(void);

// NEW: Prepare EP0 to receive data from Host (Control OUT Data Stage)
void ep0_receive_data(uint8_t* buffer, uint16_t len);

#ifdef __cplusplus
}
#endif