#include "sam3x8e.h"
#include "hw_spi.h"
#include "delay.h" // Provides get_ticks_ms, delay_us and DWT access
#include "gpio.h"  // Provides gpio_pin_set, gpio_pin_reset etc.
#include <stdio.h>

// --- State Variables ---
static bool use_sw_spi = false;
static uint32_t sw_delay_half_period = 0; // Delay in us for Bit-Banging

static uint8_t _sck_idx;

// --- Lookup Tables ---

// Mapping: USBasp Index (0-13) -> SCBR Divider (0 = use SW Mode)
// Note: Hardware SCBR is limited to 8-bit (max 255).
static const uint8_t speed_map[14] = {
    0,   // Idx 0: Auto (187.5 kHz SW) 
    0,   // Idx 1: 500 Hz (SW)
    0,   // Idx 2: 1 kHz (SW)
    0,   // Idx 3: 2 kHz (SW)
    0,   // Idx 4: 4 kHz (SW)
    0,   // Idx 5: 8 kHz (SW)
    0,   // Idx 6: 16 kHz (SW)
    0,   // Idx 7: 32 kHz (SW)
    0,   // Idx 8: 93.75 kHz (SW)
    0,   // Idx 9: 187.5 kHz (SW)
    224, // Idx 10: 375 kHz (HW) -> 84M/224
    112, // Idx 11: 750 kHz (HW) -> 84M/112
    56,  // Idx 12: 1.5 MHz (HW) -> 84M/56
    28   // Idx 13: 3.0 MHz (HW) -> 84M/28
};

// Delays for SW-SPI (Half Period in µs) for indices 1..9
static const uint32_t sw_delays_us[10] = {
    2,    // idx 0: 187.5 kHz (default)
    1000, // Idx 1: 500 Hz
    500,  // Idx 2: 1 kHz
    250,  // Idx 3: 2 kHz
    125,  // Idx 4: 4 kHz
    62,   // Idx 5: 8 kHz
    31,   // Idx 6: 16 kHz
    15,   // Idx 7: 32 kHz
    5,    // Idx 8: ~93 kHz
    2     // Idx 9: ~187 kHz
};

// --- Software SPI Helper ---
// Mode 0: CPOL=0, CPHA=0 (Sample on Rising, Setup on Falling)
static uint8_t sw_spi_transfer_byte(uint8_t data_out)
{
    uint8_t data_in = 0;

    // Ensure Clock starts LOW
    gpio_pin_reset(PIOA, SPI0_SPCK_PIN);

    for (int i = 7; i >= 0; i--)
    {
        // 1. Setup Data (MOSI)
        if (data_out & (1 << i))
        {
            gpio_pin_set(PIOA, SPI0_MOSI_PIN);
        }
        else
        {
            gpio_pin_reset(PIOA, SPI0_MOSI_PIN);
        }

        delay_us(sw_delay_half_period);

        // 2. Clock High (Sample MISO)
        gpio_pin_set(PIOA, SPI0_SPCK_PIN);

        // Read MISO
        if (gpio_get(PIOA, SPI0_MISO_PIN))
        {
            data_in |= (1 << i);
        }

        delay_us(sw_delay_half_period);

        // 3. Clock Low
        gpio_pin_reset(PIOA, SPI0_SPCK_PIN);
    }
    return data_in;
}

// --- Public API ---

void hw_spi0_enable(void)
{
    // 1. Enable SPI0 Peripheral Clock
    PMC->PMC_PCER0 = (1u << ID_SPI0);

    // Default: Hardware Mode (Start safe with slowest HW speed)
    // We use MASKS here for direct register access!
    // Disable GPIO (Enable Peripheral Control) for SPI Pins
    PIOA->PIO_PDR = SPI0_MISO_MASK | SPI0_MOSI_MASK | SPI0_SPCK_MASK;

    // Select Peripheral A (ABSR=0)
    PIOA->PIO_ABSR &= ~(SPI0_MISO_MASK | SPI0_MOSI_MASK | SPI0_SPCK_MASK);

    // Reset & Config
    SPI0->SPI_CR = SPI_CR_SWRST;

    // MSTR: Master Mode, MODFDIS: Mode Fault Disable, PCS: Chip Select 0
    SPI0->SPI_MR = SPI_MR_MSTR | SPI_MR_MODFDIS | SPI_MR_PCS(~(1u << 0));

    // FIX: SCBR register is only 8-bit! Max value is 255.
    // 84MHz / 255 = ~350 kHz. This is the slowest hardware speed.
    SPI0->SPI_CSR[0] = SPI_CSR_SCBR(240) 
                     | SPI_CSR_BITS_8_BIT 
                     | SPI_CSR_CSAAT 
                     | SPI_CSR_NCPHA;

    // Enable SPI
    SPI0->SPI_CR = SPI_CR_SPIEN;

    use_sw_spi = false;
    hw_spi_set_br_value(_sck_idx);
}

