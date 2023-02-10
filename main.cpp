

#include "DigitalOut.h"
#include "ThisThread.h"
#include "Ticker.h"
#include "mbed_thread.h"
#include <lcd.h>
#include <methods.h>
#include <cstdio>
#include <mbed.h>
#include <string>
#include <time.h>

void isr_col(void); // Rising edge Interrupt Service Routine for column pins
                    // [PF_14, PE_11, PE_9, PF_13]
void isr_falling_edge(void); // Falling edge Interrupt Service Routine for
                             // column pins [PF_14, PE_11, PE_9, PF_13]

void isr_microphone(void); // Rising edge ISR for micrphone PD_7

void isr_ultrasonic(void); // Rising edge ISR for ultrasonic sensor echo pin PD_5
void isr_ultrasonic_falling_edge(void); // Falling edge ISR for ultrasonic echo pin 

void ultrasonic_handler(void); // Handler after timeout is reached to see if object is within specified range
void trigger_ultrasonic_sensor(void); // Send 10us pulse to ultrasonic trigger pin

void microphone_handler(void); // Handles switching to triggered mode if sound is detected

void row_handler(void); // Thread callback that handles the powering of rows on the matrix keypad
void key_handler(void); // Thread callback that handles key presses based on current system mode

void power_on_mode(void); // Initial power on state where passcode is defined
void unarmed_mode(void); // Unarmed state where sensors do not trigger the system
void armed_mode(void); // Armed state (after entering passcode in unarmed mode) where sensors trigger the system
void triggered_mode(void); // State when a sensor is tripped in the armed state
void trigger_mode_transition(void); // Used to call blocking code from ultrasonic ISR when motion detected

void idle_timeout_handler(void); // Timeout handler after 10 seconds has passed without system input
void set_display_off(void); // Calls blocking code from idle timeout to set the display off and reset LCD text

const uint32_t TIMEOUT_MS = 5000; // Watchdog timeout before triggering system reset

int key_pressed = 0; // Determines if key is pressed (toggled by keypad ISRs)
int debounced = 0; // Determines if a key press is valid after debouncing it
int display_on = 1; // Flag to determine LCD state
volatile int echo_on = 0; // Determines if the echo pin is high or low (can change while thread is going to access it)

string password = "****"; // Passcode entered on system boot
string password_entered = "****"; // Passcode entered when attempting to switch between system modes
int password_position = 0; // Flag to determine which digit is being entered
int entering_password = 0; // Flag to determine if a passcode is being entered

LCD_EM LCD(16, 2, LCD_5x8DOTS, PB_9, PB_8); // Initialize LCD

// Initialize interrupts for columns of keypad. Set pull down to pull port down
// to 0 volts
InterruptIn col_0(PF_14, PullDown);
InterruptIn col_1(PE_11, PullDown);
InterruptIn col_2(PE_9, PullDown);
InterruptIn col_3(PF_13, PullDown);

InterruptIn microphone(PD_7, PullDown); // Initialize microphone Dout as an interrupt
InterruptIn ultrasonic_echo(PD_5, PullDown); // Initialize ultrasonic sensor echo as an interrupt

DigitalOut ultrasonic_trigger(PD_6); // Set ultrasonic trigger as a digit output
DigitalOut active_buzzer(PD_4); // Set active buzzer as a digit output
DigitalOut microphone_enable(PF_12); // Set pin going to microphone AND Gate as a digit output to enable and disable the mic interrupt pin
DigitalOut alarm_leds(PD_15); // Set LEDs as a digital output

Thread row_thread; // Declare thread handling keypad rows
Thread key_thread; // Declare thread maintaining system modes

Mutex resource_lock; // Declare mutex to maintain thread sychronization and protect against race conditions

EventQueue queue; // Initialize EventQueue to queue blocking code from ISR

Timeout idle_timeout; // Timeout to disable LCD backlight after 10 seconds
Timeout ultrasonic_timeout; // Timeout to check if ultrasonic echo is still active to determine object distance 

Ticker ultrasonic_ticker; // Ticker to trigger ultrasonic sensor pulses

char keypad[4][4] = {{'1', '2', '3', 'A'},
                     {'4', '5', '6', 'B'},
                     {'7', '8', '9', 'C'},
                     {'*', '0', '#', 'D'}}; // Enumerate keypad matrix

int mode = 0; // 0 -> Power On Mode (Define Code), 1 -> Unarmed, 2 -> Armed, 3
              // -> Triggered

