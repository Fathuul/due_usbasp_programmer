#pragma once

// -----------------------------------------------------------------------------
// Debug hooks (all optional)
// -----------------------------------------------------------------------------

// Enable debug hooks
#define USB_DEBUG 0

#if USB_DEBUG

// Called on USB reset (EORST)
#define USB_DBG_ON_RESET()        do {} while (0)

// Called on SETUP packet
#define USB_DBG_ON_SETUP()        do {} while (0)

// Called on CDC RX/TX
#define USB_DBG_ON_CDC_RX(n)      do { (void)(n); } while (0)
#define USB_DBG_ON_CDC_TX(n)      do { (void)(n); } while (0)

// Called on Vendor RX/TX
#define USB_DBG_ON_VND_RX(n)      do { (void)(n); } while (0)
#define USB_DBG_ON_VND_TX(n)      do { (void)(n); } while (0)

#else

// Compile to nothing
#define USB_DBG_ON_RESET()        do {} while (0)
#define USB_DBG_ON_SETUP()        do {} while (0)
#define USB_DBG_ON_CDC_RX(n)      do {} while (0)
#define USB_DBG_ON_CDC_TX(n)      do {} while (0)
#define USB_DBG_ON_VND_RX(n)      do {} while (0)
#define USB_DBG_ON_VND_TX(n)      do {} while (0)

#endif
