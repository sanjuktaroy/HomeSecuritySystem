# About
This system is a traditional security alarm system which will be used to contribute in the public interest of safety by providing consumers with the peace of mind of a
safer area. This is achieved by using an ultrasonic sensor to keep track of nearby objects and a microphone to detect loud noises. If these are detected, the system will
activate and notify the user that something has been detected. The user can interact with the system by using a keypad to arm/disarm the system through the LCD interface.

The Nucleo-L4R5ZI will be the microcontroller used to control this system and achieve the specified design constraints and specifications.

Contributor List: 
* **Miguel Bautista** (50298507)

# Features
* Ability to define an initial security code when first powered on
* Arms the system or disarm the system by pressing “A” (After security code is entered)
* Display the system status when idling (No button has been pressed in 10 seconds)
* Trigger the armed alarm system when the microphone or ultrasonic sensor has been tripped
* Notifies system owner which sensor has been tripped upon alarm system trigger
* At all times, shows which sensors are activated
* “Alerts authorities” if system is not disarmed within 10 seconds after triggered
* Must run “forever”

# Required Materials
* 1602 LCD with I2C chip: Used to display the UI
* Nucleo-L4R5ZI: This microcontroller will control the system logic
* 4x4 Matrix Keypad: System input device the user will control
* Solderless Breadboard: Provides an easy way to connect the parts together
* Jumper wires: Provides connections between the pieces
* LEDs: Will be used as outputs to indicate various events such as alarms or inputs
* Microphone sensor: Used to detect nearby sound
* HC-SR04 Ultrasonic Sensor: Used to detect nearby motion
* One Digit Seven Segment Display: Used for Alarm Countdown
* Active Buzzer: Used as the alarm sound of the triggered system

# Resources and References
* MBED Bare Metal Guide - https://os.mbed.com/docs/mbed-os/v6.15/bare-metal/index.html

* STM32L48 User Guide - https://www.st.com/resource/en/reference_manual/rm0351-stm32l47xxx-stm32l48xxx-stm32l49xxx-and-stm32l4axxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf 

# Getting Started
*	Connect the 4x4 matrix keypad to the Nucleo
    *	With the keypad facing upwards, connect the keypad starting from left to right
    *	Going from left to right, the pins should be connected to PA3, PC0, PC3, PC1, PF14, PE11, PE9, PF13
        *	Visually, there should be 4 wires on the left of the Nucleo – corresponding to the keypad rows – and then 4 wires on the right of the Nucleo – corresponding to the keypad columns.
*	Connect VCC and Ground to the red and blue lines of the breadboard respectively
*	Place the ultrasonic sensor on the breadboard
    *	Connect to VCC and ground
    *	Connect the Trigger pin to PD6 of the Nucleo
    *	Connect the Echo pin to PD5 of the Nucleo
*	Place an and gate on the breadboard
    *	Connect to VCC and ground
*	Place the microphone on the breadboard
    *	Connect to VCC and ground
    *	Connect the Digital Output pin to the first And gate input
    *	Connect PF12 on the Nucleo to the second And gate input
    *	Connect the And gate output to PD7
*	Adjust the microphone sensitivity by turning the potentiometer clockwise or anti-clockwise so that the first LED only blinks for loud noises and doesn’t activate for ambient noise.	
*	Place the 1602 LCD on the breadboard
    *	Connect to VCC and ground
    *	Connect the SDA pin to PB9
    *	Connect the SCL pin to PB8
*	Place LEDs on the board in parallel
    *	Connect to ground
    *	Connect the positive side of the LEDs to PD15 of the Nucleo
*	Place the active buzzer on the breadboard
    *	Connect to ground
    *	Connect the positive side to PD4

# Modules

## CSE321_project2_mabautis_main.cpp:
Main file which contains the initialization and code to run the timer system.

### Things Declared:

