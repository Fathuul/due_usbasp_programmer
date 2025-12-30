#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void uart_dbg_init(uint32_t baud);
void uart_dbg_putc(char c);
void uart_dbg_write(const char* s, uint32_t len);

#ifdef __cplusplus
}
#endif
