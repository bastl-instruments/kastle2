/*
MIT License

Copyright (c) 2024 Vaclav Mach (Bastl Instruments)

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

// INPUTS CCs
static constexpr uint8_t MAIN_VOLUME = 7; // main volume
static constexpr uint8_t INPUT_GAIN = 9;  // input gain
static constexpr uint8_t LFO = 22;        // lfo (bottom right knob)
static constexpr uint8_t LFO_MOD = 23;    // lfo mod (bottom left knob)
static constexpr uint8_t TEMPO = 24;      // tempo (shift+ bottom right knob)
static constexpr uint8_t RHYTHM = 25;     // rhythm (shift + bottom left knob)

static constexpr uint8_t RESET_CONTROLLERS = 121; // reset all controllers
static constexpr uint8_t ALL_NOTES_OFF = 123;     // all notes off

// OUTPUTS CCs
static constexpr uint8_t OUT_POT_1 = 15;
static constexpr uint8_t OUT_POT_2 = 17;
static constexpr uint8_t OUT_POT_3 = 23;
static constexpr uint8_t OUT_POT_4 = 18;
static constexpr uint8_t OUT_POT_5 = 14;
static constexpr uint8_t OUT_POT_6 = 16;
static constexpr uint8_t OUT_POT_7 = 22;

}
}