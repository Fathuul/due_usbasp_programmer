#include "usb_hw.h"
#include "usb_config.h"

// -----------------------------------------------------------------------------
// Sanity checks: this HW layer currently supports only these packet sizes
// (EPSIZE encoding is not linear; keep it explicit).
// -----------------------------------------------------------------------------
#if (USB_EP0_MPS != 64)
  #error "USB_EP0_MPS must be 64 for SAM3X UOTGHS FS in this implementation"
#endif

#if (USB_EP_BULK_MPS != 64)
  #error "USB_EP_BULK_MPS must be 64 for SAM3X UOTGHS FS bulk endpoints"
#endif

#if (USB_EP_INT_MPS != 16)
  #error "USB_EP_INT_MPS must be 16 for CDC notification endpoint in this implementation"
#endif

// -----------------------------------------------------------------------------
// Local compatibility defines (if your header lacks them)
// -----------------------------------------------------------------------------
#ifndef UOTGHS_DEVEPTCFG_EPTYPE_Pos
  #define UOTGHS_DEVEPTCFG_EPTYPE_Pos 11
#endif

#ifndef UOTGHS_DEVEPTCFG_EPDIR
  #define UOTGHS_DEVEPTCFG_EPDIR (0x1u << 8)
#endif

#ifndef UOTGHS_DEVEPTCFG_EPTYPE_CTRL
  #define UOTGHS_DEVEPTCFG_EPTYPE_CTRL   (0x0u << UOTGHS_DEVEPTCFG_EPTYPE_Pos)
#endif

#ifndef UOTGHS_DEVEPTCFG_EPTYPE_BLK
  #define UOTGHS_DEVEPTCFG_EPTYPE_BLK    (0x2u << UOTGHS_DEVEPTCFG_EPTYPE_Pos)
#endif

#ifndef UOTGHS_DEVEPTCFG_EPTYPE_INTRPT
  #define UOTGHS_DEVEPTCFG_EPTYPE_INTRPT (0x3u << UOTGHS_DEVEPTCFG_EPTYPE_Pos)
#endif

void usb_enable_clock(void)
{
  PMC->CKGR_UCKR = CKGR_UCKR_UPLLCOUNT(3) | CKGR_UCKR_UPLLEN;
  while (!(PMC->PMC_SR & PMC_SR_LOCKU)) {}

  PMC->PMC_USB = PMC_USB_USBS | PMC_USB_USBDIV(9);
  PMC->PMC_PCER1 = (1UL << (ID_UOTGHS - 32));
}

void usb_hw_init_core(void)
{
  // device mode + pads + controller
  UOTGHS->UOTGHS_CTRL = UOTGHS_CTRL_UIMOD | UOTGHS_CTRL_OTGPADE | UOTGHS_CTRL_USBE;
  UOTGHS->UOTGHS_DEVCTRL = UOTGHS_DEVCTRL_SPDCONF_FORCED_FS;
}

static void ep_cfg_ctrl(uint8_t ep)
{
  (void)ep;
  UOTGHS->UOTGHS_DEVEPTCFG[ep] =
      UOTGHS_DEVEPTCFG_EPSIZE_64_BYTE |
      UOTGHS_DEVEPTCFG_EPTYPE_CTRL |
      UOTGHS_DEVEPTCFG_EPBK_1_BANK |
      UOTGHS_DEVEPTCFG_ALLOC;
}

static void ep_cfg_bulk64(uint8_t ep, uint8_t dir_in)
{
  uint32_t cfg =
      UOTGHS_DEVEPTCFG_EPSIZE_64_BYTE |
      UOTGHS_DEVEPTCFG_EPTYPE_BLK |
      UOTGHS_DEVEPTCFG_EPBK_1_BANK |
      UOTGHS_DEVEPTCFG_ALLOC;

  if (dir_in) cfg |= UOTGHS_DEVEPTCFG_EPDIR;

  UOTGHS->UOTGHS_DEVEPTCFG[ep] = cfg;
}

static void ep_cfg_int16(uint8_t ep, uint8_t dir_in)
{
  uint32_t cfg =
      UOTGHS_DEVEPTCFG_EPSIZE_16_BYTE |
      UOTGHS_DEVEPTCFG_EPTYPE_INTRPT |
      UOTGHS_DEVEPTCFG_EPBK_1_BANK |
      UOTGHS_DEVEPTCFG_ALLOC;

  if (dir_in) cfg |= UOTGHS_DEVEPTCFG_EPDIR;

  UOTGHS->UOTGHS_DEVEPTCFG[ep] = cfg;
}

