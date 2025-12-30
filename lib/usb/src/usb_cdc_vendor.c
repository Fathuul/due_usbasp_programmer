#include "usb_cdc_vendor.h"
#include "usb_hw.h"
#include "usb_ep0.h"
#include "delay.h"

void usb_cdc_vendor_init(void)
{
  usb_enable_clock();
  usb_hw_init_core();

  UOTGHS->UOTGHS_DEVCTRL |= UOTGHS_DEVCTRL_DETACH;
  delay_ms(20);

  NVIC_ClearPendingIRQ(UOTGHS_IRQn);
  NVIC_SetPriority(UOTGHS_IRQn, 1);
  NVIC_EnableIRQ(UOTGHS_IRQn);

  // Enable end-of-reset interrupt
  UOTGHS->UOTGHS_DEVIER = UOTGHS_DEVIER_EORSTES;

  // Connect
  UOTGHS->UOTGHS_DEVCTRL &= ~UOTGHS_DEVCTRL_DETACH;

  ep0_reset_state();
}
