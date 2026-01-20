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

#include "Svf.hpp"
#include <cmath>
#include <numbers>
#include "hardware/sync.h"
#include "common/dsp/math/math_utils.hpp"

using namespace kastle2;

void Svf::Init(float sample_rate)
{
    sample_rate_ = sample_rate;
    max_frequency_ = sample_rate_ / 3.f;

    // Initialize states
    qinput_ = 0;
    qnotch_ = 0;
    qlow_ = 0;
    qhigh_ = 0;
    qband_ = 0;
    qout_notch_ = 0;
    qout_low_ = 0;
    qout_high_ = 0;
    qout_band_ = 0;

    // Set default values
    SetResonance(0.5f);
    SetDrive(0.5f);
    SetFrequency(500.0f);
}

FASTCODE q15_t Svf::Process(q15_t in)
{
    // Some magic and guesswork so it fits into int_32t
    // Ideally we'd use int64_t for all the calculations but there is just not enough performace
    qinput_ = in >> kDownsample;

    // First pass
    qnotch_ = qinput_ - mult(qdamp_, qband_);
    qlow_ = qlow_ + mult(qinternal_frequency_, qband_);
    qhigh_ = qnotch_ - qlow_;
    qband_ = mult(qinternal_frequency_, qhigh_) + qband_ - mult(qdrive_, mult(qband_, mult(qband_, qband_)));

    qnotch_ = q15_saturate(qnotch_);
    qlow_ = q15_saturate(qlow_);
    qhigh_ = q15_saturate(qhigh_);
    qband_ = q15_saturate(qband_);

    // Second pass
    qnotch_ = qinput_ - mult(qdamp_, qband_);
    qlow_ = qlow_ + mult(qinternal_frequency_, qband_);
    qhigh_ = qnotch_ - qlow_;
    qband_ = mult(qinternal_frequency_, qhigh_) + qband_ - mult(qdrive_, mult(qband_, mult(qband_, qband_)));

    qnotch_ = q15_saturate(qnotch_);
    qlow_ = q15_saturate(qlow_);
    qhigh_ = q15_saturate(qhigh_);
    qband_ = q15_saturate(qband_);

    // Getting the results

    // Dividing by two and upscaling with kDownsample for output
    // It's done with (x << (kDownsample - 1)) instead of ((x / 2) << kDownsample)
    qout_notch_ = qnotch_ << (kDownsample - 1);
    qout_low_ = qlow_ << (kDownsample - 1);
    qout_high_ = qhigh_ << (kDownsample - 1);
    qout_band_ = qband_ << (kDownsample - 1);

    switch (type_)
    {
    case Type::LOWPASS:
        return qout_low_;
    case Type::HIGHPASS:
        return qout_high_;
    case Type::BANDPASS:
        return qout_band_;
    case Type::NOTCH:
        return qout_notch_;
    case Type::BYPASS:
        return qinput_;
    default:
        return 0;
    }
}

void Svf::SetFrequency(float frequency)
{
    frequency = constrain(frequency, 1.0e-6, max_frequency_);

    // Set Internal Frequency for "frequency"
    internal_frequency_ = 2.0f * sinf(std::numbers::pi * std::min(0.25f, frequency / (sample_rate_ * 2.0f))); // fs*2 because double sampled
    tmp_qinternal_frequency_ = internal_frequency_ * 32768.0f;

    RecalculateDamp();
    FinishValueSetting();
}

void Svf::SetResonance(float resonance, ForceValue force)
{
    if (force == ForceValue::TRUE)
    {
        resonance_ = constrain(resonance, 0.0f, 1.f);
    }
    else
    {
        resonance_ = constrain(resonance, 0.005f, 1.f);
    }
    RecalculateDamp();
    RecalculateDrive();
    FinishValueSetting();
}

void Svf::SetDrive(float drive)
{
    pre_drive_ = constrain(drive, 0.f, 1.f);
    RecalculateDrive();
    FinishValueSetting();
}

void Svf::RecalculateDrive()
{
    float drive = pre_drive_ * resonance_;
    tmp_qdrive_ = drive * 32768.0f;
}

void Svf::RecalculateDamp()
{
    float damp = std::min(2.0f * (1.0f - std::pow(resonance_, 0.25f)), std::min(2.0f, 2.0f / internal_frequency_ - internal_frequency_ * 0.5f));
    tmp_qdamp_ = damp * 32768.0f;
}

void Svf::FinishValueSetting()
{
    uint32_t interrupts = save_and_disable_interrupts();

    qdrive_ = tmp_qdrive_;
    qdamp_ = tmp_qdamp_;
    qinternal_frequency_ = tmp_qinternal_frequency_;

    restore_interrupts(interrupts);
}

q15_t Svf::GetLowPassOutput() const
{
    return qout_low_;
}

q15_t Svf::GetHighPassOutput() const
{
    return qout_high_;
}

q15_t Svf::GetBandPassOutput() const
{
    return qout_band_;
}

q15_t Svf::GetNotchOutput() const
{
    return qout_notch_;
}

void Svf::SetType(Type type)
{
    type_ = type;
}

Svf::Type Svf::GetType() const
{
    return type_;
}