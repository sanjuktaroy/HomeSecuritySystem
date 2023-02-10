/*
 * Author: Miguel Bautista (50298507)
 *
 * File Purpose: Contains initialization code for the RCC and GPIO pins and code to write to MODER
 *
 * Modules: 
 * Subroutines:
 * void set_pin_mode(unsigned int pin, GPIO_TypeDef *port, unsigned int mode) - Set the designed pin/port to be an input/output
 * void enable_rcc(unsigned int port) - Enable the reset control clock for the specified GPIO port
 * void write_to_pin(unsigned int pin, GPIO_TypeDef *port, unsigned int value) - Writes the designated value (0/1) to the entered pin and port
 *
 * Assignment: Project 2
 * Inputs: 
 * Outputs: 
 * Constraints:
 * References: 
 *      MBED Bare Metal Guide - https://os.mbed.com/docs/mbed-os/v6.15/bare-metal/index.html
 *      STM32L48 User Guide - https://www.st.com/resource/en/reference_manual/rm0351-stm32l47xxx-stm32l48xxx-stm32l49xxx-and-stm32l4axxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf 
 */
 #include "mbed.h"

void set_pin_mode(unsigned int pin, GPIO_TypeDef *port, unsigned int mode);
void enable_rcc(unsigned int port);
void write_to_pin(unsigned int pin, GPIO_TypeDef *port, unsigned int value);
