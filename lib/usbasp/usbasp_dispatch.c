/**
 * usbasp_dispatch.c
 *
 * Central dispatcher for USBasp protocol requests
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include "usb_std.h"
#include "usbasp_handle.h"



// ---------------------------------------------------------------------------
// Core dispatcher
// ---------------------------------------------------------------------------

bool usbasp_dispatch_setup(uint8_t rhport, usb_setup_req_t const *request)
{

    switch (request->bRequest)
    {
        // ---------------------------
        // Management / information
        // ---------------------------
        case USBASP_FUNC_GETCAPABILITIES:
            return usbasp_handle_getcapabilities(rhport, request);

        case USBASP_FUNC_GETVERSION:
            return usbasp_handle_getversion(rhport, request);

        case USBASP_FUNC_SETISPSCK:
            return usbasp_handle_setispsck(rhport, request);

        // ---------------------------
        // Session control
        // ---------------------------
        case USBASP_FUNC_CONNECT:
            return usbasp_handle_connect(rhport, request);

        case USBASP_FUNC_DISCONNECT:
            return usbasp_handle_disconnect(rhport, request);

        case USBASP_FUNC_ENABLEPROG: 
            return usbasp_handle_enableprog(rhport, request);

        // ---------------------------
        // Core ISP transmit
        // ---------------------------
        case USBASP_FUNC_TRANSMIT: 
            return usbasp_handle_transmit(rhport, request);

        // ---------------------------
        // Read Flash
        // ---------------------------
        case USBASP_FUNC_READFLASH: 
            return usbasp_handle_read_flash(rhport, request);

        // ---------------------------
        // Write Flash
        // ---------------------------
        case USBASP_FUNC_WRITEFLASH: 
            return usbasp_handle_write_flash(rhport, request);

        // ---------------------------
        // Read EEPROM
        // ---------------------------
        case USBASP_FUNC_READEEPROM: 
            return usbasp_handle_read_eeprom(rhport, request);

        // ---------------------------
        // Write EEPROM
        // ---------------------------
        case USBASP_FUNC_WRITEEEPROM: 
            return usbasp_handle_write_eeprom(rhport, request);

        case USBASP_FUNC_SETLONGADDRESS: 
            return usbasp_handle_setlongaddress(rhport, request);            

        // ---------------------------
        // Default
        // ---------------------------
        default:
            return usbasp_handle_default(rhport, request);
    }
}


bool usbasp_dispatch_ack(uint8_t rhport, usb_setup_req_t const *request)
{
    switch (request->bRequest)
    {
        case USBASP_FUNC_READFLASH: 
            return usbasp_handle_read_flash_ack(rhport, request);

        case USBASP_FUNC_WRITEFLASH: 
            return usbasp_handle_write_flash_ack(rhport, request);

        // ---------------------------
        // Read EEPROM
        // ---------------------------
        case USBASP_FUNC_READEEPROM: 
            return usbasp_handle_read_eeprom_ack(rhport, request);

        // ---------------------------
        // Write EEPROM
        // ---------------------------
        case USBASP_FUNC_WRITEEEPROM: 
            return usbasp_handle_write_eeprom_ack(rhport, request);

        default:
            return true;
    }
}