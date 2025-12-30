#include "usb_ep0.h"
#include "usb_hw.h"
#include "usb_desc.h"
#include "usb_config.h"
#include "usb_std.h"
#include <string.h>
#include "usart1.h"
#include "usbasp_dispatch.h"

// EP0 transfer state
static volatile uint8_t pending_address = 0;
static volatile uint8_t address_pending = 0;

static const uint8_t *ep0_in_ptr = NULL;
static volatile uint16_t ep0_in_rem = 0;

static uint8_t *ep0_out_ptr = NULL;
static volatile uint16_t ep0_out_rem = 0;
static volatile uint8_t ep0_out_expect_status_in = 0;
static usb_setup_req_t ep0_saved_request;

// CDC Line Coding
typedef struct __attribute__((packed))
{
  uint32_t dwDTERate;
  uint8_t bCharFormat;
  uint8_t bParityType;
  uint8_t bDataBits;
} cdc_line_coding_t;

static volatile cdc_line_coding_t g_line_coding = {
    .dwDTERate = USB_CDC_DEFAULT_BAUD,
    .bCharFormat = USB_CDC_DEFAULT_STOPBITS,
    .bParityType = USB_CDC_DEFAULT_PARITY,
    .bDataBits = USB_CDC_DEFAULT_DATABITS};

static volatile uint8_t g_dtr = 0;
static volatile uint8_t g_rts = 0;

// Temp storage for incoming SET_LINE_CODING data
static usb_cdc_line_coding_t ep0_line_coding_tmp;
static uint8_t ep0_is_line_coding_request = 0;

static void ep0_send_next_in_packet(void)
{

  uint16_t n = (ep0_in_rem > USB_EP0_MPS) ? USB_EP0_MPS : ep0_in_rem;

  if (n > 0)
  {
    ep_write(0, ep0_in_ptr, n);
    ep0_in_ptr += n;
    ep0_in_rem -= n;
  }
  else
  {
    ep_write(0, NULL, 0);
  }

  ep_commit_in(0);

  if (n < USB_EP0_MPS && !address_pending)
  {
    UOTGHS->UOTGHS_DEVEPTIDR[0] = UOTGHS_DEVEPTIDR_TXINEC;
    ep0_in_ptr = NULL;
  }
}

static void ep0_stall(void)
{
  UOTGHS->UOTGHS_DEVEPTIER[0] = UOTGHS_DEVEPTIER_STALLRQS;
  UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);
}

static inline void ep0_arm_out(void)
{
  UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_RXOUTES;
  UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);
}

void ep0_reset_state(void)
{
  ep0_in_ptr = NULL;
  ep0_in_rem = 0;
  ep0_out_ptr = NULL;
  ep0_out_rem = 0;
  ep0_out_expect_status_in = 0;
  pending_address = 0;
  address_pending = 0;
}

// -----------------------------------------------------------------------------
// Implementation of ep0_receive_data
// -----------------------------------------------------------------------------
void ep0_receive_data(uint8_t *buffer, uint16_t len)
{
  ep0_out_ptr = buffer;
  ep0_out_rem = len;

  // We expect to send a Status IN (ZLP) AFTER we received all data
  ep0_out_expect_status_in = 1;

  // Enable RXOUT interrupt for EP0 to actually receive the data packets
  UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_RXOUTES;
  UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);
}