int row = 0; // Current keypad row to power

int main() {
    // Enable interrupts
  col_0.enable_irq();
  col_1.enable_irq();
  col_2.enable_irq();
  col_3.enable_irq();
  
  // Enable clock control register for GPIO A & C
  enable_rcc('a');
  enable_rcc('c');

  // Declare GPIOA pin 3 as an output
  set_pin_mode(3, GPIOA, 1);
  // Declare GPIOC pin 0 as an output
  set_pin_mode(0, GPIOC, 1);
  // Declare GPIOC pin 3 as an output
  set_pin_mode(3, GPIOC, 1);
  // Declare GPIOC pin 1 as an output
  set_pin_mode(1, GPIOC, 1);

  LCD.begin(); // Initialize LCD
  LCD.print("Set Passcode: "); // Print prompt
  LCD.setCursor(0, 1); // Set cursor to next row

  // Declare interrupts for rising edge of each column of keypad
  col_0.rise(&isr_col);
  col_1.rise(&isr_col);
  col_2.rise(&isr_col);
  col_3.rise(&isr_col);

  microphone.rise(&isr_microphone); // Set microphone rising edge ISR

  ultrasonic_echo.rise(&isr_ultrasonic); // Set ultrasonic sensor rising edge ISR
  ultrasonic_echo.fall(&isr_ultrasonic_falling_edge); // Set ultrasonic sensor falling edge ISR

  // Declare interrupts for falling edge of each column of keypad
  col_0.fall(&isr_falling_edge);
  col_1.fall(&isr_falling_edge);
  col_2.fall(&isr_falling_edge);
  col_3.fall(&isr_falling_edge);

  idle_timeout.attach(&idle_timeout_handler, 10s); // Attach timeout to handle when system has not received user input
  
  ultrasonic_ticker.attach(&trigger_ultrasonic_sensor, 500ms); // Attach ticker to trigger ultrasonic sensor pulses

  row_thread.start(row_handler); // Start thread to handle keypad row powering
  key_thread.start(key_handler); // Start thread to handle system mode functions

  Watchdog &watchdog = Watchdog::get_instance(); // Initialize watchdog 
  watchdog.start(TIMEOUT_MS); // Start watchdog with specified timeout

  queue.dispatch_forever(); // Use main thread to handle any blocking code sent from ISR to the queue
}

void isr_col(void) { key_pressed = 1; } // Set flag to handle in debounce ticker

void isr_falling_edge(void) { // Set flag to indicate key no longer pressed
  key_pressed = 0;
  debounced = 0;
}

void isr_microphone(void) { 
  microphone_enable = 0; // Disable mic input to prevent ISR overflow
  queue.call(&microphone_handler); // Send ISR blocking code to Queue
}

void isr_ultrasonic(void) {
  echo_on = 1; // Set echo flag
  ultrasonic_timeout.attach(&ultrasonic_handler, 888us); // Set timeout to check detected object distance
}

void isr_ultrasonic_falling_edge(void) { echo_on = 0; } // Set echo flag

void microphone_handler() {
  resource_lock.lock(); // Lock system resources before modifying flags
  if (mode == 2) { // If armed, reset flags and enter triggered mode
    mode = 3;
    alarm_leds = 1;
    password_position = 0;
    entering_password = 0;
    active_buzzer = 1;
    LCD.clear();
    LCD.print("Triggered");
  }
  else { // Reenable microphone
      microphone_enable = 1;
  }
  resource_lock.unlock(); // Unlock system resources after modifying flags
}

void ultrasonic_handler() {
  if (mode == 2 && !echo_on) { // If armed and within triggering distance, handle in queue for blocking code
    queue.call(&trigger_mode_transition);
  }
}

void trigger_mode_transition() {
    // Reset flags, enable alarm outputs, and enter triggered mode
  mode = 3;
  alarm_leds = 1;
  password_position = 0;
  entering_password = 0;
  microphone_enable = 0;
  active_buzzer = 1;
  LCD.clear();
  LCD.print("Triggered");
}

