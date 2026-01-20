# FX Wizard File Format

The FX Wizard format is a file with a maximum size of 7.5 MB, located at RP2040 memory address 0x10080000. It's based on the Wave Bard file format, but it's simplified and contains just the core settings. Maybe some FX specific settings can be added in the future?

_Sections are 4-byte aligned to work with the ARM CPU memory layout._

| Field             | Value                                  | Size (bytes) |
| ----------------- | -------------------------------------- | ------------ |
| **Main Header**   |                                        | **20**       |
| Magic String      | "k2fx"                                 | 4            |
| File Size         | Total file size (including "end")      | 4            |
| Num Rhythms       | usually 16                             | 1            |
| Sequencer length  | usually 8                              | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
| Reserved          |                                        | 1            |
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
| **End Marker**    |                                        | **4**        |
| End Marker        | "ahoj"                                 | 4            |