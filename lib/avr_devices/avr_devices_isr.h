#pragma once
#include <stdint.h>

typedef struct 
{
    uint32_t signature;
    uint8_t  flash_mode;
    uint16_t flash_page_size;
    uint8_t  flash_delay_ms;
    uint8_t  eeprom_mode;
    uint8_t  eeprom_page_size;
    uint8_t  eeprom_delay_ms;
    
} avr_device_isr_t;

extern const avr_device_isr_t* current_device_isr;

void avr_device_isr_set_current_device(uint32_t signature);

inline uint8_t avr_device_isr_get_flash_mode(void){
  return current_device_isr->flash_mode;
}

inline uint16_t avr_device_isr_get_flash_page_size(void){
  return current_device_isr->flash_page_size;
}

inline uint8_t avr_device_isr_get_flash_delay_ms(void){
  return current_device_isr->flash_delay_ms;
}

inline uint8_t avr_device_isr_get_eeprom_mode(void){
  return current_device_isr->eeprom_mode;
}

inline uint8_t avr_device_isr_get_eeprom_page_size(void){
  return current_device_isr->eeprom_page_size;
}

inline uint8_t avr_device_isr_get_eeprom_delay_ms(void){
  return current_device_isr->eeprom_delay_ms;
}
