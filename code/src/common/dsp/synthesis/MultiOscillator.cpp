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

#include "MultiOscillator.hpp"
#include "common/dsp/math/math_utils.hpp"

using namespace kastle2;

void MultiOscillator::Init(const float sample_rate)
{
    sample_rate_ = sample_rate;
    phase_ = Q31_MIN;
    pulse_width_ = Q31_ZERO; // 50% duty cycle
    SetFrequency(440); // 440 Hz
}

void MultiOscillator::SetFrequency(const float frequency)
{
    SetNativeFrequency(freq_to_q31(frequency, sample_rate_));
}

void MultiOscillator::SetNativeFrequency(const q31_t native_frequency)
{
    native_frequency_ = native_frequency;
    phase_inc_ = CalcPhaseIncrement(native_frequency_);
}

void MultiOscillator::SetPulseWidth(const q31_t pulse_width)
{
    if (pulse_width < Q31_ZERO)
    {
        pulse_width_ = Q31_ZERO; // Clamp to 0
    }
    else {
        pulse_width_ = (pulse_width - Q31_HALF) * 2; // Convert to 0-1 range
    }
}

q31_t MultiOscillator::GetPhase() const
{
    return phase_;
}

void MultiOscillator::PhaseAdd(const q31_t phase)
{
    // don't use q31_add to enable overflow
    phase_ = (int32_t)phase_ + (int32_t)phase;
}

void MultiOscillator::SetPhaseFeedback(const q31_t feedback)
{
    feedback_ = feedback;
}

void MultiOscillator::Reset(const q31_t _phase)
{
    phase_ = _phase;
}

FASTCODE MultiOscillator::Outputs MultiOscillator::Process()
{    
    if (feedback_ != Q31_ZERO)
    {
        outputs_.sine = q31_sine(((int32_t)phase_ + (int32_t)q31_mult(feedback_, outputs_.sine)) / 2 + Q31_HALF);
        // it really has to be done like this, adding the feedback to the actual phase made the output go silent for anything above 10% feedback
    }
    else
    {
        outputs_.sine = q31_sine(phase_ / 2 + Q31_HALF);
    }
    outputs_.square = phase_ < pulse_width_ ? Q31_MAX : Q31_MIN;
    outputs_.ramp = phase_;

    // don't use arm_add_q31 to enable overflow
    phase_ = (int32_t)phase_ + (int32_t)phase_inc_;

    return outputs_;
}

q31_t MultiOscillator::CalcPhaseIncrement(const q31_t frequency)
{

    int64_t inc = frequency << 1;
    if (inc > Q31_MAX)
    {
        inc = Q31_MAX;
    }
    return inc;
}

