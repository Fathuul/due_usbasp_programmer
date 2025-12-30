#include "sam3x8e.h"
#include "usart1.h"
#include "usb_cdc_vendor.h" // To call cdc_write from ISR
#include "usb_io.h"
#include <string.h>

// --- Configuration ---
#define USART1_RX_BUF_SIZE  256 
#define PIN_RXD1 (1u << 12)
#define PIN_TXD1 (1u << 13)

// Circular DMA Buffer for RX
static uint8_t dma_rx_buffer[USART1_RX_BUF_SIZE];
static volatile uint32_t dma_rx_tail = 0;

// Temporary buffer for copying from Ringbuffer to DMA (TX)
// DMA needs a contiguous block, Ringbuffer might wrap.
static uint8_t dma_tx_buffer[256]; 

static usb_cdc_line_coding_t current_coding = {
    .dwDTERate = 115200, .bCharFormat = 0, .bParityType = 0, .bDataBits = 8
};

void usart1_init_dma(void)
{
  // 1. Clock & Pins
  PMC->PMC_PCER0 = (1u << ID_USART1);
  PIOA->PIO_PDR  = PIN_RXD1 | PIN_TXD1;
  PIOA->PIO_ABSR &= ~(PIN_RXD1 | PIN_TXD1);

  // 2. Reset & Config
  USART1->US_CR = US_CR_RSTRX | US_CR_RSTTX | US_CR_RXDIS | US_CR_TXDIS;
  USART1->US_MR = US_MR_USART_MODE_NORMAL | US_MR_USCLKS_MCK | US_MR_CHRL_8_BIT | US_MR_PAR_NO;
  
  // 3. Setup Baudrate
  usart1_set_line_coding(&current_coding);

  // 4. DMA (PDC) RX Setup
  USART1->US_RPR  = (uint32_t)dma_rx_buffer;
  USART1->US_RCR  = USART1_RX_BUF_SIZE;
  USART1->US_RNPR = (uint32_t)dma_rx_buffer;
  USART1->US_RNCR = USART1_RX_BUF_SIZE;
  
  USART1->US_PTCR = US_PTCR_RXTEN | US_PTCR_TXTEN;

  // 5. Interrupt Setup
  // KORREKTUR: Nur TIMEOUT aktivieren, NICHT ENDTX!
  // ENDTX aktivieren wir erst, wenn wir wirklich Daten senden.
  USART1->US_IER = US_IER_TIMEOUT; 
  
  USART1->US_RTOR = 40; 
  USART1->US_CR |= US_CR_STTTO;

  NVIC_EnableIRQ(USART1_IRQn);

  // 6. Enable
  USART1->US_CR = US_CR_RXEN | US_CR_TXEN;
}

// --- HELPER: Checks if USB buffer has data and UART is ready ---
// Called from: USB_Handler (when data arrives) AND USART1_Handler (when TX finishes)
void usart1_tx_dma_check_and_send(void)
{
  // 1. Check if DMA TX is busy
  if (USART1->US_TCR > 0) return; 

  // 2. Check if we have data in CDC RX Ringbuffer
  uint16_t n = rb_read(&cdc_rx, dma_tx_buffer, sizeof(dma_tx_buffer));
  
  if (n > 0) {
      USART1->US_TPR = (uint32_t)dma_tx_buffer;
      USART1->US_TCR = n;
      
      // KORREKTUR: Jetzt, wo wir senden, brauchen wir den Interrupt,
      // um zu wissen, wann wir fertig sind.
      USART1->US_IER = US_IER_ENDTX; 
  }
}

// --- INTERRUPT HANDLER ---
void USART1_Handler(void)
{
  uint32_t status = USART1->US_CSR;

  // 1. Handle RX Event
  if ((status & US_CSR_TIMEOUT) || (status & US_CSR_RXBUFF) || (status & US_CSR_ENDRX))
  {
      if (status & US_CSR_TIMEOUT) USART1->US_CR = US_CR_STTTO;

      uint32_t dma_head = USART1_RX_BUF_SIZE - USART1->US_RCR;
      while (dma_rx_tail != dma_head) {
          uint8_t c = dma_rx_buffer[dma_rx_tail];
          cdc_write(&c, 1); 
          dma_rx_tail++;
          if (dma_rx_tail >= USART1_RX_BUF_SIZE) dma_rx_tail = 0;
      }
  }

  // 2. Handle TX Complete (ENDTX)
  if (status & US_CSR_ENDTX)
  {
      // Versuche, mehr Daten aus dem Ringbuffer zu holen
      uint16_t n = rb_read(&cdc_rx, dma_tx_buffer, sizeof(dma_tx_buffer));

      if (n > 0) {
          // Wir haben Daten -> DMA neu laden
          USART1->US_TPR = (uint32_t)dma_tx_buffer;
          USART1->US_TCR = n;
          // Interrupt bleibt an (wir senden ja weiter)
      } else {
          // Keine Daten mehr -> Interrupt ABSCHALTEN!
          // Sonst feuert er endlos, da TCR=0 bleibt.
          USART1->US_IDR = US_IDR_ENDTX;
      }
  }
}

// ... Set/Get Line Coding functions remain same as before ...
void usart1_set_line_coding(const usb_cdc_line_coding_t* coding) {
  memcpy(&current_coding, coding, sizeof(usb_cdc_line_coding_t));
  if (current_coding.dwDTERate == 0) current_coding.dwDTERate = 115200;
  uint32_t cd = SystemCoreClock / (16 * current_coding.dwDTERate);
  USART1->US_BRGR = cd;
  
  uint32_t mr = US_MR_USART_MODE_NORMAL | US_MR_USCLKS_MCK;
  // ... Parity/Stopbits logic from previous step ...
  switch (current_coding.bParityType) {
    case 0: mr |= US_MR_PAR_NO; break;
    case 1: mr |= US_MR_PAR_ODD; break;
    case 2: mr |= US_MR_PAR_EVEN; break;
    case 3: mr |= US_MR_PAR_MARK; break;
    case 4: mr |= US_MR_PAR_SPACE; break;
    default: mr |= US_MR_PAR_NO;
  }
  if (current_coding.bCharFormat == 2) mr |= US_MR_NBSTOP_2_BIT;
  else if (current_coding.bCharFormat == 1) mr |= US_MR_NBSTOP_1_5_BIT;
  else mr |= US_MR_NBSTOP_1_BIT;
  
  if (current_coding.bDataBits == 7) mr |= US_MR_CHRL_7_BIT;
  else mr |= US_MR_CHRL_8_BIT;

  USART1->US_MR = mr;
}

void usart1_get_line_coding(usb_cdc_line_coding_t* coding) {
    memcpy(coding, &current_coding, sizeof(usb_cdc_line_coding_t));
}