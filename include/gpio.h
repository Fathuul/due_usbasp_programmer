#pragma once
#include "sam3x8e.h"

// -----------------------------------------------------------------------------
// GPIO Initialization
// -----------------------------------------------------------------------------

// Enable the Peripheral Clock for the specified PIO Port
// This must be called once before using any pins on the port.
static inline void gpio_init(Pio* port) {
    uint32_t id = 0;
    
    // Map Port Address to Peripheral ID
    if      (port == PIOA) id = ID_PIOA;
    else if (port == PIOB) id = ID_PIOB;
    else if (port == PIOC) id = ID_PIOC;
    else if (port == PIOD) id = ID_PIOD;
#ifdef ID_PIOE
    else if (port == PIOE) id = ID_PIOE;
#endif
#ifdef ID_PIOF
    else if (port == PIOF) id = ID_PIOF;
#endif

    // Enable Clock in PMC (Peripheral Control Enable Register 0)
    if (id != 0) {
        PMC->PMC_PCER0 = (1u << id);
    }
}

// -----------------------------------------------------------------------------
// GPIO Configuration Helper Functions (Inline for max performance)
// -----------------------------------------------------------------------------

// Configure pin(s) as Push-Pull Output
// - Enables PIO control
// - Enables Output Driver
// - Disables internal Pull-Up (clean state)
static inline void gpio_cfg_output(Pio* port, uint8_t pin) {
    port->PIO_PER  = (1 << pin); // PIO Enable
    port->PIO_OER  = (1 << pin); // Output Enable
    port->PIO_PUDR = (1 << pin); // Pull-Up Disable
}

// Configure pin(s) as Input (High Impedance / Hi-Z)
// - Enables PIO control
// - Disables Output Driver
// - Disables internal Pull-Up (Floating Input)
static inline void gpio_cfg_input(Pio* port, uint8_t pin) {
    port->PIO_PER  = (1 << pin); // PIO Enable
    port->PIO_ODR  = (1 << pin); // Output Disable (Input Mode)
    port->PIO_PUDR = (1 << pin); // Pull-Up Disable
}

// Configure internal Pull-Up Resistor
// enable: true = Enable Pull-Up, false = Disable Pull-Up
static inline void gpio_cfg_pullup(Pio* port, uint8_t pin, int enable) {
    if (enable) {
        port->PIO_PUER = (1 << pin); // Pull-Up Enable
    } else {
        port->PIO_PUDR = (1 << pin); // Pull-Up Disable
    }
}

// -----------------------------------------------------------------------------
// GPIO Control Functions
// -----------------------------------------------------------------------------

// Set pin(s) to HIGH level (3.3V)
static inline void gpio_pin_set(Pio* port, uint8_t pin) {
    port->PIO_SODR = (1 << pin); // Set Output Data Register
}

// Set pin(s) to LOW level (GND)
static inline void gpio_pin_reset(Pio* port, uint8_t pin) {
    port->PIO_CODR = (1 << pin); // Clear Output Data Register
}

// Toggle pin state (High <-> Low)
static inline void gpio_pin_toggle(Pio* port, uint8_t pin) {
    if (port->PIO_ODSR & pin) {
        port->PIO_CODR = (1 << pin);
    } else {
        port->PIO_SODR = (1 << pin);
    }
}

// Read current pin level
// Returns non-zero if pin is HIGH, 0 if LOW
static inline uint32_t gpio_get(Pio* port, uint8_t pin) {
    return (port->PIO_PDSR & (1 << pin)); // Read Pin Data Status Register
}