void key_handler() {
  while (1) {
    resource_lock.lock(); // Lock system resources before modifying flags
    if (key_pressed) { // Check keypress
      if (!debounced) { // Debounce key if not already handled
        thread_sleep_for(10); // Debounce time is 10ms
        if (key_pressed) { // If key is still pressed, it is a valid press
          debounced = 1;
          if (!display_on) { // Turn display on if the system was in idle state
            display_on = 1;
            LCD.backlight();
          }
          idle_timeout.detach(); // Reset idle timeout
          idle_timeout.attach(&idle_timeout_handler, 10s);
          switch (mode) { // Handle the keypress based on current system mode
          case 0:
            power_on_mode();
            break;
          case 1:
            unarmed_mode();
            break;
          case 2:
            armed_mode();
            break;
          case 3:
            triggered_mode();
            break;
          }
        }
      }
    }
    resource_lock.unlock(); // Unlock system resources after modifying flags
  }
}

void row_handler() {
  while (1) {
    resource_lock.lock(); // Lock system resources before modifying flags
    if (!key_pressed) {
      row++;    // Increment row
      row %= 4; // Keep row between 0 and 3
      switch (row) {
      case 0: // Turn off other rows, turn on row 0
        write_to_pin(0, GPIOC, 0);
        write_to_pin(3, GPIOC, 0);
        write_to_pin(1, GPIOC, 0);
        write_to_pin(3, GPIOA, 1);
        break;
      case 1: // Turn off other rows, turn on row 1
        write_to_pin(3, GPIOA, 0);
        write_to_pin(3, GPIOC, 0);
        write_to_pin(1, GPIOC, 0);
        write_to_pin(0, GPIOC, 1);
        break;
      case 2: // Turn off other rows, turn on row 2
        write_to_pin(3, GPIOA, 0);
        write_to_pin(0, GPIOC, 0);
        write_to_pin(1, GPIOC, 0);
        write_to_pin(3, GPIOC, 1);
        break;
      case 3: // Turn off other rows, turn on row 3
        write_to_pin(3, GPIOA, 0);
        write_to_pin(0, GPIOC, 0);
        write_to_pin(3, GPIOC, 0);
        write_to_pin(1, GPIOC, 1);
        break;
      }
    }
    resource_lock.unlock(); // Unlock system resources after modifying flags
    Watchdog::get_instance().kick(); // Reset watchdog timer since user input is still working correctly and not blocked
  }
}

void power_on_mode() {
  if (col_0.read() && keypad[row][0] != '*') { // Check if column powered and if number pressed
    password[password_position] = keypad[row][0];
  } else if (col_1.read()) { // Check if column powered
    password[password_position] = keypad[row][1];
  } else if (col_2.read() && keypad[row][2] != '#') { // Check if column powered and if number pressed
    password[password_position] = keypad[row][2];
  }
  if ((col_0.read() && keypad[row][0] != '*') || col_1.read() ||
      (col_2.read() && keypad[row][2] != '#')) { // If valid number pressed, set password, update position, and update LCD
    password_position++;
    LCD.print("*");
    if (password_position == 4) {
      password_position = 0;
      mode = 1;
      LCD.clear();
      LCD.print("Unarmed");
    }
  }
}

void unarmed_mode() {
  if (col_0.read() && keypad[row][0] != '*' && entering_password) { // Check if column powered and if number pressed if entering password
    password_entered[password_position] = keypad[row][0];
  } else if (col_1.read() && entering_password) { // Check if column powered and if number pressed if entering password
    password_entered[password_position] = keypad[row][1];

  } else if (col_2.read() && keypad[row][2] != '#' && entering_password) { // Check if column powered and if number pressed if entering password
    password_entered[password_position] = keypad[row][2];
  } else if (col_3.read() && keypad[row][3] == 'A' && !entering_password) { // Check if column powered and if A pressed if NOT entering password
    entering_password = 1;
    LCD.clear();
    LCD.print("Enter Passcode: ");
    LCD.setCursor(0, 1);
  }
  if ((col_0.read() && keypad[row][0] != '*' && entering_password) ||
      (col_1.read() && entering_password) ||
      (col_2.read() && keypad[row][2] != '#' && entering_password)) { // Check for valid number press and then update flags
    password_position++;
    LCD.print("*");
    if (password_position == 4) { // If password entered, compare between actual password
      password_position = 0;
      entering_password = 0;
      if (password_entered == password) { // If correct password -> armed mode else stay in unarmed mode
        mode = 2;
        microphone_enable = 1;
        LCD.clear();
        LCD.print("Armed");
      } else {
        LCD.clear();
        LCD.print("Incorrect");
        LCD.setCursor(0, 1);
        LCD.print("Passcode");
        thread_sleep_for(2000);
        LCD.clear();
        LCD.print("Unarmed");
      }
    }
  }
}

