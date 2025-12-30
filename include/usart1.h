#pragma once
#include <stdint.h>

typedef struct __attribute__((packed)) {
  uint32_t dwDTERate;
  uint8_t  bCharFormat;
  uint8_t  bParityType;
  uint8_t  bDataBits;
} usb_cdc_line_coding_t;

void usart1_init_dma(void);
void usart1_set_line_coding(const usb_cdc_line_coding_t* coding);
void usart1_get_line_coding(usb_cdc_line_coding_t* coding);

// NEW: Called by USB ISR to trigger UART logic
void usart1_tx_dma_check_and_send(void);