void ep0_handle_setup(const uint8_t setup[8])
{
  const uint8_t bmRequestType = setup[0];
  const uint8_t bRequest = setup[1];
  const uint16_t wValue = (uint16_t)setup[2] | ((uint16_t)setup[3] << 8);
  const uint16_t wIndex = (uint16_t)setup[4] | ((uint16_t)setup[5] << 8);
  const uint16_t wLength = (uint16_t)setup[6] | ((uint16_t)setup[7] << 8);

  // Save request parameters for later use in ep0_on_rxouti (dispatch_ack)
  ep0_saved_request.bmRequestType = bmRequestType;
  ep0_saved_request.bRequest = bRequest;
  ep0_saved_request.wValue = wValue;
  ep0_saved_request.wIndex = wIndex;
  ep0_saved_request.wLength = wLength;

  // Reset transfer state
  ep0_out_ptr = NULL;
  ep0_out_rem = 0;
  ep0_out_expect_status_in = 0;

  // Reset Line Coding Flag by default (will be set only for SET_LINE_CODING)
  ep0_is_line_coding_request = 0;

  // ---------------------------------------------------------------------------
  // Standard requests
  // ---------------------------------------------------------------------------
  if ((bmRequestType & 0x60) == 0x00)
  {
    // GET_DESCRIPTOR
    if (bRequest == 0x06)
    {
      const uint8_t desc_type = (uint8_t)(wValue >> 8);
      const uint8_t desc_idx = (uint8_t)(wValue & 0xFF);

      const uint8_t *p = NULL;
      uint16_t len = 0;

      if (desc_type == 1)
      {
        p = usb_desc_get_device(&len);
      }
      else if (desc_type == 2)
      {
        p = usb_desc_get_config(&len);
      }
      else if (desc_type == 3)
      {
        p = usb_desc_get_string(desc_idx, &len);
      }
      else
      {
        ep0_stall();
        return;
      }

      if (!p)
      {
        ep0_stall();
        return;
      }

      if (len > wLength)
        len = wLength;

      ep0_in_ptr = p;
      ep0_in_rem = len;

      UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_TXINES;
      UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);

      if (UOTGHS->UOTGHS_DEVEPTISR[0] & UOTGHS_DEVEPTISR_TXINI)
        ep0_send_next_in_packet();
      return;
    }

    // SET_ADDRESS
    if (bRequest == 0x05)
    {
      pending_address = (uint8_t)(wValue & 0x7F);
      address_pending = 1;

      ep0_in_ptr = NULL;
      ep0_in_rem = 0;

      UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_TXINES;
      UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);

      if (UOTGHS->UOTGHS_DEVEPTISR[0] & UOTGHS_DEVEPTISR_TXINI)
        ep0_send_next_in_packet();
      return;
    }

    // SET_CONFIGURATION
    if (bRequest == 0x09)
    {
      ep0_in_ptr = NULL;
      ep0_in_rem = 0;

      UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_TXINES;
      UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);

      if (UOTGHS->UOTGHS_DEVEPTISR[0] & UOTGHS_DEVEPTISR_TXINI)
        ep0_send_next_in_packet();
      return;
    }

    // GET_CONFIGURATION
    if (bRequest == 0x08)
    {
      static uint8_t cfg = 1;
      ep0_in_ptr = &cfg;
      ep0_in_rem = (wLength < 1) ? wLength : 1;

      UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_TXINES;
      UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);

      if (UOTGHS->UOTGHS_DEVEPTISR[0] & UOTGHS_DEVEPTISR_TXINI)
        ep0_send_next_in_packet();
      return;
    }

    ep0_stall();
    return;
  }

  // ---------------------------------------------------------------------------
  // CDC Class requests (Interface 0)
  // ---------------------------------------------------------------------------
  // Check for Class Request (0x20)
  if ((bmRequestType & 0x60) == 0x20)
  {
    if ((uint8_t)wIndex != 0)
    {
      ep0_stall();
      return;
    }

    // SET_LINE_CODING (0x20): OUT 7 bytes
    // Host sends Baudrate/Parity -> We must receive it
    if (bRequest == 0x20 && (bmRequestType & 0x80) == 0x00 && wLength == 7)
    {
      // Point to our TEMP buffer, not the live config yet
      ep0_out_ptr = (uint8_t *)&ep0_line_coding_tmp;
      ep0_out_rem = 7;

      // Important: Set flag so we know to apply settings when data arrives completely
      ep0_is_line_coding_request = 1;

      ep0_out_expect_status_in = 1;
      ep0_arm_out();
      return;
    }

    // GET_LINE_CODING (0x21): IN 7 bytes
    // Host asks for current Baudrate -> We fetch from HW driver
    if (bRequest == 0x21 && (bmRequestType & 0x80) == 0x80 && wLength == 7)
    {
      // 1. Update temp struct with current hardware settings
      usart1_get_line_coding(&ep0_line_coding_tmp);

      // 2. Send it to Host
      ep0_in_ptr = (const uint8_t *)&ep0_line_coding_tmp;
      ep0_in_rem = 7;

      UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_TXINES;
      UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);

      if (UOTGHS->UOTGHS_DEVEPTISR[0] & UOTGHS_DEVEPTISR_TXINI)
        ep0_send_next_in_packet();
      return;
    }

    // SET_CONTROL_LINE_STATE (0x22): no data
    if (bRequest == 0x22 && (bmRequestType & 0x80) == 0x00 && wLength == 0)
    {
      // Optional: Store DTR/RTS state if needed for flow control later
      // g_dtr = (wValue & 0x0001) ? 1 : 0;
      // g_rts = (wValue & 0x0002) ? 1 : 0;

      ep0_in_ptr = NULL;
      ep0_in_rem = 0;

      UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_TXINES;
      UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);

      if (UOTGHS->UOTGHS_DEVEPTISR[0] & UOTGHS_DEVEPTISR_TXINI)
        ep0_send_next_in_packet();
      return;
    }

    ep0_stall();
    return;
  }

  // ---------------------------------------------------------------------------
  // VENDOR Requests (USBasp) -> Delegate to an Dispatcher
  // ---------------------------------------------------------------------------
  if ((bmRequestType & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_VENDOR)
  {
    // Cast raw data buffer
    if (usbasp_dispatch_setup(0, (const usb_setup_req_t *)setup))
    {
      return;
    }
    else
    {
      ep0_stall();
      return;
    }
  }

  ep0_stall();
}

