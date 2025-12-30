#pragma once
#include <stdint.h>
#include <stdbool.h>

// SPI0 Pin Definitions (Peripheral A)
// Values are PIN NUMBERS (0-31), not bitmasks!
#define SPI0_MISO_PIN  25
#define SPI0_MOSI_PIN  26
#define SPI0_SPCK_PIN  27

// Bitmasks for direct register access
#define SPI0_MISO_MASK (1u << SPI0_MISO_PIN)
#define SPI0_MOSI_MASK (1u << SPI0_MOSI_PIN)
#define SPI0_SPCK_MASK (1u << SPI0_SPCK_PIN)

// Setup SPI0 hardware (Pins A.25 - A.27)
void hw_spi0_enable(void);

// Disable SPI0 and set pins to High-Z (GPIO Input)
void hw_spi0_disable(void);

// Set baud rate based on USBasp index (0..12)
// Uses a hybrid approach: Hardware SPI for fast speeds, Software SPI (Bit-Banging) for slow speeds
void hw_spi_set_br_value(uint16_t sck_idx);

// Transmit/Receive buffer with timeout
// tx: Source buffer (can be NULL to send 0xFF dummy bytes)
// rx: Dest buffer (can be NULL to ignore received bytes)
// len: Number of bytes to transfer
// t: Timeout in milliseconds (per transfer unit)
bool hw_spi0_tx_rx(const uint8_t* tx, uint8_t* rx, uint16_t len, uint16_t t);