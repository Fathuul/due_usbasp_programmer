/* 
   AVR - Serial Programming Instructions

*/
#pragma once

#include "stdio.h"
#include "stdint.h"
#include <stdbool.h>
#include "gpio.h"

#define AVR_RESET_PIN 24 // A.24 (A1)
#define AVR_RESET_PORT PIOA

#define AVR_OE_PIN 23 // A.23 (A2)
#define AVR_OE_PORT PIOA

static inline void avr_spi_reset_pin_init() {
   gpio_init(AVR_RESET_PORT);
   gpio_cfg_output(AVR_RESET_PORT, AVR_RESET_PIN);
   gpio_pin_reset(AVR_RESET_PORT, AVR_RESET_PIN);
}

static inline void avr_spi_reset_pin_set() {
   gpio_pin_set(AVR_RESET_PORT, AVR_RESET_PIN);
}

static inline void avr_spi_reset_pin_reset() {
   gpio_pin_reset(AVR_RESET_PORT, AVR_RESET_PIN);
}

static inline void avr_spi_oe_pin_init() {
   gpio_init(AVR_OE_PORT);
   gpio_cfg_output(AVR_OE_PORT, AVR_OE_PIN);
   gpio_pin_reset(AVR_OE_PORT, AVR_OE_PIN);
}

static inline void avr_spi_oe_pin_set() {
   gpio_pin_set(AVR_OE_PORT, AVR_OE_PIN);
}

static inline void avr_spi_oe_pin_reset() {
   gpio_pin_reset(AVR_OE_PORT, AVR_OE_PIN);
}



// bool avr_spi_set_slow_mode(bool mode);
// bool avr_spi_get_slow_mode(void);
uint8_t* avr_spi_get_tx_ptr(void);
uint8_t* avr_spi_get_rx_ptr(void);
uint8_t* avr_spi_transmit(uint8_t* tx);
void avr_spi_reset_ext_addr(void);
void avr_spi_reset_sig(void);
uint32_t avr_spi_get_sig(void);
void avr_spi_check_ext_addr(uint32_t byte_addr);
uint8_t avr_spi_enableprog(void);
uint8_t avr_spi_read_flash_byte(uint32_t byte_addr);
void avr_spi_write_flash_byte(uint32_t byte_addr, uint8_t data);
void avr_spi_flash_page_commit(uint32_t byte_addr);
uint8_t avr_spi_read_eeprom_byte(uint32_t byte_addr);
void avr_spi_write_eeprom_byte(uint32_t byte_addr, uint8_t data);
void avr_spi_eeprom_page_commit(uint32_t byte_addr);
void avr_spi_chip_erase(void);
