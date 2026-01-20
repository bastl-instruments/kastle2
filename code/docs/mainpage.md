# Kastle 2

A modular DSP platform that fits into your pocket!

Kastle 2 is an open-source codebase embedded audio processing platform built on the Raspberry Pi RP2040 MCU using the official Pico SDK.

## Project Overview

- **Platform**: Raspberry Pi RP2040 (ARM Cortex-M0+ dual-core)
- **Memory**: 264 kB RAM, 8 MB flash storage
- **Audio**: 44kHz/16-bit stereo processing
- **I/O**: 18 analog inputs, 3 digital inputs, 3 analog outputs, 3 digital outputs
- **Connectivity**: USB-C, MIDI, audio I/O, sync I/O

## Development Resources

For comprehensive development information, visit the [Kastle 2 GitHub repository](https://github.com/bastl-instruments/kastle2/).

### Guides
- [Toolchain Installation](https://github.com/bastl-instruments/kastle2/blob/main/TOOLCHAIN_INSTALL.md) - Set up your development environment
- [Coding Style Guide](https://github.com/bastl-instruments/kastle2/blob/main/CODING_STYLE.md) - Project coding standards
- [Glossary & Examples](https://github.com/bastl-instruments/kastle2/blob/main/GLOSSARY_EXAMPLES.md) - Key concepts and code examples
- [FAQ](https://github.com/bastl-instruments/kastle2/blob/main/FAQ.md) - Common development issues
- [Application List](https://github.com/bastl-instruments/kastle2/blob/main/APP_LIST.md) - Overview of all available firmwares
- [MIDI Mappings](https://github.com/bastl-instruments/kastle2/blob/main/MIDI_MAPPINGS.md) - MIDI implementation details

## Architecture

The codebase is organized into several key components:

- **DSP Engine**: Real-time audio processing algorithms
- **Hardware Abstraction**: RP2040-specific peripheral drivers  
- **Application Framework**: Common functionality shared across firmwares
- **User Interface**: Knobs, buttons, LEDs
- **MIDI/USB**: Communication protocols

## License

- **Code**: MIT License
- **Schematics/Documentation**: CC BY SA 4.0
- **Panel graphics/Case artwork/Samples**: All rights reserved
