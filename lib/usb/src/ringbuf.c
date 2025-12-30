#include "ringbuf.h"

uint16_t rb_used(const rb_t* r) { return (uint16_t)(r->head - r->tail); }
uint16_t rb_free(const rb_t* r) { return (uint16_t)(sizeof(r->buf) - rb_used(r)); }

uint16_t rb_write(rb_t* r, const uint8_t* p, uint16_t n)
{
  uint16_t w = 0;
  while (w < n && rb_free(r)) {
    r->buf[r->head % sizeof(r->buf)] = p[w++];
    r->head++;
  }
  return w;
}

uint16_t rb_read(rb_t* r, uint8_t* p, uint16_t n)
{
  uint16_t rd = 0;
  while (rd < n && rb_used(r)) {
    p[rd++] = r->buf[r->tail % sizeof(r->buf)];
    r->tail++;
  }
  return rd;
}
