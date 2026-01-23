# Kastle 2 Wave Bard

Sample player firmware for the Kastle 2 platform. Has three main parameters: Sample, Pitch and Length.  
Bank button switches between banks.   
By default there are 6 banks of 8 samples.

## Wave Bard Sample Format

The samples, scales and rhythms are packed in a special format. You can find the specs in the [WAVE_BARD_FORMAT.md](WAVE_BARD_FORMAT.md).

## SAMPLES.bin

Factory audio samples:  
Copyright (c) 2025 Oliver Torr for Bastl Instruments. All rights reserved.  

This file contains the factory default samples. You can generate a new file using Wave Bard Sample Loader with "Advanced" option on. In click on button labeled `BIN` to download the binary file containing samples.

http://apps.bastl-instruments.com/wave-bard-sample-loader/?advanced=1

After swapping SAMPLES.bin make sure to call task `clean` to make a clean build, otherwise CMake might store previously generated output.

*Regarding samples: The SAMPLES.BIN file in the repository should not be commercially distributed as part of a custom-built firmware, nor separately, as it is marked “All rights reserved.” However, using the official compiled firmware provided by Bastl (which includes the samples) is fine.*