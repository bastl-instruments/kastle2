# Wave Bard Sample Format

The Wave Bard binary format is a file with a maximum size of 7.5 MB, located at RP2040 memory address 0x10080000. It contains raw signed 16-bit audio samples, without WAV header. It follows the application space, which begins at 0x10000000 and spans 512 kB. To ensure precise alignment, the Wave Bard firmware is padded with zeros to exactly 512 kB before appending this binary file. Finally, the combined data is converted into the UF2 format using a Python script.

_Sections are 4-byte aligned to work with the ARM CPU memory layout._

| Field             | Value                                  | Size (bytes) |
| ----------------- | -------------------------------------- | ------------ |
| **Main Header**   |                                        | **20**       |
| Magic String      | "k2wb"                                 | 4            |
| File Size         | Total file size (including "end")      | 4            |
| Sample Rate       | eg. 44100                              | 4            |
| Bit Depth         | only 16 is now supported               | 1            |
| Num Banks         | usually 1 to 9                         | 1            |
| Num Samples       | usually 8                              | 1            |
| Num Scales        | usually 7                              | 1            |
| Num Rhythms       | usually 16                             | 1            |
| Sequencer Length  | usually 16                             | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| **Scales**        | LSB First (12 bits)                    | **28?**      |
| Minor Chord       | 0b000010001001                         | 4            |
| Minor Pentatonic  | 0b101010110101                         | 4            |
| Minor Diatonic    | 0b010110101101                         | 4            |
| Chromatic         | 0b111111111111                         | 4            |
| Major Diatonic    | 0b101010110101                         | 4            |
| Major Pentatonic  | 0b001010010101                         | 4            |
| Major Chord       | 0b000010010001                         | 4            |
| **Rhythms**       | MSB first (16 bits)                    | **64?**      |
| Rhythm 1          | 0b1000000010000000                     | 4            |
| Rhythm 2          | 0b1000100010001000                     | 4            |
| Rhythm 3          | 0b1000101010001010                     | 4            |
| Rhythm 4          | 0b1000001001001000                     | 4            |
| Rhythm 5          | 0b1000000000100000                     | 4            |
| Rhythm 6          | 0b1010000110100000                     | 4            |
| Rhythm 7          | 0b1000100000101000                     | 4            |
| Rhythm 8          | 0b1000100010010010                     | 4            |
| Rhythm 9          | 0b0010001000100010                     | 4            |
| Rhythm 10         | 0b0010010001000110                     | 4            |
| Rhythm 11         | 0b0010001000110010                     | 4            |
| Rhythm 12         | 0b1111101010111010                     | 4            |
| Rhythm 13         | 0b0000111100001100                     | 4            |
| Rhythm 14         | 0b1010110010101100                     | 4            |
| Rhythm 15         | 0b0011001100110010                     | 4            |
| Rhythm 16         | 0b0111011101110111                     | 4            |
| **Bank Header 1** |                                        | **12**       |
| Name              | "bank1111"                             | 8            |
| Color R           | 0-255                                  | 1            |
| Color G           | 0-255                                  | 1            |
| Color B           | 0-255                                  | 1            |
| Reserved          |                                        | 1            |
| **Sample 1**      |                                        | **16-??**    |
| Sample Size       | eg. 22050 (means 11025 16-bit samples) | 4            |
| Sample Channels   | 1 or 2                                 | 1            |
| Sample Name       | "sample01"                             | 8            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Sample Data       | Audio data                             | eg. 22050    |
| **Sample 2**      |                                        | **16-??**    |
| Sample Size       | eg. 22050 (means 11025 16-bit samples) | 4            |
| Sample Channels   | 1 or 2                                 | 1            |
| Sample Name       | "sample02"                             | 8            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Sample Data       | Audio data                             | eg. 22050    |
| **Bank Header 2** |                                        | **12**       |
| Name              | "bank2222"                             | 8            |
| Color R           | 0-255                                  | 1            |
| Color G           | 0-255                                  | 1            |
| Color B           | 0-255                                  | 1            |
| Reserved          |                                        | 1            |
| **Sample 1**      |                                        | **16-??**    |
| Sample Size       | eg. 22050 (means 11025 16-bit samples) | 4            |
| Sample Channels   | 1 or 2                                 | 1            |
| Sample Name       | "sample01"                             | 8            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Sample Data       | Audio data                             | eg. 22050    |
| **End Marker**    |                                        | **4**        |
| End Marker        | "ahoj"                                 | 4            |