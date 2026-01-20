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

#include "CorrectingTrackAndHold.hpp"
#include "common/dsp/math/qmath.hpp"

using namespace kastle2;

FASTCODE q15_t CorrectingTrackAndHold::Process(q15_t audio_input, q31_t control_input)
{
    // If the control input is above the threshold, hold the last output
    if (control_input >= threshold_)
    {
        // hold
    }
    else if (control_input < last_control_input_ && last_control_input_ <= threshold_ && threshold_ < Q31_MAX)
    {
        // the control input almost reached the threshold, meaning it would only be held for one sample, hold it for one sample
        // hold
    }
    else
    {
        // track
        out_15_ = audio_input;
    }

    last_control_input_ = control_input;

    return out_15_;
}

FASTCODE q31_t CorrectingTrackAndHold::Process31(q31_t audio_input, q31_t control_input)
{
    // If the control input is above the threshold, hold the last output
    if (control_input >= threshold_)
    {
        // hold
    }
    else if (control_input < last_control_input_ && last_control_input_ <= threshold_ && threshold_ < Q31_MAX)
    {
        // the control input almost reached the threshold, meaning it would only be held for one sample, hold it for one sample
        // hold
    }
    else
    {
        // track
        out_31_ = audio_input;
    }

    last_control_input_ = control_input;

    return out_31_;
}
