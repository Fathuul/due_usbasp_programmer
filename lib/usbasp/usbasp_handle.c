/**
 * usbasp_proto.c
 *
 * Helper functions for USBasp protocol handling
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include "usbasp_handle.h"
#include "delay.h"
#include "hw_spi.h"
#include "avr_spi.h"
#include "avr_devices_isr.h"
#include "usb_std.h"
#include "usb_ep0.h"

#define usbasp_handle_reply_ack() ep0_reply_zlp()
#define usbasp_handle_reply_data(data, len) ep0_reply_data(data, len)
#define usbasp_handle_receive_data(data, len) ep0_receive_data(data, len)

bool usbasp_send_ok(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void)request;
    static uint8_t ok = 0;
    // return tud_control_xfer(rhport, request, &ok, 1);
    usbasp_handle_reply_data(&ok, 1);
    return true;
}

bool usbasp_send_nok(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void)request;

    static uint8_t nok = 1;
    // return tud_control_xfer(rhport, request, &nok, 1);
    usbasp_handle_reply_data(&nok, 1);
    return true;
}

// ---------------------------------------------------------------------------
// Static capability and version data
// ---------------------------------------------------------------------------

// static const uint8_t usbasp_caps[4] = {0x00, 0x00, 0x00, 0x01};
// static const uint8_t usbasp_caps[4] = {
//        USBASP_CAP_1_SCK | USBASP_CAP_2_SCK_X3 | USBASP_CAP_3_ADDR_32K,
//        0x00, 0x00, 0x00 };

static const uint8_t usbasp_caps[4] = {
    // --- Fischl (2011) kompatibel ---
    // Byte 0 wird direkt interpretiert (8-Bit-Flags)
    //  Bit 0: TPI supported
    //  Bit 1: SCK control available
    //  Bit 2: 3x SCK speed (≈ 3 MHz)
    //  Bit 3: 32 k address (SETLONGADDRESS)
    0x00            // *ToDo* (1 << 0)  // TPI
        | (1 << 1)  // SCK control (für -B)
        | (1 << 2)  // 3× SCK speed (~3 MHz)
        | (1 << 3), // 32k address support (für Mega2560 etc.)

    0x00, 0x00,

    // --- UsbAsp-flash (neuere avrdude-Versionen) ---
    // avrdude 7/8 interpretiert die 32-Bit little-endian Zahl aus res[0..3]
    // Bit 24 = 3 MHz SCK support
    0x01 // entspricht (1 << 24)
};

// Viele Clones (und Avrdude) erwarten eine Versionsantwort (z. B. 2.0)
static const uint8_t usbasp_version[2] = {2, 0};

// --- global / static state ---
static uint32_t rf_addr = 0; // laufende Flash-Adresse (16/32-bit)
// static uint16_t rf_len  = 0;         // Bytes, die dieser Block liefern soll
static uint8_t rf_flags = 0;        // USBASP_BLOCKFLAG_FIRST / _LAST
static bool rf_expect_addr = false; // erwartet OUT-Dataphase mit Startadresse?
static uint8_t rf_addr_buf[2];      // hier landen die 2 Adress-Bytes (klassisch)
static uint32_t long_addr = 0;      // für SETLONGADDRESS (optional)
static bool long_addr_flag = false;
static bool avr_prog_enabled = false;
static uint8_t page_size_mask = 128 - 1;

// --- State für WRITEFLASH (oben global) ---
static uint8_t rw_buf[256];
static uint16_t wf_len = 0;
static uint32_t wf_addr = 0;
static uint8_t wf_flags = 0;

// ---------------------------------------------------------------------------
// Handler-Funktionen
// ---------------------------------------------------------------------------

bool usbasp_handle_getcapabilities(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void)request;
    usbasp_handle_reply_data((void *)usbasp_caps, sizeof(usbasp_caps));
    return true;
}

bool usbasp_handle_getversion(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void)request;
    usbasp_handle_reply_data((void *)usbasp_version, sizeof(usbasp_version));
    return true;
}

bool usbasp_handle_setispsck(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void)request;

    uint8_t sck_idx = (uint8_t)(request->wValue & 0xFF);

    hw_spi_set_br_value(sck_idx);

    static uint8_t ok = 0;

    usbasp_handle_reply_data(&ok, 1);
    return true;
}

bool usbasp_handle_connect(uint8_t rhport, usb_setup_req_t const *request)
{
    
    // Safety: Ensure we start from a clean slate
    hw_spi0_disable(); 
    
    // Re-Enable and Re-Config SPI (resets controller)
    hw_spi0_enable();
    
    // Reset Logic Variables
    avr_spi_reset_ext_addr();
    avr_spi_reset_sig();
    avr_spi_oe_pin_reset(); // enable SN74HCT254 (low active)
    avr_prog_enabled = false;

    return usbasp_send_ok(rhport, request);
}

bool usbasp_handle_disconnect(uint8_t rhport, usb_setup_req_t const *request)
{
    // Reset all internal state flags
    avr_prog_enabled = false;
    long_addr_flag   = false;
    long_addr        = 0;
    rf_addr          = 0;
    wf_addr          = 0;
    wf_len           = 0;
    
    avr_spi_oe_pin_set(); // disable SN74HCT254 (low active)

    // Disable Hardware SPI (High-Z)
    hw_spi0_disable();
    
    // Ensure Reset Pin is released (High)
    avr_spi_reset_pin_set();

    return usbasp_send_ok(rhport, request);
}

bool usbasp_handle_enableprog(uint8_t rhport, usb_setup_req_t const *request)
{
    rf_addr = request->wValue; // word address (low)


    // try it 10 time
    for (int attempt = 0; attempt < 10; ++attempt)
    {
        // RESET-Puls

        avr_spi_reset_pin_set();
        delay_ms(5);
        avr_spi_reset_pin_reset();
        delay_ms(40 + attempt * 2); // increase after each attempt

        uint8_t rv = avr_spi_enableprog();

        // verify the third byte
        if (rv == 0x53)
        {
            avr_prog_enabled = true;
            return usbasp_send_ok(rhport, request);
        }
    }

    avr_prog_enabled = false;

    avr_spi_oe_pin_set(); // disable SN74HCT254 (low active)
    hw_spi0_disable();

    uint8_t tx[4] = {0x00};
    usbasp_handle_reply_data(tx, 4);

    return true;
}

bool usbasp_handle_transmit(uint8_t rhport, usb_setup_req_t const *request)
{     
    (void)rhport;
    
    uint8_t *tx = avr_spi_get_tx_ptr();
    tx[0] = request->wValue & 0xFF;
    tx[1] = request->wValue >> 8;
    tx[2] = request->wIndex & 0xFF;
    tx[3] = request->wIndex >> 8;

    uint8_t *rx = avr_spi_transmit(tx);

    usbasp_handle_reply_data(rx, 4);
    return true;
}

// -----------------------------------------------------------------------------
// READFLASH handler – robust version for real-world use
// -----------------------------------------------------------------------------
bool usbasp_handle_read_flash(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    // Flags aus wIndex high-byte (avrdude: PROG_BLOCKFLAG_FIRST/LAST)
    uint8_t flags = (request->wIndex >> 8) & 0xFF;
    uint16_t n = request->wLength; 
    rf_addr = request->wValue;     // word address (low)

    if (n == 0)
        n = 1; // defensive guard
    if (n > sizeof(rw_buf))
        n = sizeof(rw_buf);

    if (long_addr_flag)
        rf_addr = long_addr;

    avr_spi_check_ext_addr(rf_addr);

    memset(rw_buf, 0xFF, sizeof(rw_buf));

    if (!avr_prog_enabled)
    {
        usbasp_handle_reply_data(rw_buf, n);
        return true;
    }

    for (uint16_t i = 0; i < n; i++)
    {
        rw_buf[i] = avr_spi_read_flash_byte(rf_addr); //  rx[3];
        rf_addr++;
    }

    usbasp_handle_reply_data(rw_buf, n);

    if (flags & PROG_BLOCKFLAG_LAST)
    {
        rf_addr = 0;
    }

    return true;
}

bool usbasp_handle_read_flash_ack(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void)request;

    if (rf_expect_addr)
    {
        rf_expect_addr = false;
        rf_addr = ((uint16_t)rf_addr_buf[0] << 8) | rf_addr_buf[1];
    }

    if (rf_flags & PROG_BLOCKFLAG_LAST)
    {
        rf_addr = 0;
    }

    return true;
}

// -----------------------------------------------------------------------------
// WRITEFLASH handler
// -----------------------------------------------------------------------------
bool usbasp_handle_write_flash(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    wf_flags = (request->wIndex >> 8) & 0xFF;
    uint16_t n = request->wLength;
    wf_addr = request->wValue;

    if (n == 0)
        n = 1;
    if (n > sizeof(rw_buf))
        n = sizeof(rw_buf);
    wf_len = n;

    if (wf_flags & PROG_BLOCKFLAG_FIRST)
    {
        page_size_mask = (((request->wIndex >> 12) & 0x01) << 8 | (request->wIndex & 0x00FF)) - 1;
    }

    if (!avr_prog_enabled)
    {
        usbasp_handle_reply_ack();
        return true;
    }

    if (long_addr_flag)
        wf_addr = long_addr;

    avr_spi_check_ext_addr(wf_addr);

    usbasp_handle_receive_data(rw_buf, wf_len);
    return true;
}

bool usbasp_handle_write_flash_ack(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void)request;
    uint32_t addr = wf_addr;

    for (uint16_t i = 0; i < wf_len; i++)
    {
        avr_spi_write_flash_byte(addr, rw_buf[i]);
        addr++;

        if ((addr & (page_size_mask)) == 0)
            avr_spi_flash_page_commit(addr - 1);
    }

    return true;
}

// -----------------------------------------------------------------------------
// READ-EEPROM handler – robust version for real-world use
// -----------------------------------------------------------------------------
bool usbasp_handle_read_eeprom(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    uint16_t n = request->wLength;
    rf_addr = request->wValue;     // word address (low)

    if (n == 0)
        n = 1; // defensive guard
    if (n > sizeof(rw_buf))
        n = sizeof(rw_buf);

    memset(rw_buf, 0xFF, sizeof(rw_buf));

    if (!avr_prog_enabled)
    {
        usbasp_handle_reply_data(rw_buf, n);
        return true;
    }

    for (uint16_t i = 0; i < n; i++)
    {
        rw_buf[i] = avr_spi_read_eeprom_byte(rf_addr);
        rf_addr++;
    }

    usbasp_handle_reply_data(rw_buf, n);

    return true;
}

bool usbasp_handle_read_eeprom_ack(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void)request;
    return true;
}

// -----------------------------------------------------------------------------
// WRITE-EEPROM handler – robust version for RP2040 USBasp
// -----------------------------------------------------------------------------
bool usbasp_handle_write_eeprom(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    wf_flags = (request->wIndex >> 8) & 0xFF;
    uint16_t n = request->wLength;
    wf_addr = request->wValue;

    if (n == 0)
        n = 1;
    if (n > sizeof(rw_buf))
        n = sizeof(rw_buf);
    wf_len = n;

    if (wf_flags & PROG_BLOCKFLAG_FIRST)
    {
        page_size_mask = (((request->wIndex >> 12) & 0x01) << 8 | (request->wIndex & 0x00FF)) - 1;
    }

    if (!avr_prog_enabled)
    {
        usbasp_handle_reply_ack();
        return true;
    }

    usbasp_handle_receive_data(rw_buf, wf_len);
    return true;
}

bool usbasp_handle_write_eeprom_ack(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void)request;

    uint32_t addr = wf_addr;

    for (uint16_t i = 0; i < wf_len; i++)
    {
        avr_spi_write_eeprom_byte(addr, rw_buf[i]);
        addr++;

        if ((addr & (page_size_mask)) == 0)
            avr_spi_eeprom_page_commit(addr - 1);
    }

    return true;
}

bool usbasp_handle_setlongaddress(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    long_addr = ((uint32_t)request->wIndex << 16) | request->wValue;
    long_addr_flag = true;

    static uint8_t ok[4] = {0};
    usbasp_handle_reply_data(ok, 4);
    return true;
}

bool usbasp_handle_default(uint8_t rhport, usb_setup_req_t const *request)
{
    (void)rhport;
    (void) request;

    usbasp_handle_reply_ack();
    return true;
}
