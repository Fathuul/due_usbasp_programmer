#include "sam3x8e.h"
#include "usb_cdc_vendor.h"
#include "uart_dbg.h"
#include "delay.h"
#include "usart1.h"
#include "avr_spi.h"
#include "wdt_driver.h"
#include "usb_desc.h"

int main(void)
{
  // WDT->WDT_MR = WDT_MR_WDDIS;

  SystemInit();
  // uart_dbg_init(115200);
  delay_init();

  usb_cdc_vendor_init();

  usart1_init_dma(); 
  
  __enable_irq();

  avr_spi_reset_pin_init();
  avr_spi_reset_pin_set();
  avr_spi_oe_pin_init();
  avr_spi_oe_pin_set(); // disable HCT254 (low active)

  wdt_enable(3000);

  while (1)
  {
    delay_ms(1);  
    wdt_reset();  
  }
}
