#pragma once
#include <stdint.h>

// Standard USB Setup Packet (8 Bytes)
// See USB 2.0 Specification, Table 9-2
typedef struct __attribute__((packed)) {
    uint8_t  bmRequestType; // Characteristics of request (Dir, Type, Recipient)
    uint8_t  bRequest;      // Specific request (e.g., GET_DESCRIPTOR)
    uint16_t wValue;        // Word-sized field that varies according to request
    uint16_t wIndex;        // Word-sized field that varies according to request;
                            // typically used to pass an index or offset
    uint16_t wLength;       // Number of bytes to transfer if there is a Data stage
} usb_setup_req_t;

// --- Bitmasks for bmRequestType ---
// D7: Data Transfer Direction
#define USB_REQ_DIR_MASK    0x80
#define USB_REQ_DIR_OUT     0x00 // Host to Device
#define USB_REQ_DIR_IN      0x80 // Device to Host

// D6..5: Type
#define USB_REQ_TYPE_MASK   0x60
#define USB_REQ_TYPE_STD    0x00 // Standard
#define USB_REQ_TYPE_CLASS  0x20 // Class
#define USB_REQ_TYPE_VENDOR 0x40 // Vendor

// D4..0: Recipient
#define USB_REQ_RCPT_MASK   0x1F
#define USB_REQ_RCPT_DEV    0x00 // Device
#define USB_REQ_RCPT_IF     0x01 // Interface
#define USB_REQ_RCPT_EP     0x02 // Endpoint
#define USB_REQ_RCPT_OTHER  0x03 // Other

// --- Standard Requests (bRequest) ---
// See USB 2.0 Specification, Table 9-4
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x03
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIG        0x08
#define USB_REQ_SET_CONFIG        0x09
#define USB_REQ_GET_INTERFACE     0x0A
#define USB_REQ_SET_INTERFACE     0x0B
#define USB_REQ_SYNCH_FRAME       0x0C