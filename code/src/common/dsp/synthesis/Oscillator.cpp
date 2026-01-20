/*
MIT License

Copyright (c) 2020 Electrosmith, Corp
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

#include "Oscillator.hpp"
#include "common/dsp/math/math_utils.hpp"

using namespace kastle2;

void Oscillator::Init(const float sample_rate)
{
    sample_rate_ = sample_rate;
    amplitude_ = Q31_MAX;
    phase_ = Q31_MIN;
    waveform_ = Waveform::SINE;
    pulse_width_ = Q31_ZERO; // 50% duty cycle
    eoc_ = true;
    eor_ = true;
    SetFrequency(100); // 100 Hz
}

void Oscillator::SetFrequency(const float frequency)
{
    SetNativeFrequency(freq_to_q31(frequency, sample_rate_));
}

void Oscillator::SetNativeFrequency(const q31_t native_frequency)
{
    native_frequency_ = native_frequency;
    phase_inc_ = CalcPhaseIncrement(native_frequency_);
    ticks_ = Q31_MAX / native_frequency_;
}

void Oscillator::SetTicks(const uint32_t ticks)
{
    uint32_t t = ticks;
    if (t == 0)
    {
        t = 1;
    }
    phase_inc_ = q31_div(Q31_MAX, t / 2); // similar to CalcPhaseIncrement()
}

uint32_t Oscillator::GetTicks() const
{
    return ticks_;
}

uint32_t Oscillator::GetElapsedTicks() const
{
    return ticks_counter_;
}

void Oscillator::SetAmplitude(const q31_t amplitude)
{
    amplitude_ = amplitude;
}

void Oscillator::SetWaveform(const Waveform waveform)
{
    waveform_ = waveform;
}

void Oscillator::SetPulseWidth(const q31_t pulse_width)
{
    pulse_width_ = pulse_width;
}

bool Oscillator::IsEOR() const
{
    return eor_;
}

bool Oscillator::IsEOC() const
{
    return eoc_;
}

bool Oscillator::IsRising() const
{
    return phase_ < Q31_ZERO;
}

bool Oscillator::IsFalling() const
{
    return phase_ >= Q31_ZERO;
}

q31_t Oscillator::GetPhase() const
{
    return phase_;
}

void Oscillator::PhaseAdd(const q31_t phase)
{
    // don't use q31_add to enable overflow
    phase_ = (int32_t)phase_ + (int32_t)phase;
}

void Oscillator::Reset(const q31_t _phase)
{
    phase_ = _phase;
}

FASTCODE q31_t Oscillator::Process()
{
    switch (waveform_)
    {
    case Waveform::SINE:
        // convert -1:1 range to 0:1 and apply fast sine method
        out_ = q31_sine(phase_ / 2 + Q31_HALF);
        break;
    case Waveform::TRI:
        if (phase_ < Q31_ZERO)
        {
            out_ = (phase_ + Q31_HALF) * 2;
        }
        else
        {
            out_ = (Q31_HALF - phase_) * 2;
        }
        break;
    case Waveform::SAW:
        out_ = -phase_;
        break;
    case Waveform::RAMP:
        out_ = phase_;
        break;
    case Waveform::SQUARE:
        out_ = phase_ < pulse_width_ ? Q31_MAX : Q31_MIN;
        break;
    default:
        out_ = Q31_ZERO;
        break;
    }

    q31_t new_phase;

    // don't use arm_add_q31 to enable overflow
    new_phase = (int32_t)phase_ + (int32_t)phase_inc_;
    // update the ticks counter as well
    ticks_counter_++;

    // overflow
    if (phase_ > new_phase)
    {
        eoc_ = true;
        ticks_counter_ = 0;
    }
    else
    {
        eoc_ = false;
    }

    // output amplitude
    if (amplitude_ != Q31_MAX)
    {
        out_ = q31_mult(out_, amplitude_);
    }

    // end of rise, complete phase
    eor_ = (phase_ < Q31_ZERO && new_phase >= Q31_ZERO);
    phase_ = new_phase;

    return out_;
}

q31_t Oscillator::GetOutput() const
{
    return out_;
}

/**
 * @brief Calculates the phase increment for a given relative frequency. Basically multiply by 2, since the oscillators use -1 to 1 range.
 * @param frequency The frequency to calculate the phase increment for.
 * @return The phase increment for the given frequency.
 */
q31_t Oscillator::CalcPhaseIncrement(const q31_t frequency)
{

    int64_t inc = frequency << 1;
    if (inc > Q31_MAX)
    {
        inc = Q31_MAX;
    }
    return inc;
}