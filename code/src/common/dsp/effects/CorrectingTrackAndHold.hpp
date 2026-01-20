/*
MIT License

Copyright (c) 2025 Marek Mach (Bastl Instruments)

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

#include "common/fastcode.hpp"
#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

/**
 * @class CorrectingTrackAndHold
 * @ingroup dsp_effects
 * @brief Audio sample and hold correcting for threshold position with frequency.
 * @authors Marek Mach (Bastl Instruments)
 * @date 2025-06-25
 */
class CorrectingTrackAndHold
{

public:

    /**
     * @brief Sets the threshold above which to hold the sample.
     *        If the threshold isn't reached, it'll adjust based on the zero crossing of the control input.
     * @param threshold The threshold value in Q31 format.
     */
    void SetThreshold(q31_t threshold)
    {
        threshold_ = threshold;
    }

    /**
     * @brief Processes the audio input with the sample and hold based on the control input.
     * @param audio_input The audio input sample in Q15 format.
     * @param control_input The control input sample in Q31 format.
     * @return The processed audio output sample in Q15 format.
     */
    FASTCODE q15_t Process(q15_t audio_input, q31_t control_input);

    /**
     * @brief Processes the audio input with the sample and hold based on the control input.
     * @param audio_input The audio input sample in Q31 format.
     * @param control_input The control input sample in Q31 format.
     * @return The processed audio output sample in Q31 format.
     */
    FASTCODE q31_t Process31(q31_t audio_input, q31_t control_input);


private:
    q31_t threshold_ = Q31_ZERO;
    q15_t out_15_ = Q15_ZERO;
    q31_t out_31_ = Q31_ZERO;
    q31_t last_control_input_ = Q31_ZERO;
};
}
