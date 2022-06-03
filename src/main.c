/*
* main.c
*
* Created: 03-Jun-21
* Author : Leo
*/

/*
Mode of operation:
- The button can be used to manually change values. For that:
  - If it's long-pressed, the selected digit is changed.
  - The selected digit is blinking.
  - If no digit is selected, values can be set via I2C.
  - If _any_ digit is selected, _no_ values will be accepted via I2C.
- The I2C register file contains three registers:
  - 0x00 is a status byte. The rightmost bit indicates whether writing via I2C is currently _disabled_. The next bit indicates whether _no_ watchdog reset has occurred.
  - 0x01 is the air-intake mode. If manual mode is active, this can be read to get the currently selected mode. Otherwise, it can be written to set a mode.
  - 0x02 is the air-out mode, same as above.
  - Only after register 0x02 is written are the changes to registers 0x01 and 0x02 processed!
- The relays controlled by this form two binary trees. See the relay.h source for more information on that.
- A seven-segment display is attached, which is controlled by segment.h.
*/

#ifndef F_CPU
# define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "twislave.h"
#include "segment.h"
#include "relay.h"

// The currently active air-intake mode.
// This is volatile since it is being changed from within the timer interrupt on button presses.
volatile uint8_t air_mode_in = 0;
// The currently active air-out mode.
// This is volatile since it is being changed from within the timer interrupt on button presses.
volatile uint8_t air_mode_out = 0;
// Whether the left digit should be displayed.
// This is volatile since it is being changed from within the timer interrupt on button presses.
volatile uint8_t left_digit_on = 1;
// Whether the right digit should be displayed.
// This is volatile since it is being changed from within the timer interrupt on button presses.
volatile uint8_t right_digit_on = 1;

// Initialize outputs.
static void io_init() {
    relay_io_init();

    segment_io_init();

    // The button is connected to PIND5.
    DDRD &= ~(1 << DDD5);
}

// Initialize the timer.
// This sets up timer 1 to fire at 100 Hz, i.e. every 10ms.
// Timer interrupts have higher priority than I2C, but I don't know if that could cause problems irl...
void init_timer() {
    // See http://www.8bit-era.cz/arduino-timer-interrupts-calculator.html
    // Or https://www.arduinoslovakia.eu/application/timer-calculator

    TCCR1A = 0; // set entire TCCR1A register to 0
    TCCR1B = 0; // same for TCCR1B
    TCNT1 = 0; // initialize counter value to 0
    // set compare match register for 100 Hz increments
    OCR1A = 9999; // = 8000000 / (8 * 100) - 1 (must be <65536)
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS12, CS11 and CS10 bits for 8 prescaler
    TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
    // enable timer compare interrupt
    TIMSK |= (1 << OCIE1A);
}

// The number of digits on the segment, which is also the number of modes that can be changed using the button.
#define NUM_DIGITS 2
// The blinking duration to show a digit is selected, in multiples of 10ms.
#define TIMER_CNT_THRESH 30

ISR(TIMER1_COMPA_vect) // every 10ms
{
    static uint8_t selected_digit = 0;
    static uint8_t down_for_cycles = 0;
    static uint8_t timer_cnt = 0;

    // The button is connected to PIND5, but HIGH by default.
    uint8_t is_pressed = !(PIND & (1 << PIND5));
    if (is_pressed) {
        down_for_cycles++;

        if (down_for_cycles == 50) {
            // Long press
            selected_digit = (selected_digit + 1) % (NUM_DIGITS + 1);
            // Set the blink counter to trigger immediately.
            timer_cnt = TIMER_CNT_THRESH - 1;
            if (selected_digit == 0) {
                i2c_write_disabled = 0;
            } else {
                i2c_write_disabled = 1;
            }
        }
        if (down_for_cycles > 250) {
            // This will overflow soon, so we reset it to something low now.
            down_for_cycles = 51;
        }
    } else {
        // Button up
        if (down_for_cycles > 1 && down_for_cycles < 50) {
            // Short press
            if (selected_digit == 1) {
                air_mode_in = (air_mode_in + 1) % NUM_AIR_IN_MODES;
            } else if (selected_digit == 2) {
                air_mode_out = (air_mode_out + 1) % NUM_AIR_OUT_MODES;
            }
        }
        down_for_cycles = 0;
    }

    // Make selected digit blink.
    if (selected_digit == 0) {
        // No digit selected, no blinking.
        timer_cnt = 0;
        left_digit_on = 1;
        right_digit_on = 1;
    } else {
        timer_cnt++;
        if (timer_cnt == TIMER_CNT_THRESH) {
            timer_cnt = 0;
            if (selected_digit == 1) {
                right_digit_on = 1;
                left_digit_on = !left_digit_on;
            } else {
                left_digit_on = 1;
                right_digit_on = !right_digit_on;
            }
        }
    }
}

int main(void) {
    // Set inputs/outputs.
    io_init();

    // Clear I2C register buffer.
    for (int i = 0; i < i2c_buffer_size; i++) {
        i2cdata[i] = 0;
    }
    i2cdata[1] = air_mode_in;
    i2cdata[2] = air_mode_out;

    // Mark watchdog reset in status byte.
    if (MCUCSR & (1 << WDRF)) {
        // A reset by the watchdog has occurred.
        // Signal this by clearing bit 2 in status byte.
        i2cdata[0] &= ~(I2C_BIT_WDT_RESET);
        // Clear flag for next time.
        MCUCSR &= ~(1 << WDRF);
    } else {
        // 1 means no WDT reset occurred.
        i2cdata[0] |= I2C_BIT_WDT_RESET;
    }

    // Enable watchdog to restart if we didn't reset it for 2 seconds.
    wdt_enable(WDTO_2S);
    // Enable I2C.
    init_twi_slave(I2C_SLAVE_ADDRESS);
    // Enable timer.
    init_timer();

    // Enable Interrupts.
    sei();

    while (1) {
        // Reset watchdog timer.
        wdt_reset();

        // If we got new values via I2C...
        {
            cli();
            if (i2c_write_disabled) {
                i2cdata[0] |= I2C_BIT_I2C_DISABLED;
                i2cdata[1] = air_mode_in;
                i2cdata[2] = air_mode_out;
            } else {
                i2cdata[0] &= ~I2C_BIT_I2C_DISABLED;
                if (i2c_fully_written) {
                    if (i2cdata[1] < NUM_AIR_IN_MODES)
                        air_mode_in = i2cdata[1];
                    if (i2cdata[2] < NUM_AIR_OUT_MODES)
                        air_mode_out = i2cdata[2];
                }
            }
            i2c_fully_written = 0;
            sei();
        }

        // Set relays.
        drive_relays(air_mode_in, air_mode_out);

        // Set display.
        drive_display(air_mode_in, air_mode_out, left_digit_on, right_digit_on);
    }
}