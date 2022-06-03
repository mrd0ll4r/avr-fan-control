//
// Created by leo on 29/05/22.
//
// We have eight relays. The first four deal with air intake, the last four with air out.
// All relays are in their default state if the corresponding pins are HIGH.
// All relays are connected to PORTB. This file manages PORTB.
//
// ==============================
// AIR INTAKE
// ==============================
//
// Relays 1 and 3 have power inputs for air intake.
// Relay 1:
// - Not connected (default)
// - 100V (active)
// Relay 3:
// - 190V (default)
// - 230V (active)
//
// Relay 2 sits above that to multiplex. It has relay 1 selected by default.
// Relay 3 demuxes the output of relay 2 to the two fan modes:
// - Intake air fan mode 1 (default)
// - Intake air fan mode 2 (active)
//
// We thus have six air intake modes, plus off:
// - off: all relays in default position
// - mode 1: 100V + fan mode 1
// - mode 2: 100V + fan mode 2
// - mode 3: 190V + fan mode 1
// - mode 4: 230V + fan mode 1
// - mode 5: 190V + fan mode 2
// - mode 6: 230V + fan mode 2
//
// Additionally, there is one connection between the fan-controlling MCU and the heater-controlling MCU, which is
// hardwired to relay 1, with a pullup.
// This connection is used as a safety feature: Only if the pin is LOW will the heating run.
// This means that, even if the "path" via relay 1 is unused, we still need to actively drive it LOW to indicate that
// the fan is running.
//
// ==============================
// AIR OUT
// ==============================
//
// Relays 5,6, and 8 have power inputs. All arrays are arranged like a tree again:
// Relay 5:
// - Not connected (default)
// - 80 V (active)
// Relay 7:
// - 120V (default)
// - 150V (active)
// Relay 8:
// - [output of relay 6] (default)
// - 230V (active)
// Relay 6 multiplexes between relays 5 and 7.
//
// We thus end up with 4 air-out modes, plus off.

#ifndef FAN_CONTROL_RELAY_H
#define FAN_CONTROL_RELAY_H

#include <avr/io.h>

// Relay 1 is hardwired to the second MCU with a pullup, so it has to be on B5.
// Also, we have to keep that ON whenever air is coming in, even if the "path" is unused.
// Also, our relays are HIGH by default, i.e., all of this is actually inverted.
#define RELAY_1 (1 << PORTB5)
#define RELAY_2 (1 << PORTB1)
#define RELAY_3 (1 << PORTB2)
#define RELAY_4 (1 << PORTB3)
#define RELAY_5 (1 << PORTB4)
#define RELAY_6 (1 << PORTB0)
#define RELAY_7 (1 << PORTB6)
#define RELAY_8 (1 << PORTB7)

#define AIR_IN_MASK (RELAY_1 | RELAY_2 | RELAY_3 | RELAY_4)
#define AIR_OUT_MASK (RELAY_5 | RELAY_6 | RELAY_7 | RELAY_8)

#define NUM_AIR_IN_MODES 7
#define NUM_AIR_OUT_MODES 5

// Do not use this.
static uint8_t relay_pattern_air_in_inverted(uint8_t level) {
    switch (level) {
        case 0:
            return 0;
        case 1:
            return RELAY_1;
        case 2:
            return RELAY_1 | RELAY_4;
        case 3:
            return RELAY_1 | RELAY_2;
        case 4:
            return RELAY_1 | RELAY_3 | RELAY_2;
        case 5:
            return RELAY_1 | RELAY_2 | RELAY_4;
        case 6:
            return RELAY_1 | RELAY_2 | RELAY_3 | RELAY_4;
        default:
            return 0;
    }
}

// Computes a masked pattern for PORTB for the given air intake level.
static uint8_t relay_pattern_air_in(uint8_t level) {
    // Our relays are in their default position when HIGH, which means we need to invert some stuff.
    return (~relay_pattern_air_in_inverted(level) & AIR_IN_MASK);
}

// Do not use this.
static uint8_t relay_pattern_air_out_inverted(uint8_t level) {
    switch (level) {
        case 0:
            return 0;
        case 1:
            return RELAY_5;
        case 2:
            return RELAY_6;
        case 3:
            return RELAY_6 | RELAY_7;
        case 4:
            return RELAY_8;
        default:
            return 0;
    }
}

// Computes a masked pattern for PORTB for the given air out level.
static uint8_t relay_pattern_air_out(uint8_t level) {
    return (~relay_pattern_air_out_inverted(level) & AIR_OUT_MASK);
}

static uint8_t portb_relay_pattern(uint8_t air_mode_in, uint8_t air_mode_out) {
    uint8_t relay_pattern = 0;

    relay_pattern |= relay_pattern_air_in(air_mode_in);
    relay_pattern |= relay_pattern_air_out(air_mode_out);

    return relay_pattern;
}

// Computes and sets the outputs for the relays according to the given modes.
void drive_relays(uint8_t air_mode_in, uint8_t air_mode_out) {
    // Compute relay pattern.
    uint8_t relay_pattern = portb_relay_pattern(air_mode_in, air_mode_out);

    // Set relays
    PORTB = relay_pattern;
}


// Initializes outputs used for the relays.
// Sets bank B to outputs and writes the default pattern for (in,out) modes (0,0).
void relay_io_init() {
    PORTB = portb_relay_pattern(0, 0);

    DDRB |= 0xFF;
}

#endif //FAN_CONTROL_RELAY_H
