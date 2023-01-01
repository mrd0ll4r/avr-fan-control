# avr-fan-control

A project that uses an ATmega8 to control a bunch of relays for our ventilation setup.

## Setup

Some AVR MCU, eight relays, a two-digit seven-segment display, one button.
Using cmake, everything should be set up correctly.

## Mode of operation

Ventilation modes can be either set manually using a button on the device, or via I2C.
The I2C register file contains three bytes, one of which is a status byte.
The next two bytes are the air-in and air-out modes respectively.

Using the button overrides whatever was set via I2C and locks-out modification via I2C.
I2C can then be used to read the currently-set values.

All of this, and more, is explained in the source.

## License

MIT, but would be nice if you would link back here.
