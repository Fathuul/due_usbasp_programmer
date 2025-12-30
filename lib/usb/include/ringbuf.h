#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t buf[1024];
  volatile uint16_t head;
  volatile uint16_t tail;
} rb_t;

uint16_t rb_write(rb_t* r, const uint8_t* p, uint16_t n);
uint16_t rb_read(rb_t* r, uint8_t* p, uint16_t n);
uint16_t rb_used(const rb_t* r);
uint16_t rb_free(const rb_t* r);

#ifdef __cplusplus
}
#endif
