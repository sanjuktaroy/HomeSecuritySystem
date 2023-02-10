
#include <mbed.h>

#define UPPERCASE 65
#define LOWERCASE 97

void set_pin_mode(unsigned int pin, GPIO_TypeDef *port, unsigned int mode);
void enable_rcc(unsigned int port);
void write_to_pin(unsigned int pin, GPIO_TypeDef *port, unsigned int value);

void enable_rcc(unsigned int port) {
  // Enable RCC for GPIO port
  unsigned int offset =
      (port - LOWERCASE) < 0
          ? port - UPPERCASE
          : port - LOWERCASE; // Determine if character is uppercase/lowercase
  RCC->AHB2ENR |= (0x1 << offset); // Enable port using port offset
}

void set_pin_mode(unsigned int pin, GPIO_TypeDef *port, unsigned int mode) {
  // MODE 0 -> Input Mode || MODE 1 -> Output Mode
  unsigned int offset = pin * 2; // offset pin times 2; accounts for 2-bits
  if (mode) {
    port->MODER &= ~(0x2 << (offset)); // Set leftmost bit of pin to 0
    port->MODER |= (0x1 << (offset));  // Set rightmost bit of pin to 1
  } else {
    port->MODER &= ~(0x3 << (offset)); // Set both pins to 0
  }
}

void write_to_pin(unsigned int pin, GPIO_TypeDef *port, unsigned int value) {
  // Writes logic high/low to pin
  value ? port->ODR |= (0x1 << pin) : port->ODR &= ~(0x1 << pin);
}