void ep_write(uint8_t ep, const uint8_t* src, uint16_t len)
{
  volatile uint8_t* fifo = EP_RAM8(ep);
  for (uint16_t i = 0; i < len; i++) *fifo++ = src ? src[i] : 0;
}

void ep_read(uint8_t ep, uint8_t* dst, uint16_t len)
{
  volatile uint8_t* fifo = EP_RAM8(ep);
  for (uint16_t i = 0; i < len; i++) dst[i] = *fifo++;
}

void ep_commit_in(uint8_t ep)
{
  UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_TXINIC;
  UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_FIFOCONC;
}

void ep_release_out(uint8_t ep)
{
  UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_RXOUTIC;
  UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_FIFOCONC;
}

uint16_t ep_out_count(uint32_t epsr)
{
  return (uint16_t)((epsr & UOTGHS_DEVEPTISR_BYCT_Msk) >> UOTGHS_DEVEPTISR_BYCT_Pos);
}

void usb_handle_end_reset(void)
{
  const uint8_t max_ep = USB_EP_VND_DATA_IN; // currently highest used EP number

  // Enable EP0..max_ep (expects contiguous numbering 0..5 as in usb_config.h)
  uint32_t ep_en = 0;
  for (uint8_t ep = 0; ep <= max_ep; ep++) {
    ep_en |= (UOTGHS_DEVEPT_EPEN0 << ep);
  }
  UOTGHS->UOTGHS_DEVEPT = ep_en;

  // Configure endpoints (numbers from usb_config.h)
  ep_cfg_ctrl(0);
  
  // CDC notify (interrupt IN) - configured, but IRQ left disabled by default
  ep_cfg_int16(USB_EP_CDC_NOTIFY_IN, 1);
  
  // CDC data
  ep_cfg_bulk64(USB_EP_CDC_DATA_OUT, 0);
  ep_cfg_bulk64(USB_EP_CDC_DATA_IN,  1);

  // Vendor data
  ep_cfg_bulk64(USB_EP_VND_DATA_OUT, 0);
  ep_cfg_bulk64(USB_EP_VND_DATA_IN,  1);

  // Clear EP status bits
  for (uint8_t ep = 0; ep <= max_ep; ep++) {
    UOTGHS->UOTGHS_DEVEPTIDR[ep] = 0xFFFFFFFFu; // Disable all EP interrupts

    if (ep == 0) {
        // EP0: Clear everything including TXINI to keep it silent until SETUP packet arrives.
        UOTGHS->UOTGHS_DEVEPTICR[ep] = 0xFFFFFFFFu;
    } else {
        // Data Endpoints: Clear everything EXCEPT TXINI.
        // We need TXINI=1 ("Buffer Empty") to remain set, so that the first 
        // cdc_write() can trigger the interrupt.
        UOTGHS->UOTGHS_DEVEPTICR[ep] = ~UOTGHS_DEVEPTICR_TXINIC;
    }
  }

  // EP0: Enable SETUP, IN, OUT interrupts
  UOTGHS->UOTGHS_DEVEPTIER[0] =
      UOTGHS_DEVEPTIER_RXSTPES |
      UOTGHS_DEVEPTIER_TXINES |
      UOTGHS_DEVEPTIER_RXOUTES;

  // OUT endpoints (Enable RX interrupt immediately)
  UOTGHS->UOTGHS_DEVEPTIER[USB_EP_CDC_DATA_OUT] = UOTGHS_DEVEPTIER_RXOUTES;
  UOTGHS->UOTGHS_DEVEPTIER[USB_EP_VND_DATA_OUT] = UOTGHS_DEVEPTIER_RXOUTES;

  // Enable device global interrupt for EP0..max_ep and EORST
  // Skip EP1 (CDC Notify) as it is unused in this demo
  uint32_t devier = UOTGHS_DEVIER_EORSTES;
  for (uint8_t ep = 0; ep <= max_ep; ep++) {
    if (ep == USB_EP_CDC_NOTIFY_IN) continue;
    devier |= (UOTGHS_DEVIER_PEP_0 << ep);
  }
  UOTGHS->UOTGHS_DEVIER = devier;

  // Acknowledge End of Reset
  UOTGHS->UOTGHS_DEVICR = UOTGHS_DEVICR_EORSTC;
}
