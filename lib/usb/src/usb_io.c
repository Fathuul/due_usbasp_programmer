#include "usb_cdc_vendor.h"
#include "usb_io.h"
#include "usb_hw.h"
#include "usb_config.h"

// Ring buffers defined in header or elsewhere
rb_t cdc_rx, cdc_tx;
rb_t vnd_rx, vnd_tx;

// --- CDC READ ---
int cdc_read(uint8_t* dst, int maxlen) {
  // Retrieve data from RX ring buffer (filled by ISR)
  return (int)rb_read(&cdc_rx, dst, (uint16_t)maxlen);
}

// --- CDC KICK ---
// Enables the TX interrupt. 
// The ISR will see: "TXINI is set AND enabled -> send data from buffer!"
void cdc_tx_kick(void)
{
  // 1. Enable interrupt at endpoint level
  UOTGHS->UOTGHS_DEVEPTIER[USB_EP_CDC_DATA_IN] = UOTGHS_DEVEPTIER_TXINES;
  
  // 2. Enable interrupt at global level (just to be safe/ensure it's active)
  UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << USB_EP_CDC_DATA_IN);
}

// --- VENDOR KICK ---
void vnd_tx_kick(void)
{
  UOTGHS->UOTGHS_DEVEPTIER[USB_EP_VND_DATA_IN] = UOTGHS_DEVEPTIER_TXINES;
  UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << USB_EP_VND_DATA_IN);
}

// --- CDC WRITE ---
int cdc_write(const uint8_t* src, int len)
{
  if (len <= 0) return 0;

  // 1. Write data to software ring buffer
  int w = (int)rb_write(&cdc_tx, src, (uint16_t)len);

  // 2. Trigger transmission if we actually wrote something
  if (w > 0) {
    cdc_tx_kick();
  }
  
  return w;
}

// --- VENDOR IO ---
int vnd_read(uint8_t* dst, int maxlen) { 
  return (int)rb_read(&vnd_rx, dst, (uint16_t)maxlen); 
}

int vnd_write(const uint8_t* src, int len) {
  int w = (int)rb_write(&vnd_tx, src, (uint16_t)len);
  if (w > 0) {
    vnd_tx_kick();
  }
  return w;
}