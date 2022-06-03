//
// Created by leo on 28/05/22.
//
// This drives a two-digit seven-segment display.
// The display is connected to pins D0,D1,D2,D3,D4,D6 and C0,C1,C2,C3.
// This file manages those pins. All write operations are masked accordingly.
//
// Displaying digits on the display works by sequentially going over each digit and displaying that.
// The segment pins are shared between both digits, but each digit has its own enable line.

#ifndef FAN_CONTROL_SEGMENT_H
#define FAN_CONTROL_SEGMENT_H

#include <avr/io.h>

#define PORTD_SEGMENT_MASK ((1 << PORTD0) | (1 << PORTD1) | (1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4) | (1 << PORTD6))
#define PORTC_SEGMENT_MASK ((1 << PORTC0) | (1 << PORTC1) | (1 << PORTC2) | (1 << PORTC3))

#define PORTD_SEGMENT_MASK_NO_ENABLE  ((1 << PORTD0) | (1 << PORTD1) | (1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4))
#define PORTC_SEGMENT_MASK_NO_ENABLE  ((1 << PORTC1) | (1 << PORTC2) | (1 << PORTC3))

#define SEGMENT_RIGHT_ENABLE (1 << PORTD6)
#define SEGMENT_LEFT_ENABLE (1 << PORTC0)

#define SEGMENT_DOT (1 << PORTD0)
#define SEGMENT_TOP_MIDDLE (1 << PORTD1)
#define SEGMENT_TOP_RIGHT (1 << PORTD2)
#define SEGMENT_BOTTOM_RIGHT (1 << PORTD3)
#define SEGMENT_BOTTOM_MIDDLE (1 << PORTD4)

#define SEGMENT_TOP_LEFT (1 << PORTC1)
#define SEGMENT_MIDDLE_MIDDLE (1 << PORTC2)
#define SEGMENT_BOTTOM_LEFT (1 << PORTC3)

static uint8_t portd_segment_pattern(uint8_t num) {
    switch (num) {
        case 0:
            return SEGMENT_BOTTOM_RIGHT | SEGMENT_BOTTOM_MIDDLE | SEGMENT_TOP_RIGHT | SEGMENT_TOP_MIDDLE;
        case 1:
            return SEGMENT_TOP_RIGHT | SEGMENT_BOTTOM_RIGHT;
        case 2:
            return SEGMENT_TOP_MIDDLE | SEGMENT_TOP_RIGHT | SEGMENT_BOTTOM_MIDDLE;
        case 3:
            return SEGMENT_TOP_MIDDLE | SEGMENT_TOP_RIGHT | SEGMENT_BOTTOM_RIGHT | SEGMENT_BOTTOM_MIDDLE;
        case 4:
            return SEGMENT_TOP_RIGHT | SEGMENT_BOTTOM_RIGHT;
        case 5:
            return SEGMENT_TOP_MIDDLE | SEGMENT_BOTTOM_RIGHT | SEGMENT_BOTTOM_MIDDLE;
        case 6:
            return SEGMENT_TOP_MIDDLE | SEGMENT_BOTTOM_MIDDLE | SEGMENT_BOTTOM_RIGHT;
        case 7:
            return SEGMENT_TOP_MIDDLE | SEGMENT_TOP_RIGHT | SEGMENT_BOTTOM_RIGHT;
        case 8:
            return SEGMENT_BOTTOM_RIGHT | SEGMENT_BOTTOM_MIDDLE | SEGMENT_TOP_RIGHT | SEGMENT_TOP_MIDDLE;
        case 9:
            return SEGMENT_TOP_MIDDLE | SEGMENT_TOP_RIGHT | SEGMENT_BOTTOM_RIGHT | SEGMENT_BOTTOM_MIDDLE;
        default:
            return 0;
    }
}

static uint8_t portc_segment_pattern(uint8_t num) {
    switch (num) {
        case 0:
            return SEGMENT_TOP_LEFT | SEGMENT_BOTTOM_LEFT;
        case 1:
            return 0;
        case 2:
            return SEGMENT_MIDDLE_MIDDLE | SEGMENT_BOTTOM_LEFT;
        case 3:
            return SEGMENT_MIDDLE_MIDDLE;
        case 4:
            return SEGMENT_TOP_LEFT | SEGMENT_MIDDLE_MIDDLE;
        case 5:
            return SEGMENT_TOP_LEFT | SEGMENT_MIDDLE_MIDDLE;
        case 6:
            return SEGMENT_TOP_LEFT | SEGMENT_BOTTOM_LEFT | SEGMENT_MIDDLE_MIDDLE;
        case 7:
            return 0;
        case 8:
            return SEGMENT_TOP_LEFT | SEGMENT_MIDDLE_MIDDLE | SEGMENT_BOTTOM_LEFT;
        case 9:
            return SEGMENT_TOP_LEFT | SEGMENT_MIDDLE_MIDDLE;
        default:
            return 0;
    }
}

// Drives the display with the given digits.
// The enabled flags can be used to turn off the left or right digit, respectively.
// Internally, this displays the left and right digit for 5ms each, sequentially.
void drive_display(uint8_t left_digit, uint8_t right_digit, uint8_t left_digit_enabled, uint8_t right_digit_enabled) {
    uint8_t left_segment_portd = portd_segment_pattern(left_digit);
    uint8_t left_segment_portc = portc_segment_pattern(left_digit);
    uint8_t right_segment_portd = portd_segment_pattern(right_digit);
    uint8_t right_segment_portc = portc_segment_pattern(right_digit);

    if (left_digit_enabled) {
        PORTD &= ~PORTD_SEGMENT_MASK;
        PORTC &= ~PORTC_SEGMENT_MASK;

        PORTD |= (PORTD_SEGMENT_MASK_NO_ENABLE & ~left_segment_portd);
        PORTC |= (SEGMENT_LEFT_ENABLE | (PORTC_SEGMENT_MASK_NO_ENABLE & ~left_segment_portc));
        _delay_ms(5);
    }

    if (right_digit_enabled) {
        PORTD &= ~PORTD_SEGMENT_MASK;
        PORTC &= ~PORTC_SEGMENT_MASK;

        PORTD |= (SEGMENT_RIGHT_ENABLE | (PORTD_SEGMENT_MASK_NO_ENABLE & ~right_segment_portd));
        PORTC |= (PORTC_SEGMENT_MASK_NO_ENABLE & ~right_segment_portc);
        _delay_ms(5);
    }
}

// Sets up outputs for the segment display.
void segment_io_init() {
    PORTD |= PORTD_SEGMENT_MASK;
    PORTC |= PORTC_SEGMENT_MASK;

    DDRD |= PORTD_SEGMENT_MASK;
    DDRC |= PORTC_SEGMENT_MASK;
}

#endif //FAN_CONTROL_SEGMENT_H
