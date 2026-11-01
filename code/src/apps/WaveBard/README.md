# Kastle 2 Wave Bard

Sample player firmware for the Kastle 2 platform. Has three main parameters: Sample, Pitch and Length.  
Bank button switches between banks.   

Hold the Bank button and turn the Sample knob to select 3, 6, 9, 13, 16, 19, 22, 28, or 32 sequencer steps from left to right. The nine choices are evenly distributed across the knob, with 16 steps at the midpoint. The upper-left LED briefly flashes off only when the selected step count changes, and the length takes effect on the next step without restarting playback.

Quickly press and release SHIFT + Bank together to start or stop the sequencer clock. Tap tempo is disabled for this combination in Wave Bard; holding both buttons still enters Settings as before.

The tempo LED flashes once per bar in Wave Bard (once every four beats at 120 BPM). This only slows the indicator, not playback, and avoids the artificial fast-tempo blinking used by other apps.

In Wave Bard, patching the CV feed low randomizes the sequencer CV on each step. The resulting CV is sent from the CV output and can be patched to any compatible input.

By default there are 6 banks of 8 samples.

## How to build

Running `make wave-bard` produces a firmware image with just the app itself—ideal for development or upgrade process.

Running `make wave-bard-with-samples` builds the app and appends the data from SAMPLES.bin, creating the complete firmware. That's the way the official firmwares are generated.

## Wave Bard Sample Format

The samples, scales and rhythms are packed in a special format. You can find the specs in the [WAVE_BARD_FORMAT.md](WAVE_BARD_FORMAT.md).

## SAMPLES.bin

Factory audio samples:  
Copyright (c) 2025 Oliver Torr for Bastl Instruments. All rights reserved.  

This file contains the factory default samples. You can generate a new file using Wave Bard Sample Loader with "Advanced" option on. In click on button labeled `GENERATE BIN` to download the binary file containing samples.

http://apps.bastl-instruments.com/wave-bard-sample-loader/?advanced=1

After swapping SAMPLES.bin make sure to call task `clean` to make a clean build, otherwise CMake might store previously generated output.

*Regarding samples: The SAMPLES.BIN file in the repository should not be commercially distributed as part of a custom-built firmware, nor separately, as it is marked “All rights reserved.” However, using the official compiled firmware provided by Bastl (which includes the samples) is fine.*