* key_pressed [int] - Determines if key is pressed (toggled by keypad ISRs)
* debounced [int] - Determines if a key press is valid after debouncing it
* display_on [int] - Flag to determine LCD state
* echo_on [int] - Determines if the echo pin is high or low (can change while thread is going to access it)
* password = [string] - Passcode entered on system boot
* string password_entered [string] - Passcode entered when attempting to switch between system modes
* password_position [int] - Flag to determine which digit is being entered
* entering_password [int] - Flag to determine if a passcode is being entered
* LCD [CSE321_LCD] - LCD instance as defined by lcd1602.cpp
* col_0, col_1, col_2, col_3 [InterruptIn] - Interrupts associated with the 4x4 matrix keypad columns. NOTE: pins sets to PullDown mode to ensure pin is pulled to 0V
* microphone [InterruptIn] - Initialize microphone Dout as an interrupt
* ultrasonic_echo [InterruptIn] - Initialize ultrasonic sensor echo as an interrupt
* ultrasonic_trigger [DigitalOut] - Set ultrasonic trigger as a digit output
* active_buzzer [DigitalOut] - Set active buzzer as a digit output
* microphone_enable [DigitalOut] - Set pin going to microphone AND Gate as a digit output to enable and disable the mic interrupt pin
* alarm_leds [DigitalOut] - Set LEDs as a digital output
* row_thread [Thread] - Declare thread handling keypad rows
* key_thread [Thread] - Declare thread maintaining system modes
* resource_lock [Mutex] - Declare mutex to maintain thread sychronization and protect against race conditions
* queue [EventQueue] - Initialize EventQueue to queue blocking code from ISR
* idle_timeout [Timeout] - Timeout to disable LCD backlight after 10 seconds
* ultrasonic_timeout [Timeout] - Timeout to check if ultrasonic echo is still active to determine object distance 
* ultrasonic_ticker [Ticker] - Ticker to trigger ultrasonic sensor pulses
* keypad [char] - Enumerate keypad matrix
* mode [int] - 0 -> Power On Mode (Define Code), 1 -> Unarmed, 2 -> Armed, 3 -> Triggered
* row [int] - Current keypad row to power
* debounce_ticker [Ticker] - 1 millisecond interval ticker to ensure that key presses are debounced to validate input
* timer_ticker [Ticker] - 1 second interval ticker to handle the timer when in mode 2
* keypad char[4][4] - Nested array to represent keypad buttons
* mode [int] -  0 -> Off, 1 -> Input, 2 -> Timer
* key_pressed [int] - Flag to determine if there is a key currently pressed to debounce and handle
* debounce_buffer [int] - Debounce flag to know when a key press is valid
* debounced [int] - Flag to know if key has been debounced already
* cursor [int] - Keeps track of what spot user is entering numbers into M:SS
* count_direction [int] - 0 -> Down, 1 -> Up,
* time_remaining [int] - In seconds (count_direction = 0)
* time_passed [int] - In Seconds (count_direction = 1)

### API and Built-In Elements Used:
* Mbed – Microcontroller API used for InterruptIn initialization

### Custom Functions:
* isr_col(void) - Rising edge Interrupt Service Routine for column pins [PF_14, PE_11, PE_9, PF_13]
* isr_falling_edge(void) - Falling edge Interrupt Service Routine for column pins [PF_14, PE_11, PE_9, PF_13]
* isr_microphone(void) - Rising edge ISR for micrphone PD_7
* isr_ultrasonic(void) - Rising edge ISR for ultrasonic sensor echo pin PD_5
* isr_ultrasonic_falling_edge(void) - Falling edge ISR for ultrasonic echo pin 
* ultrasonic_handler(void) - Handler after timeout is reached to see if object is within specified range
* trigger_ultrasonic_sensor(void) - Send 10us pulse to ultrasonic trigger pin
* microphone_handler(void) - Handles switching to triggered mode if sound is detected
* row_handler(void) - Thread callback that handles the powering of rows on the matrix keypad
* key_handler(void) - Thread callback that handles key presses based on current system mode
* power_on_mode(void) - Initial power on state where passcode is defined
* unarmed_mode(void) - Unarmed state where sensors do not trigger the system
* armed_mode(void) - Armed state (after entering passcode in unarmed mode) where sensors trigger the system
* triggered_mode(void) - State when a sensor is tripped in the armed state
* trigger_mode_transition(void) - Used to call blocking code from ultrasonic ISR when motion detected
* idle_timeout_handler(void) - Timeout handler after 10 seconds has passed without system input
* set_display_off(void) - Calls blocking code from idle timeout to set the display off and reset LCD text

## CSE321_project2_mabautis_stm_methods.cpp:
Contains initialization code for the RCC and GPIO pins and code to write to MODER

### API and Built-In Elements Used:
* Mbed – Microcontroller API used for pin definitions

### Custom Functions:
* write_to_pin(pin, *port, value) - Writes the designated value (0/1) to the entered pin and port
* set_pin_mode(pin, *port, mode) - Set the designed pin/port to be an input/output
* enable_rcc(*port) - Enable the reset control clock for the specified GPIO port

## CSE321_project2_mabautis_lcd1602.cpp:
File that declares the initialization and methods to operate the 1602 LCD for printing, clearing, and powering the display.

### Things Declared: 
* CSE321_LCD : Class definition to enable and utilize the 1602 LCD

### API and Built-In Elements Used:
* Mbed – Microcontroller API used for pin definitions
* lcd1602 - API to initialize and control 1602 LCD

### Custom Functions:
* clear() - Clear display and reset cursor to (0,0)
* setCursor(a,b) -  Puts cursor in col a and row b, note indexing starts at 0
* print("string") - Prints strings to LCD