void ep0_on_txini(void)
{
  // Apply pending address after status stage
  if (address_pending && ep0_in_ptr == NULL && ep0_in_rem == 0)
  {
    uint32_t devctrl = UOTGHS->UOTGHS_DEVCTRL;
    devctrl &= ~UOTGHS_DEVCTRL_UADD_Msk;
    devctrl |= UOTGHS_DEVCTRL_UADD(pending_address) | UOTGHS_DEVCTRL_ADDEN;
    devctrl &= ~UOTGHS_DEVCTRL_SPDCONF_Msk;
    devctrl |= UOTGHS_DEVCTRL_SPDCONF_FORCED_FS;
    UOTGHS->UOTGHS_DEVCTRL = devctrl;

    address_pending = 0;
    pending_address = 0;
  }

  if (ep0_in_ptr || ep0_in_rem == 0)
  {
    ep0_send_next_in_packet();
    ep0_arm_out();
  }
}

void ep0_on_rxouti(uint32_t eps)
{
  uint16_t cnt = ep_out_count(eps);

  if (ep0_out_ptr && ep0_out_rem)
  {
    uint16_t n = cnt;
    if (n > ep0_out_rem)
      n = ep0_out_rem;

    if (n)
    {
      ep_read(0, ep0_out_ptr, n);
      ep0_out_ptr += n;
      ep0_out_rem -= n;
    }

    ep_release_out(0);

    if (ep0_out_rem == 0 && ep0_out_expect_status_in)
    {
      ep0_in_ptr = NULL;
      ep0_in_rem = 0;
      if (UOTGHS->UOTGHS_DEVEPTISR[0] & UOTGHS_DEVEPTISR_TXINI)
        ep0_send_next_in_packet();
    }

    if (ep0_out_rem == 0 && ep0_out_expect_status_in)
    {
      // Data stage is done. If this was a Vendor Write Request,
      // we must trigger the actual action (e.g. writing to Flash).
      if ((ep0_saved_request.bmRequestType & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_VENDOR)
      {
        // Call the ACK handler in usbasp_dispatch.c
        // This function will perform the actual SPI flash write using the data now in the buffer.
        usbasp_dispatch_ack(0, &ep0_saved_request);
      }

      if (ep0_is_line_coding_request)
      {
        ep0_is_line_coding_request = 0;
        usart1_set_line_coding(&ep0_line_coding_tmp);
      }

      ep0_in_ptr = NULL;
      ep0_in_rem = 0;

      // Trigger ZLP
      UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_TXINES;
      UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);

      if (UOTGHS->UOTGHS_DEVEPTISR[0] & UOTGHS_DEVEPTISR_TXINI)
        ep0_send_next_in_packet();
    }

    return;
  }

  // status OUT
  ep_release_out(0);
}

void ep0_reply_data(const uint8_t *data, uint16_t len)
{
  ep0_in_ptr = data;
  ep0_in_rem = len;

  // Enable interrupts to start the transmission process
  UOTGHS->UOTGHS_DEVEPTIER[0] |= UOTGHS_DEVEPTIER_TXINES;
  UOTGHS->UOTGHS_DEVIER |= (UOTGHS_DEVIER_PEP_0 << 0);

  // If the hardware buffer is already empty (TXINI set), send the first packet immediately
  if (UOTGHS->UOTGHS_DEVEPTISR[0] & UOTGHS_DEVEPTISR_TXINI)
  {
    ep0_send_next_in_packet();
  }
}

void ep0_reply_zlp(void)
{
  // Sending a ZLP (Zero Length Packet) is equivalent to sending 0 bytes of data
  ep0_reply_data(NULL, 0);
}