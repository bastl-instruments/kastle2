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

#include "Fm2.hpp"

using namespace kastle2;

void Fm2::Init(const float sample_rate)
{
    sample_rate_ = sample_rate;

    // Init oscillators
    car_.Init(sample_rate_);
    mod_.Init(sample_rate_);

    // Set some reasonable values
    SetFrequency(440.0f);
    SetIndex(Q31_HALF);
    SetRatio(Q31_HALF);

    car_.SetWaveform(Oscillator::Waveform::SINE);
    mod_.SetWaveform(Oscillator::Waveform::SINE);
}

q31_t Fm2::Process()
{
    if (prev_ratio_ != ratio_ || prev_freq_ != freq_)
    {
        prev_ratio_ = ratio_;
        prev_freq_ = freq_;
        car_.SetNativeFrequency(prev_freq_);
        mod_.SetNativeFrequency(q31_mult(prev_freq_, prev_ratio_));
    }

    q31_t modval = mod_.Process();
    modval = q31_mult(modval, index_);
    car_.PhaseAdd(modval);
    return car_.Process();
}

void Fm2::SetFrequency(const float frequency)
{
    SetNativeFrequency(freq_to_q31(frequency, sample_rate_));
}

void Fm2::SetNativeFrequency(const q31_t freq)
{
    freq_ = freq;
}

void Fm2::SetRatio(const q31_t ratio)
{
    ratio_ = ratio;
}

void Fm2::SetIndex(const q31_t index)
{
    index_ = q31_mult(index, kIndexScalar);
}

q31_t Fm2::GetIndex() const
{
    return q31_div(index_, kIndexScalar);
}

void Fm2::Reset()
{
    car_.Reset();
    mod_.Reset();
}
