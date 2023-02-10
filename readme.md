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

