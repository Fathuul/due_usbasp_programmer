#include "uart_dbg.h"
#include "sam3x8e.h"

// UART I/O lines (SAM3X datasheet): URXD=PA8 (Peripheral A), UTXD=PA9 (Peripheral A)
#ifndef UART_DBG_PIO
  #define UART_DBG_PIO        PIOA
#endif

#ifndef UART_DBG_ID_PIO
  #define UART_DBG_ID_PIO     ID_PIOA
#endif

#ifndef UART_DBG_RX_PIN
  #define UART_DBG_RX_PIN     (1u << 8)   // PA8
#endif

#ifndef UART_DBG_TX_PIN
  #define UART_DBG_TX_PIN     (1u << 9)   // PA9
#endif

#ifndef UART_DBG_PERIPH_IS_B
  #define UART_DBG_PERIPH_IS_B 0          // Peripheral A
#endif

static void uart_dbg_config_pins(void)
{
  // Enable PIO clock for A
  PMC->PMC_PCER0 = (1u << UART_DBG_ID_PIO);

  // Hand PA8/PA9 over to peripheral
  UART_DBG_PIO->PIO_PDR = UART_DBG_RX_PIN | UART_DBG_TX_PIN;

  // Select peripheral A (ABSR=0) or B (ABSR=1)
  if (UART_DBG_PERIPH_IS_B) {
    UART_DBG_PIO->PIO_ABSR |=  (UART_DBG_RX_PIN | UART_DBG_TX_PIN);
  } else {
    UART_DBG_PIO->PIO_ABSR &= ~(UART_DBG_RX_PIN | UART_DBG_TX_PIN);
  }

  // No pull-ups by default (optional; enable if you need)
  // UART_DBG_PIO->PIO_PUDR = UART_DBG_RX_PIN | UART_DBG_TX_PIN;
}

static uint32_t uart_dbg_calc_brgr(uint32_t baud)
{
  // UART baud = MCK / (16 * CD)
  if (baud == 0) baud = 115200;

  uint32_t mck = SystemCoreClock;
  uint32_t cd  = (mck + (baud * 8u)) / (baud * 16u); // rounded

  if (cd < 1u) cd = 1u;
  if (cd > 65535u) cd = 65535u;

  return cd;
}

void uart_dbg_init(uint32_t baud)
{
  // Enable UART peripheral clock
  PMC->PMC_PCER0 = (1u << ID_UART);

  uart_dbg_config_pins();

  // Reset & disable
  UART->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS | UART_CR_TXDIS;

  // Mode: normal, no parity
  UART->UART_MR = UART_MR_PAR_NO;

  // Baud
  UART->UART_BRGR = UART_BRGR_CD(uart_dbg_calc_brgr(baud));

  // Enable TX (and RX, harmless even if unused)
  UART->UART_CR = UART_CR_TXEN | UART_CR_RXEN;
}

void uart_dbg_putc(char c)
{
  while ((UART->UART_SR & UART_SR_TXRDY) == 0) {}
  UART->UART_THR = (uint32_t)(uint8_t)c;
}

void uart_dbg_write(const char* s, uint32_t len)
{
  for (uint32_t i = 0; i < len; i++) {
    char c = s[i];
    uart_dbg_putc(c);
  }
}
