#include "sam3x8e.h"
#include <string.h> 
#include "usb_desc.h"
#include "usb_config.h"


// -----------------------------------------------------------------------------
// Small helpers
// -----------------------------------------------------------------------------
#define U16_LO(x) ((uint8_t)((uint16_t)(x) & 0xFFu))
#define U16_HI(x) ((uint8_t)(((uint16_t)(x) >> 8) & 0xFFu))

// Endpoint address encoding for descriptors
#define EP_ADDR_OUT(num) ((uint8_t)((num) & 0x0Fu))
#define EP_ADDR_IN(num)  ((uint8_t)(0x80u | ((num) & 0x0Fu)))


// -----------------------------------------------------------------------------
// Raw descriptors are private to this translation unit
// -----------------------------------------------------------------------------

static const uint8_t desc_device[] = {
  18, 0x01,
  0x00, 0x02,

  // IAD device (Composite)
  0xEF, 0x02, 0x01,

  // bMaxPacketSize0
  USB_EP0_MPS,

  // idVendor / idProduct (USBasp Standard)
  // VID: 0x16C0 (VOTI)
  // PID: 0x05DC (Obdev's USBasp)
  0xC0, 0x16, 
  0xDC, 0x05,

  // bcdDevice
  0x00, 0x01,

  // iManufacturer, iProduct, iSerialNumber
  1, 2, 3,

  // bNumConfigurations
  1
};

static const uint8_t desc_config[] = {
  // ---------------------------------------------------------------------------
  // Configuration Descriptor
  // ---------------------------------------------------------------------------
  9, 0x02,
  0, 0,          // wTotalLength (patched at runtime)
  3,             // bNumInterfaces
  1,             // bConfigurationValue
  0,             // iConfiguration
  0x80,          // bmAttributes
  50,            // bMaxPower (100mA)

  // ---------------------------------------------------------------------------
  // IAD (CDC): IF0 + IF1
  // ---------------------------------------------------------------------------
  8, 0x0B,
  0, 2,          // first interface, interface count
  0x02, 0x02, 0x01,
  0,

  // ---------------------------------------------------------------------------
  // IF0 CDC Communication Interface
  // ---------------------------------------------------------------------------
  9, 0x04,
  0, 0,
  1,             // bNumEndpoints
  0x02, 0x02, 0x01,
  0,

  // CDC Header Functional Descriptor
  5, 0x24, 0x00,
  0x10, 0x01,

  // Call Management Functional Descriptor
  5, 0x24, 0x01,
  0x00,
  1,

  // ACM Functional Descriptor
  4, 0x24, 0x02,
  0x02,

  // Union Functional Descriptor (Master IF0, Slave IF1)
  5, 0x24, 0x06,
  0, 1,

  // EP1 IN Interrupt (CDC notifications)
  7, 0x05,
  EP_ADDR_IN(USB_EP_CDC_NOTIFY_IN),
  0x03,                                   // Interrupt
  U16_LO(USB_EP_INT_MPS), U16_HI(USB_EP_INT_MPS),
  10,

  // ---------------------------------------------------------------------------
  // IF1 CDC Data Interface
  // ---------------------------------------------------------------------------
  9, 0x04,
  1, 0,
  2,             // bNumEndpoints
  0x0A, 0x00, 0x00,
  0,

  // EP2 OUT Bulk (CDC data OUT)
  7, 0x05,
  EP_ADDR_OUT(USB_EP_CDC_DATA_OUT),
  0x02,                                   // Bulk
  U16_LO(USB_EP_BULK_MPS), U16_HI(USB_EP_BULK_MPS),
  0,

  // EP3 IN Bulk (CDC data IN)
  7, 0x05,
  EP_ADDR_IN(USB_EP_CDC_DATA_IN),
  0x02,                                   // Bulk
  U16_LO(USB_EP_BULK_MPS), U16_HI(USB_EP_BULK_MPS),
  0,

  // ---------------------------------------------------------------------------
  // IF2 Vendor Interface
  // ---------------------------------------------------------------------------
  9, 0x04,
  2, 0,
  2,             // bNumEndpoints
  0xFF, 0x00, 0x00,
  0,

  // EP4 OUT Bulk (Vendor OUT)
  7, 0x05,
  EP_ADDR_OUT(USB_EP_VND_DATA_OUT),
  0x02,                                   // Bulk
  U16_LO(USB_EP_BULK_MPS), U16_HI(USB_EP_BULK_MPS),
  0,

  // EP5 IN Bulk (Vendor IN)
  7, 0x05,
  EP_ADDR_IN(USB_EP_VND_DATA_IN),
  0x02,                                   // Bulk
  U16_LO(USB_EP_BULK_MPS), U16_HI(USB_EP_BULK_MPS),
  0
};

static uint8_t  desc_config_tmp[sizeof(desc_config)];
static uint16_t str_buf[32];


static const char* str_table[] = {
  (const char[]){ 0x09, 0x04 },   // LangID: 0x0409
  "Saryndor Dev.",
  "USBasp+CDC",
  "49695256-670c-41fe-8271-25b9085f4070"
};

static const uint32_t str_table_count =
  sizeof(str_table) / sizeof(str_table[0]);

const uint8_t* usb_desc_get_device(uint16_t* len)
{
  if (len) *len = (uint16_t)sizeof(desc_device);
  return desc_device;
}

const uint8_t* usb_desc_get_config(uint16_t* len)
{
  memcpy(desc_config_tmp, desc_config, sizeof(desc_config));

  // Patch wTotalLength (LE16)
  desc_config_tmp[2] = U16_LO(sizeof(desc_config));
  desc_config_tmp[3] = U16_HI(sizeof(desc_config));

  if (len) *len = (uint16_t)sizeof(desc_config);
  return desc_config_tmp;
}


const uint8_t* usb_desc_get_string(uint8_t index, uint16_t* len)
{
  if (index >= str_table_count) return NULL;

  // Language ID
  if (index == 0) {
    str_buf[1] = (uint16_t)(
      (uint8_t)str_table[0][0] |
      ((uint16_t)(uint8_t)str_table[0][1] << 8));
    str_buf[0] = (uint16_t)((0x03 << 8) | 4);
    if (len) *len = 4;
    return (const uint8_t*)str_buf;
  }

  // Other Strings
  const char* s = str_table[index];
  uint8_t n = 0;
  while (s[n] && n < 31) n++;

  str_buf[0] = (uint16_t)((0x03 << 8) | (2u*n + 2u));
  for (uint8_t i = 0; i < n; i++) {
    str_buf[1 + i] = (uint16_t)s[i];
  }

  if (len) *len = (uint16_t)(2u*n + 2u);
  return (const uint8_t*)str_buf;
}