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

#include "BeatDetector.hpp"
#include <numbers>
#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/math/qmath.hpp"

using namespace kastle2;

#define FREQ_LP_BEAT 150.0f                                        // Low Pass filter frequency
#define T_FILTER (1.0f / (2.0f * std::numbers::pi * FREQ_LP_BEAT)) // Low Pass filter time constant
#define BEAT_RTIME 0.5f                                            // Release time of envelope detector in second

void BeatDetector::Init(float sample_rate)
// Compute all sample frequency related coeffs
{
    filter_1_out_ = Q15_ZERO;
    filter_2_out_ = Q15_ZERO;
    peak_env_ = Q15_ZERO;
    beat_trigger_ = false;
    beat_pulse_prev = false;

    k_beat_filter_ = float_to_q15(1.f / (sample_rate * T_FILTER));
    beat_release_ = float_to_q15(exp(-1.0f / (sample_rate * BEAT_RTIME)));
}

q15_t BeatDetector::AudioProcess(q15_t input)
// Process incoming signal
{
    q15_t env_in;

    // Step 1 : 3rd order low pass filter (made of two 1st order RC filter)
    filter_1_out_ = q15_add(filter_1_out_, q15_mult(k_beat_filter_, (q15_sub(input, filter_1_out_))));
    filter_2_out_ = q15_add(filter_2_out_, q15_mult(k_beat_filter_, (q15_sub(filter_1_out_, filter_2_out_))));
    filter_3_out_ = q15_add(filter_3_out_, q15_mult(k_beat_filter_, (q15_sub(filter_2_out_, filter_3_out_))));
    filter_3_out_ = q15_div(filter_3_out_, Q15_MAX - 100); // boost for better noise separation

    // Step 2 : peak detector
    env_in = q15_abs(filter_2_out_);
    if (env_in > peak_env_) // rising
    {
        peak_env_ = env_in; // Attack time = 0
    }
    else // falling
    {
        peak_env_ = q15_add(q15_mult(peak_env_, beat_release_), q15_mult(q15_sub(Q15_MAX, beat_release_), env_in));
    }
    // return peak_env_;

    // Step 3 : Schmitt trigger
    if (!beat_trigger_)
    {
        if (peak_env_ > 16384) // magic number = 0.5
        {
            beat_trigger_ = true;
        }
    }
    else
    {
        if (peak_env_ < 11469) // magic number = 0.35
        {
            beat_trigger_ = false;
        }
    }

    // Step 4 : rising edge detector
    beat_pulse_ = false;
    if (beat_trigger_ && !beat_pulse_prev)
    {
        beat_pulse_ = true;
    }
    beat_pulse_prev = beat_trigger_;

    return beat_pulse_;
}