void armed_mode() {
  if (col_0.read() && keypad[row][0] != '*' && entering_password) { // Check if column powered and if number pressed if entering password
    password_entered[password_position] = keypad[row][0];
  } else if (col_1.read() && entering_password) { // Check if column powered and if number pressed if entering password
    password_entered[password_position] = keypad[row][1];

  } else if (col_2.read() && keypad[row][2] != '#' && entering_password) { // Check if column powered and if number pressed if entering password
    password_entered[password_position] = keypad[row][2];
  } else if (col_3.read() && keypad[row][3] == 'A' && !entering_password) { // Check if column powered and if A pressed if NOT entering password
    entering_password = 1;
    LCD.clear();
    LCD.print("Enter Passcode: ");
    LCD.setCursor(0, 1);
  }
  if ((col_0.read() && keypad[row][0] != '*' && entering_password) ||
      (col_1.read() && entering_password) ||
      (col_2.read() && keypad[row][2] != '#' && entering_password)) {  // Check for valid number press and then update flags
    password_position++;
    LCD.print("*");
    if (password_position == 4) { // If password entered, compare between actual password
      password_position = 0;
      entering_password = 0;
      if (password_entered == password) { // If correct password -> unarmed mode else stay in armed mode
        mode = 1;
        LCD.clear();
        LCD.print("Unarmed");
      } else {
        LCD.clear();
        LCD.print("Incorrect");
        LCD.setCursor(0, 1);
        LCD.print("Passcode");
        thread_sleep_for(2000);
        LCD.clear();
        LCD.print("Armed");
      }
    }
  }
}

void triggered_mode() {
  if (col_0.read() && keypad[row][0] != '*' && entering_password) { // Check if column powered and if number pressed if entering password
    password_entered[password_position] = keypad[row][0];
  } else if (col_1.read() && entering_password) { // Check if column powered and if number pressed if entering password
    password_entered[password_position] = keypad[row][1];

  } else if (col_2.read() && keypad[row][2] != '#' && entering_password) { // Check if column powered and if number pressed if entering password
    password_entered[password_position] = keypad[row][2];
  } else if (col_3.read() && keypad[row][3] == 'A' && !entering_password) { // Check if column powered and if A pressed if NOT entering password
    entering_password = 1;
    LCD.clear();
    LCD.print("Enter Passcode: ");
    LCD.setCursor(0, 1);
  }
  if ((col_0.read() && keypad[row][0] != '*' && entering_password) ||
      (col_1.read() && entering_password) ||
      (col_2.read() && keypad[row][2] != '#' && entering_password)) {  // Check for valid number press and then update flags
    password_position++;
    LCD.print("*");
    if (password_position == 4) { // If password entered, compare between actual password
      password_position = 0;
      entering_password = 0;
      if (password_entered == password) {  // If correct password -> unarmed mode else stay in triggered mode
        mode = 1;
        LCD.clear();
        LCD.print("Unarmed");
        active_buzzer = 0;
        alarm_leds = 0;
      } else {
        LCD.clear();
        LCD.print("Incorrect");
        LCD.setCursor(0, 1);
        LCD.print("Passcode");
        thread_sleep_for(2000);
        LCD.clear();
        LCD.print("Triggered");
      }
    }
  }
}

void idle_timeout_handler() { // Handler acivated if system has idled without user input for 10s
  if (display_on) {
    display_on = 0;
    queue.call(&set_display_off); // Queue LCD blocking code from ISR
  }
}

void set_display_off() {
  LCD.noBacklight();  // Turn off LCD backlight since system is idling
  LCD.clear();
  password_position = 0; // Reset password flags
  entering_password = 0;
  switch (mode) { // Reset prompt to idle prompt
  case 0:
    LCD.print("Set Passcode: ");
    LCD.setCursor(0, 1);
    break;
  case 1:
    LCD.print("Unarmed");
    break;
  case 2:
    LCD.print("Armed");
    break;
  case 3:
    LCD.print("Triggered");
    break;
  }
}

void trigger_ultrasonic_sensor() {
  if (!echo_on) { // Wait for previous trigger to finish
    ultrasonic_trigger = 1; // Activate pulse
    wait_us(10); // Wait for specified time
    ultrasonic_trigger = 0; // Deactivate trigger pulse
    if (mode == 3) {
      alarm_leds = !alarm_leds; // Toggle alarm LEDs if in triggered mode
    }
  }
}
