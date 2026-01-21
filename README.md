# Kastle 2

*Compatible with [Bastl Instruments Citadel](https://github.com/bastl-instruments/citadel)*

A modular DSP platform that fits into your pocket!  
Comes with open-source codebase (including two official firmwares) and schematics.

<img src="./images/readme/kastle2-fx-wizard.webp" width="250" alt="Kastle 2" /> 

## How to develop for Kastle 2

Read [Toolchain Install Guide](TOOLCHAIN_INSTALL.md), [Coding Style Guide](CODING_STYLE.md) and [Glossary & Examples](GLOSSARY_EXAMPLES.md).  
Any questions or weird stuff hapenning? See the [FAQ](FAQ.md) or discuss your experience on our [Discord server](https://discord.com/invite/C4RXRjgYJ6).

## Specifications

- Raspberry Pi RP2040 MCU (264 kB RAM, clocked at 176 MHz)
- 8 MB flash for firmware and data
- 256 B EEPROM for storing calibration and settings
- 44kHz/16-bit stereo audio quality
- USB-C or 3xAA battery power (with automatic switching)
- 18 analog inputs (12-bit)
  - 2 direct inputs (sample rate > 1 kHz)
  - 16 multiplexed inputs (sample rate > 100 Hz)
- 3 digital inputs
- 3 analog outputs (using filtered 10-bit PWM)
- 3 digital outputs
- Stereo audio input (up to 6 Vpp)
- Stereo headphone output (suitable for up to 250 Ohm headphones)
- 3 RGB LEDs
- USB MIDI input & output - see the [MIDI Mappings](MIDI_MAPPINGS.md)

All inputs are designed to accept voltages in the range of 0-5V unless specified.  
All outputs are designed to provide 0-5V (0V to the input voltage, respectively).

## Connectivity
- USB-C (power, firmware updates, MIDI)
- Sync in jack (expects a clock signal at tip)
- Sync out jack (produces a clock signal at tip)
- Audio in jack
- Headphone out jack

## User interface
- 7 potentiometers
- 2 buttons
- Pin headers for patching
- Power switch

## Power consumption, battery life

Power consumption is approximately 100-150 mA, allowing for up to 15-18 hours of use on 3xAA batteries.


## Open Source Firmwares

The full firmware list is here: [app list](APP_LIST.md).

**FX Wizard**   
Nine time-based audio effects, with DJ-style lowpass/highpass filter.

**Wave Bard**   
Stereo sample player featuring unique sequencing techniques.

And other examples, see the `code/src/apps` subfolder.

## Useful links

**[Citadel repository](https://github.com/bastl-instruments/citadel)**  
A Eurorack adaptation of the Kastle 2 family. It's directly compatible with the Kastle 2 firmwares, there are no extra Citadel firmware builds. The firmware itself detects the variant it's running on (based on the MIDI pull-up resistor on RX line) and adjusts necessary parameters.

**[Kastle 2 Web Apps repository](https://github.com/bastl-instruments/kastle2-webapps)**  
Source codes for Wave Bard Sample Loader, Alchemist Laboratory…

**[Kastle 2 Label Maker repository](https://github.com/bastl-instruments/kastle2-labelmaker)**  
Source code for a tool which helps you visualize Kastle 2 app ideas.

## Credits

**Developed by**   
Václav Mach ([@xx0x](https://github.com/xx0x)), Marek Mach ([@machmar](https://github.com/machmar))

**Supervised by**  
Václav Peloušek ([@vaclav-bastl](https://github.com/vaclav-bastl)), Martin Klecl ([@martinklecl](https://github.com/martinklecl))

Thanks to Majkel ([@tenmajkl](https://github.com/tenmajkl)) with help around documentation and everyone at Bastl Instruments for their support while working on this!

## License

**Code**  
MIT license

**Schematics, Documentation**  
CC BY SA 4.0 license

**Voice recordings (Version, Calibration, Test Mode)**  
CC0 license, made using TTS Maker (voice 178 - Chloe) and sped up by 1.05x

**Panel graphics, Case artwork**  
All rights reserved

**Samples (eg. Wave Bard factory bank)**  
All rights reserved

*Note: You are welcome to create derived products based on the schematics and software. However, Bastl retains proprietary rights to the board layout, which is not publicly available. This prevents simple product cloning and maintains the integrity of the original Kastle 2. By purchasing an original Bastl Instruments Kastle 2 you support ongoing development.*