
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"
#include "usb_std.h"


// ---------------------------------------------------------------------------
// Core USBasp Functions (Vendor Requests)
// ---------------------------------------------------------------------------

// ISP session control
#define USBASP_FUNC_CONNECT            1   // H→D : enter programming mode (RESET low)
#define USBASP_FUNC_DISCONNECT         2   // H→D : leave programming mode (RESET high)

// 4-byte SPI transaction
#define USBASP_FUNC_TRANSMIT           3   // H→D→H : send 4 bytes, receive 4 bytes

// Flash / EEPROM access
#define USBASP_FUNC_READFLASH          4   // H→D→H : read flash block
#define USBASP_FUNC_ENABLEPROG         5   // H→D : send programming enable (0xAC53)
#define USBASP_FUNC_WRITEFLASH         6   // H→D : write flash block
#define USBASP_FUNC_READEEPROM         7   // H→D→H : read EEPROM block
#define USBASP_FUNC_WRITEEEPROM        8   // H→D : write EEPROM block

// Extended addressing and SCK control
#define USBASP_FUNC_SETLONGADDRESS     9   // H→D : send 4-byte address (for >64k flash)
#define USBASP_FUNC_SETISPSCK          10  // H→D : set ISP SCK speed index (see table below)

// TPI (Tiny Programming Interface) functions
#define USBASP_FUNC_TPI_CONNECT        11  // H→D : enter TPI mode
#define USBASP_FUNC_TPI_DISCONNECT     12  // H→D : leave TPI mode
#define USBASP_FUNC_TPI_RAWREAD        13  // H→D→H : read 1 byte
#define USBASP_FUNC_TPI_RAWWRITE       14  // H→D : write 1 byte
#define USBASP_FUNC_TPI_READBLOCK      15  // H→D→H : read data block
#define USBASP_FUNC_TPI_WRITEBLOCK     16  // H→D : write data block

// Capabilities query (introduced 2011)
#define USBASP_FUNC_GETCAPABILITIES    127 // D→H : return 4 bytes (bitmask)

// ---------------------------------------------------------------------------
// Optional / Non-standard extensions (used by some clones)
// ---------------------------------------------------------------------------

#define USBASP_FUNC_GETVERSION         126 // D→H : return 2 bytes (major/minor)
#define USBASP_FUNC_GETDEVTYPE         125 // D→H : optional, return device type (rare)

/* ISP SCK speed identifiers */
#define USBASP_ISP_SCK_AUTO   0
#define USBASP_ISP_SCK_0_5    1   /*   0.50 kHz */
#define USBASP_ISP_SCK_1      2   /*   1.00 kHz */
#define USBASP_ISP_SCK_2      3   /*   2.00 kHz */
#define USBASP_ISP_SCK_4      4   /*   4.00 kHz */
#define USBASP_ISP_SCK_8      5   /*   8.00 kHz */
#define USBASP_ISP_SCK_16     6   /*  16.00 kHz */
#define USBASP_ISP_SCK_32     7   /*  32.00 kHz */
#define USBASP_ISP_SCK_93_75  8   /*  93.75 kHz */
#define USBASP_ISP_SCK_187_5  9   /* 187.50 kHz */
#define USBASP_ISP_SCK_375    10  /* 375.00 kHz */
#define USBASP_ISP_SCK_750    11  /* 750.00 kHz */
#define USBASP_ISP_SCK_1500   12  /*   1.50 MHz */
#define USBASP_ISP_SCK_3000   13  /*   3.00 MHz */

/* programming state */
#define PROG_STATE_IDLE         0
#define PROG_STATE_WRITEFLASH   1
#define PROG_STATE_READFLASH    2
#define PROG_STATE_READEEPROM   3
#define PROG_STATE_WRITEEEPROM  4
#define PROG_STATE_TPI_READ     5
#define PROG_STATE_TPI_WRITE    6

/* Block mode flags */
#define PROG_BLOCKFLAG_FIRST    1
#define PROG_BLOCKFLAG_LAST     2

// USBASP capabilities
#define USBASP_CAP_TPI    0x01
#define USBASP_CAP_3MHZ   (1 << 24)     // 3 MHz SCK in UsbAsp-flash firmware

bool usbasp_handle_getcapabilities(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_getversion(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_setispsck(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_connect(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_disconnect(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_enableprog(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_transmit(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_read_flash(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_read_flash_ack(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_write_flash(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_write_flash_ack(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_read_eeprom(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_read_eeprom_ack(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_write_eeprom(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_write_eeprom_ack(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_setlongaddress(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_handle_default(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_send_ok(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_send_nok(uint8_t rhport, usb_setup_req_t const *request);


