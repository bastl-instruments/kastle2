/*
MIT License

Copyright (c) 2025 Marek Mach (Bastl Instruments)
Copyright (c) 2025 Vaclav Mach (Bastl Instruments)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <cstdint>

namespace kastle2
{

namespace cc
{

// Receive
static constexpr uint8_t BANK = 1;        // mapped across x number of values (0-127 mapped to 1-6 etc)
static constexpr uint8_t PITCH = 14;      // top right knob
static constexpr uint8_t PITCH_MOD = 15;  // top left knob
static constexpr uint8_t SAMPLE = 16;     // mid right knob
static constexpr uint8_t SAMPLE_MOD = 17; // mid left knob
static constexpr uint8_t LENGTH = 18;     // center knob
static constexpr uint8_t LENGTH_MOD = 19; // shift + center knob
static constexpr uint8_t FILTER = 20;     // shift + mid right knob
static constexpr uint8_t FX = 21;         // shift + mid left knob
static constexpr uint8_t BANK_MOD = 26;   // mode + center knob
static constexpr uint8_t SCALE = 27;
static constexpr uint8_t ROOT = 28;
static constexpr uint8_t FINE = 29;

// Send
static constexpr uint8_t OUT_BANK_SAMPLE_COMBO = 1;
static constexpr uint8_t OUT_FORWARD_LENGTH = 30;   // 0x1e
static constexpr uint8_t OUT_BACKWARD_LENGTH = 31;  // 0x1f
static constexpr uint8_t OUT_BANK = 32;             // 0x20
static constexpr uint8_t OUT_SAMPLE_CONTINOUS = 33; // 0x21
static constexpr uint8_t OUT_SAMPLE_SIMPLE = 34;    // 0x22

}
}