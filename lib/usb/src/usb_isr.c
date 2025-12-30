#include "usb_isr.h"
#include "usb_hw.h"
#include "usb_ep0.h"
#include "usb_io.h"
#include "usb_config.h"
#include "usb_debug.h"
#include "usart1.h"

// Define TXINEC if missing (often bit 0 in DEVEPTIDR)
#ifndef UOTGHS_DEVEPTIDR_TXINEC
  #define UOTGHS_DEVEPTIDR_TXINEC (1u << 0)
#endif

static void ep_send_from_rb(uint8_t ep, rb_t* txrb)
{
  uint8_t tmp[USB_EP_BULK_MPS]; // 64 byte buffer
  
  // Try to read up to 64 bytes from ring buffer
  uint16_t n = rb_read(txrb, tmp, USB_EP_BULK_MPS);

  if (n == 0) {
    // Buffer empty -> Mask (disable) interrupt!
    // Otherwise the hardware triggers the ISR immediately again (IRQ Storm)
    // because TXINI (Buffer Empty) stays set.
    UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_TXINEC;
   
    return;
  }

  // Write data to hardware FIFO
  ep_write(ep, tmp, n);
  
  // Commit packet (bank switch)
  ep_commit_in(ep);
  
  // Note: We do NOT disable the IRQ here. 
  // We wait for the next "TXINI" interrupt to send the next chunk.
}



void UOTGHS_Handler(void)
{
  uint32_t s = UOTGHS->UOTGHS_DEVISR;
  if (s & UOTGHS_DEVISR_EORST) {
    USB_DBG_ON_RESET();
    usb_handle_end_reset();
    ep0_reset_state();
    // Ack reset is done in usb_handle_end_reset via DEVICR
    return;
  }

  // EP0 Handling
  if (s & (UOTGHS_DEVISR_PEP_0 << 0))
  {
    uint32_t eps = UOTGHS->UOTGHS_DEVEPTISR[0];
    uint32_t epm = UOTGHS->UOTGHS_DEVEPTIMR[0]; // Read Interrupt Mask

    // SETUP packet always has priority and is not masked usually
    if (eps & UOTGHS_DEVEPTISR_RXSTPI)
    {
      uint8_t setup[8];
      ep_read(0, setup, 8);
      UOTGHS->UOTGHS_DEVEPTICR[0] = UOTGHS_DEVEPTICR_RXSTPIC | UOTGHS_DEVEPTICR_RXOUTIC;
      USB_DBG_ON_SETUP();
      ep0_handle_setup(setup);
      return;
    }

    // Check TXINI only if enabled in Mask
    if ((eps & UOTGHS_DEVEPTISR_TXINI) && (epm & UOTGHS_DEVEPTIMR_TXINE)) { 
      ep0_on_txini(); 
      return;
    }

    // Check RXOUTI only if enabled in Mask
    if ((eps & UOTGHS_DEVEPTISR_RXOUTI) && (epm & UOTGHS_DEVEPTIMR_RXOUTE)) { 
      ep0_on_rxouti(eps); 
      return;
    }
  }

  
  // CDC RX EP2 OUT
  if (s & (UOTGHS_DEVISR_PEP_0 << USB_EP_CDC_DATA_OUT))
  {
    uint32_t eps = UOTGHS->UOTGHS_DEVEPTISR[USB_EP_CDC_DATA_OUT];
    if (eps & UOTGHS_DEVEPTISR_RXOUTI)
    {
      uint16_t cnt = ep_out_count(eps);
      uint8_t tmp[USB_EP_BULK_MPS];
      if (cnt > USB_EP_BULK_MPS) cnt = USB_EP_BULK_MPS;

      if (cnt) {
        ep_read(USB_EP_CDC_DATA_OUT, tmp, cnt);
        rb_write(&cdc_rx, tmp, cnt);
        
        // EVENT DRIVEN CHANGE:
        // We received data. If UART DMA is idle, kick it off immediately!
        // This removes the need for polling in main().
        usart1_tx_dma_check_and_send(); 
      }

      ep_release_out(USB_EP_CDC_DATA_OUT);
    }
  }

  // CDC TX EP3 IN
  if (s & (UOTGHS_DEVISR_PEP_0 << USB_EP_CDC_DATA_IN))
  {
    uint32_t eps = UOTGHS->UOTGHS_DEVEPTISR[USB_EP_CDC_DATA_IN];
    uint32_t epm = UOTGHS->UOTGHS_DEVEPTIMR[USB_EP_CDC_DATA_IN]; // Read Mask

    // Only allow send if TXINI is set AND TX Interrupt is enabled
    if ((eps & UOTGHS_DEVEPTISR_TXINI) && (epm & UOTGHS_DEVEPTIMR_TXINE)) {
      ep_send_from_rb(USB_EP_CDC_DATA_IN, &cdc_tx);
    }
  }

  // Vendor RX EP4 OUT
  if (s & (UOTGHS_DEVISR_PEP_0 << USB_EP_VND_DATA_OUT))
  {
    uint32_t eps = UOTGHS->UOTGHS_DEVEPTISR[USB_EP_VND_DATA_OUT];
    if (eps & UOTGHS_DEVEPTISR_RXOUTI)
    {
      uint16_t cnt = ep_out_count(eps);
      uint8_t tmp[USB_EP_BULK_MPS];
      if (cnt > USB_EP_BULK_MPS) cnt = USB_EP_BULK_MPS;

      if (cnt) {
        ep_read(USB_EP_VND_DATA_OUT, tmp, cnt);
        rb_write(&vnd_rx, tmp, cnt);
        USB_DBG_ON_VND_RX(cnt);
#if USB_VND_ECHO_DEFAULT
        rb_write(&vnd_tx, tmp, cnt);
        // default echo
        vnd_tx_kick();
#endif
      }

      ep_release_out(USB_EP_VND_DATA_OUT);
    }
  }

  // Vendor TX EP5 IN
  if (s & (UOTGHS_DEVISR_PEP_0 << USB_EP_VND_DATA_IN))
  {
    uint32_t eps = UOTGHS->UOTGHS_DEVEPTISR[USB_EP_VND_DATA_IN];
    uint32_t epm = UOTGHS->UOTGHS_DEVEPTIMR[USB_EP_VND_DATA_IN]; // Read Mask

    // Only allow send if TXINI is set AND TX Interrupt is enabled
    if ((eps & UOTGHS_DEVEPTISR_TXINI) && (epm & UOTGHS_DEVEPTIMR_TXINE)) {
      ep_send_from_rb(USB_EP_VND_DATA_IN, &vnd_tx);
    }
  }
  
  // Acknowledge interrupt at device level (clears flags handled by hardware)
  UOTGHS->UOTGHS_DEVICR = s; 
}