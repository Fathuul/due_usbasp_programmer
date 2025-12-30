/**
 * usbasp_dispatch.h
 *
 * Central dispatcher interface for USBasp protocol requests
 * 
 */

#pragma once

#include "usb_std.h"
#include "usbasp_handle.h"
#include <stdio.h>



bool usbasp_dispatch_setup(uint8_t rhport, usb_setup_req_t const *request);
bool usbasp_dispatch_ack(uint8_t rhport, usb_setup_req_t const *request);

uint8_t usbasp_get_current_sck(void);


