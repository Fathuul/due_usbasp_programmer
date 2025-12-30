#pragma once

// -----------------------------------------------------------------------------
// USB core configuration
// -----------------------------------------------------------------------------

// Enable/disable interfaces
#define USB_ENABLE_CDC        1
#define USB_ENABLE_VENDOR     1

// -----------------------------------------------------------------------------
// Endpoint numbers
// -----------------------------------------------------------------------------

#define USB_EP_CDC_NOTIFY_IN   1
#define USB_EP_CDC_DATA_OUT    2
#define USB_EP_CDC_DATA_IN     3
#define USB_EP_VND_DATA_OUT    4
#define USB_EP_VND_DATA_IN     5

// -----------------------------------------------------------------------------
// Endpoint sizes
// -----------------------------------------------------------------------------

#define USB_EP0_MPS            64
#define USB_EP_BULK_MPS        64
#define USB_EP_INT_MPS         16

// -----------------------------------------------------------------------------
// Default CDC settings
// -----------------------------------------------------------------------------

#define USB_CDC_DEFAULT_BAUD   115200
#define USB_CDC_DEFAULT_DATABITS 8
#define USB_CDC_DEFAULT_PARITY   0   // none
#define USB_CDC_DEFAULT_STOPBITS 0   // 1 stop bit

// -----------------------------------------------------------------------------
// Behaviour toggles
// -----------------------------------------------------------------------------

// Echo RX -> TX by default (useful for bring-up)
// Set to 0 when real application logic is wired
#define USB_CDC_ECHO_DEFAULT  0
#define USB_VND_ECHO_DEFAULT  0
