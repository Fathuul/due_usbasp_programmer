#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "hw_spi.h"
#include "delay.h"
#include "avr_devices_isr.h"


static uint8_t  avr_spi_tx[4];
static uint8_t  avr_spi_rx[4];
static uint8_t  avr_spi_ext_addr;
static uint32_t avr_spi_sig;

typedef enum {
    FLASH_DELAY,
    EEPROM_DELAY
} avr_busy_mode_t;


uint8_t* avr_spi_get_tx_ptr(void) {
    return avr_spi_tx;
}

uint8_t* avr_spi_get_rx_ptr(void) {
    return avr_spi_rx;
}

bool avr_spi_wait_ready(avr_busy_mode_t mode)
{
    if (mode != FLASH_DELAY && mode != EEPROM_DELAY) {
        delay_ms(50);
        return true;
    }

    // command 0xF0 00 00 00
    avr_spi_tx[0] = 0xF0;
    avr_spi_tx[1] = 0x00;
    avr_spi_tx[2] = 0x00;
    avr_spi_tx[3] = 0x00;

    
    
    uint8_t _delay, _mode;

    if ( mode == FLASH_DELAY ) {
        _delay = avr_device_isr_get_flash_delay_ms();
        _mode = avr_device_isr_get_flash_mode();
    } else {
        _delay = avr_device_isr_get_eeprom_delay_ms();
        _mode = avr_device_isr_get_eeprom_mode();
    }

    if (_mode != 0x41)
    {
        delay_ms(_delay);
        return true;
    }

    // base wait time
    delay_ms(_delay / 2);

    // Calculate timeout in ticks
    uint32_t timeout_limit = get_ticks_ms(_delay/2);
    uint32_t start_time = DWT_CYCCNT;

    // start polling
    while (1)
    {

        hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 1);

        // ready bit is bit0 of response byte 3
        if (avr_spi_rx[3] & 0x01)
            return true;

        if (DWT_CYCCNT - start_time > timeout_limit)
            break;

        delay_us(50);
    }

    return false;
}


uint8_t* avr_spi_transmit(uint8_t* tx)
{
    hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    if (tx[0] == 0x30 && tx[2] < 3)
        avr_spi_sig |= avr_spi_rx[3] << (16 - (8 * tx[2]));

    if (tx[0] == 0x30 && tx[2] == 0)
        avr_device_isr_set_current_device(0);

    if (tx[0] == 0x30 && tx[2] == 2)
        avr_device_isr_set_current_device(avr_spi_sig);

    return avr_spi_rx;
}


void avr_spi_reset_ext_addr(void) {
    avr_spi_ext_addr = 0xFF;
}


void avr_spi_reset_sig(void) {
    avr_spi_sig = 0x00000000;
}

uint32_t avr_spi_get_sig(void) {
    return avr_spi_sig;
}


void avr_spi_check_ext_addr(uint32_t byte_addr)
{

    uint8_t ext_addr = (uint8_t)(byte_addr >> 17);
    if (ext_addr != avr_spi_ext_addr)
    {
        avr_spi_ext_addr = ext_addr;
        avr_spi_tx[0] = 0x4D; // Load Extended Address byte
        avr_spi_tx[1] = 0x00;
        avr_spi_tx[2] = ext_addr;
        avr_spi_tx[3] = 0x00;

        hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    }
}

uint8_t avr_spi_enableprog(void)
{
    avr_spi_tx[0] = 0xAC;
    avr_spi_tx[1] = 0x53;
    avr_spi_tx[2] = 0x00;
    avr_spi_tx[3] = 0x00;

    hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    return avr_spi_rx[2];
}


uint8_t avr_spi_read_flash_byte(uint32_t byte_addr)
{
    
    uint16_t word = (uint16_t)(byte_addr >> 1); 
    avr_spi_tx[0] = (byte_addr & 1) ? 0x28 : 0x20;  
    avr_spi_tx[1] = (word >> 8) & 0xFF;
    avr_spi_tx[2] = word & 0xFF;
    avr_spi_tx[3] = 0x00;

    hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    return avr_spi_rx[3];
}


void avr_spi_write_flash_byte(uint32_t byte_addr, uint8_t data)
{

    uint16_t word = (uint16_t)(byte_addr >> 1); 
    avr_spi_tx[0] = (byte_addr & 1) ? 0x48 : 0x40; 
    avr_spi_tx[1] = (word >> 8) & 0xFF;
    avr_spi_tx[2] = word & 0xFF;
    avr_spi_tx[3] = data;

    
    hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    delay_us(3);
}


void avr_spi_flash_page_commit(uint32_t byte_addr)
{

    uint16_t word = (uint16_t)(byte_addr >> 1);
    avr_spi_tx[0] = 0x4C;
    avr_spi_tx[1] = (word >> 8) & 0xFF;
    avr_spi_tx[2] = word & 0xFF;
    avr_spi_tx[3] = 0x00;

    hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    avr_spi_wait_ready(FLASH_DELAY);
    
}

uint8_t avr_spi_read_eeprom_byte(uint32_t byte_addr)
{
    avr_spi_tx[0] = 0xA0;  
    avr_spi_tx[1] = (byte_addr >> 8) & 0xFF;
    avr_spi_tx[2] = byte_addr & 0xFF;
    avr_spi_tx[3] = 0x00;

    
    hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    return avr_spi_rx[3];
}

void avr_spi_write_eeprom_byte(uint32_t byte_addr, uint8_t data)
{
    avr_spi_tx[0] = 0xC0;
    avr_spi_tx[1] = (byte_addr >> 8) & 0xFF;
    avr_spi_tx[2] = byte_addr & 0xFF;
    avr_spi_tx[3] = data;

    
    hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);
 
    delay_us(3);
    
}

void avr_spi_eeprom_page_commit(uint32_t byte_addr)
{
    avr_spi_tx[0] = 0xC2;
    avr_spi_tx[1] = (byte_addr >> 8) & 0xFF;
    avr_spi_tx[2] = byte_addr & 0xF8;
    avr_spi_tx[3] = 0x00;

    
    hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    avr_spi_wait_ready(EEPROM_DELAY);
    
}



void avr_spi_chip_erase(void)
{
    avr_spi_tx[0] = 0xAC;
    avr_spi_tx[1] = 0x80;
    avr_spi_tx[2] = 0x00;
    avr_spi_tx[3] = 0x00;

    
    hw_spi0_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    delay_ms(50);

}

