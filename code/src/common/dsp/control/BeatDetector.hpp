/*
MIT License

Copyright (c) 2024 Marek Mach (Bastl Instruments)

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

#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

/**
 * @class BeatDetector
 * @ingroup dsp_control
 * @brief Audio beat detector for clock syncing and similar
 * @authors rf.eerf@retsaMPSD https://www.musicdsp.org/en/latest/Analysis/200-beat-detector-class.html, ported by Marek Mach (Bastl Instruments)
 * @date 2024-08-26
 */

class BeatDetector
{
private:
    q15_t k_beat_filter_ = 0; // Filter coefficient
    q15_t filter_1_out_ = 0;
    q15_t filter_2_out_ = 0;
    q15_t filter_3_out_ = 0;
    q15_t beat_release_ = 0;      // Release time coefficient
    q15_t peak_env_ = 0;          // Peak enveloppe follower
    bool beat_trigger_ = false;   // Schmitt trigger output
    bool beat_pulse_prev = false; // Rising edge memory
    bool beat_pulse_ = false;     // Beat detector output

public:
    /**
     * @brief Initializes the envelope follower
     * @param sample_rate How often is the AudioProcess() function called
     */
    void Init(float sample_rate);

    /**
     * @brief Processes the audio and returns if a bette was detected
     * @param input Audio input in q15_t format
     * @return If a beat was detected or not, in bool format
     */
    q15_t AudioProcess(q15_t input);
};
}
