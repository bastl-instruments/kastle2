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

#include "hardware/timer.h"

/**
 * @file debug.hpp
 * @ingroup debug
 * @brief Common debug settings and utilities.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-03-27
 */

namespace kastle2
{

// What to debug/measure (output duty cycle on first debug pin, usually TX pin)
#define MEASURE_AUDIO_LOOP 1
#define MEASURE_UI_LOOP 0
#define MEASURE_ADC_CYCLE 0

// Which debug output to use
#define DEBUG_ONE_USING_SYNC_OUT 0
#define DEBUG_ONE_USING_TX 1
#define DEBUG_TWO_USING_RX 0 // RX is used for MIDI on Citadel, so it's not available for debugging there

// Test mode
#define ALWAYS_GO_TO_TEST_MODE 0

/**
 * @brief Fix debugging with Pi Debug Probe
 * @see https://github.com/raspberrypi/pico-sdk/issues/1622
 */
inline void fix_pi_probe_debugging()
{
    timer_hw->dbgpause = 0;
}

}