void hw_spi0_disable(void)
{
    // 1. Disable SPI
    SPI0->SPI_CR = SPI_CR_SPIDIS;

    // 2. Set Pins to Input / High-Z (GPIO Mode)
    // We use PINS here for gpio.h functions!
    gpio_cfg_input(PIOA, SPI0_MISO_PIN);
    gpio_cfg_input(PIOA, SPI0_MOSI_PIN);
    gpio_cfg_input(PIOA, SPI0_SPCK_PIN);
}

void hw_spi_set_br_value(uint16_t sck_idx)
{
    if (sck_idx > 13)
        sck_idx = 9; // Default to ~187 kHz if unknown

    _sck_idx = sck_idx;

    uint8_t scbr = speed_map[sck_idx];

    if (scbr > 0)
    {
        // --- HARDWARE MODE ---
        if (use_sw_spi)
        {
            // Switch pins back to Peripheral A control (using MASKS)
            PIOA->PIO_PDR = SPI0_MISO_MASK | SPI0_MOSI_MASK | SPI0_SPCK_MASK;
            use_sw_spi = false;
        }

        // Update SCBR (Baud Rate)
        uint32_t csr = SPI0->SPI_CSR[0];
        csr &= ~(SPI_CSR_SCBR_Msk);
        csr |= SPI_CSR_SCBR(scbr);
        SPI0->SPI_CSR[0] = csr;
        // printf("sck_idx: [%d] hw-mode\n", sck_idx);
    }
    else
    {
        // --- SOFTWARE MODE ---
        if (!use_sw_spi)
        {
            // Switch pins to GPIO control (using PINS)
            gpio_cfg_output(PIOA, SPI0_MOSI_PIN);
            gpio_cfg_output(PIOA, SPI0_SPCK_PIN);
            gpio_cfg_input(PIOA, SPI0_MISO_PIN);

            // Set Idle State (Mode 0: SCK Low)
            gpio_pin_reset(PIOA, SPI0_SPCK_PIN);

            use_sw_spi = true;
        }

        // Update SW delay
        if (sck_idx <= 9)
        {
            sw_delay_half_period = sw_delays_us[sck_idx];
        }
    }
}

bool hw_spi0_tx_rx(const uint8_t* tx, uint8_t* rx, uint16_t len, uint16_t t)
{
  if (len == 0) return true;

  if (use_sw_spi) {
      // --- SW Implementation ---
      for (uint16_t i = 0; i < len; i++) {
          uint8_t val = tx ? tx[i] : 0xFF;
          uint8_t ret = sw_spi_transfer_byte(val);
          if (rx) rx[i] = ret;
      }
      return true;
  } 
  else {
      // --- HW Implementation ---
      
      // 1. FLUSH: Clear any stale data in the Receive Data Register
      // If we aborted previously, RDR might still hold a byte.
      // Reading it clears the RDRF flag.
      volatile uint32_t dummy = SPI0->SPI_RDR;
      (void)dummy;

      uint32_t timeout_limit = get_ticks_ms(t);

      for (uint16_t i = 0; i < len; i++) {
        uint32_t start_time = DWT_CYCCNT;
        
        // 2. Wait for TX Ready (TDRE)
        while ((SPI0->SPI_SR & SPI_SR_TDRE) == 0) {
           if ((DWT_CYCCNT - start_time) > timeout_limit) {
               // TIMEOUT DETECTED!
               // The hardware might be stuck. Reset the SPI block to recover.
               hw_spi0_enable(); // Re-Init hardware to clear stuck state
               return false; 
           }
        }

        uint8_t data_out = tx ? tx[i] : 0xFF;
        SPI0->SPI_TDR = data_out;

        // 3. Wait for RX Ready (RDRF)
        start_time = DWT_CYCCNT;
        while ((SPI0->SPI_SR & SPI_SR_RDRF) == 0) {
           if ((DWT_CYCCNT - start_time) > timeout_limit) {
               // TIMEOUT DETECTED!
               // Target didn't respond or clock failure. Reset SPI block.
               hw_spi0_enable(); // Re-Init hardware
               return false;
           }
        }

        uint8_t data_in = (uint8_t)(SPI0->SPI_RDR & 0xFF);
        if (rx) rx[i] = data_in;
      }
      return true;
  }
}