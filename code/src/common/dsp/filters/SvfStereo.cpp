/*
MIT License

Copyright (c) 2020 Electrosmith, Corp
Copyright (c) 2024 Vaclav Mach (Bastl Instruments)
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

#include "SvfStereo.hpp"
#include <cmath>
#include "hardware/sync.h"
#include "common/dsp/math/math_utils.hpp"
#include <numbers>

using namespace kastle2;

void SvfStereo::Init(float sample_rate)
{
    sample_rate_ = sample_rate;
    max_frequency_ = sample_rate_ / 3.f;

    // Initialize states
    qinput_left_ = 0;
    qnotch_left_ = 0;
    qlow_left_ = 0;
    qhigh_left_ = 0;
    qband_left_ = 0;
    qout_raw_left_ = 0;
    qout_notch_left_ = 0;
    qout_low_left_ = 0;
    qout_high_left_ = 0;
    qout_band_left_ = 0;

    qinput_right_ = 0;
    qnotch_right_ = 0;
    qlow_right_ = 0;
    qhigh_right_ = 0;
    qband_right_ = 0;
    qout_raw_right_ = 0;
    qout_notch_right_ = 0;
    qout_low_right_ = 0;
    qout_high_right_ = 0;
    qout_band_right_ = 0;

    // Set default values
    SetResonance(0.5f);
    SetDrive(0.5f);
    SetFrequency(500.0f);
}

FASTCODE void SvfStereo::Process(q15_t input_left, q15_t input_right)
{
    // Some magic and guesswork so it fits into int_32t
    // Ideally we'd use int64_t for all the calculations but there is just not enough performace
    qout_raw_left_ = input_left;
    qout_raw_right_ = input_right;
    qinput_left_ = input_left >> kDownsample;
    qinput_right_ = input_right >> kDownsample;

    // First pass - left
    qnotch_left_ = qinput_left_ - mult(qdamp_, qband_left_);
    qlow_left_ = qlow_left_ + mult(qinternal_frequency_, qband_left_);
    qhigh_left_ = qnotch_left_ - qlow_left_;
    qband_left_ = mult(qinternal_frequency_, qhigh_left_) + qband_left_ - mult(qdrive_, mult(qband_left_, mult(qband_left_, qband_left_)));

    qnotch_left_ = q15_saturate(qnotch_left_);
    qlow_left_ = q15_saturate(qlow_left_);
    qhigh_left_ = q15_saturate(qhigh_left_);
    qband_left_ = q15_saturate(qband_left_);

    // First pass - right
    qnotch_right_ = qinput_right_ - mult(qdamp_, qband_right_);
    qlow_right_ = qlow_right_ + mult(qinternal_frequency_, qband_right_);
    qhigh_right_ = qnotch_right_ - qlow_right_;
    qband_right_ = mult(qinternal_frequency_, qhigh_right_) + qband_right_ - mult(qdrive_, mult(qband_right_, mult(qband_right_, qband_right_)));

    qnotch_right_ = q15_saturate(qnotch_right_);
    qlow_right_ = q15_saturate(qlow_right_);
    qhigh_right_ = q15_saturate(qhigh_right_);
    qband_right_ = q15_saturate(qband_right_);

    // Second pass - left
    qnotch_left_ = qinput_left_ - mult(qdamp_, qband_left_);
    qlow_left_ = qlow_left_ + mult(qinternal_frequency_, qband_left_);
    qhigh_left_ = qnotch_left_ - qlow_left_;
    qband_left_ = mult(qinternal_frequency_, qhigh_left_) + qband_left_ - mult(qdrive_, mult(qband_left_, mult(qband_left_, qband_left_)));

    qnotch_left_ = q15_saturate(qnotch_left_);
    qlow_left_ = q15_saturate(qlow_left_);
    qhigh_left_ = q15_saturate(qhigh_left_);
    qband_left_ = q15_saturate(qband_left_);

    // Second pass - right
    qnotch_right_ = qinput_right_ - mult(qdamp_, qband_right_);
    qlow_right_ = qlow_right_ + mult(qinternal_frequency_, qband_right_);
    qhigh_right_ = qnotch_right_ - qlow_right_;
    qband_right_ = mult(qinternal_frequency_, qhigh_right_) + qband_right_ - mult(qdrive_, mult(qband_right_, mult(qband_right_, qband_right_)));

    qnotch_right_ = q15_saturate(qnotch_right_);
    qlow_right_ = q15_saturate(qlow_right_);
    qhigh_right_ = q15_saturate(qhigh_right_);
    qband_right_ = q15_saturate(qband_right_);

    // Getting the results

    // Dividing by two and upscaling with kDownsample for output
    // It's done with (x << (kDownsample - 1)) instead of ((x / 2) << kDownsample)
    qout_notch_left_ = qnotch_left_ << (kDownsample - 1);
    qout_low_left_ = qlow_left_ << (kDownsample - 1);
    qout_high_left_ = qhigh_left_ << (kDownsample - 1);
    qout_band_left_ = qband_left_ << (kDownsample - 1);

    qout_notch_right_ = qnotch_right_ << (kDownsample - 1);
    qout_low_right_ = qlow_right_ << (kDownsample - 1);
    qout_high_right_ = qhigh_right_ << (kDownsample - 1);
    qout_band_right_ = qband_right_ << (kDownsample - 1);
}

q15_t SvfStereo::GetLeft() const
{
    switch (type_)
    {
    case Type::LOWPASS:
        return qout_low_left_;
    case Type::HIGHPASS:
        return qout_high_left_;
    case Type::BANDPASS:
        return qout_band_left_;
    case Type::NOTCH:
        return qout_notch_left_;
    case Type::BYPASS:
        return qout_raw_left_;
    default:
        return 0;
    }
}

q15_t SvfStereo::GetRight() const
{
    switch (type_)
    {
    case Type::LOWPASS:
        return qout_low_right_;
    case Type::HIGHPASS:
        return qout_high_right_;
    case Type::BANDPASS:
        return qout_band_right_;
    case Type::NOTCH:
        return qout_notch_right_;
    case Type::BYPASS:
        return qout_raw_right_;
    default:
        return 0;
    }
}

void SvfStereo::SetFrequency(float frequency)
{
    frequency = constrain(frequency, 1.0e-6, max_frequency_);

    // Set Internal Frequency for "frequency"
    internal_frequency_ = 2.0f * sinf(std::numbers::pi * std::min(0.25f, frequency / (sample_rate_ * 2.0f))); // fs*2 because double sampled
    tmp_qinternal_frequency_ = internal_frequency_ * 32768.0f;

    RecalculateDamp();
    FinishValueSetting();
}

void SvfStereo::SetResonance(float resonance, ForceValue force)
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

void SvfStereo::SetDrive(float drive)
{
    pre_drive_ = constrain(drive, 0.f, 1.f);
    RecalculateDrive();
    FinishValueSetting();
}

void SvfStereo::RecalculateDrive()
{
    float drive = pre_drive_ * resonance_;
    tmp_qdrive_ = drive * 32768.0f;
}

void SvfStereo::RecalculateDamp()
{
    float damp = std::min(2.0f * (1.0f - std::pow(resonance_, 0.25f)), std::min(2.0f, 2.0f / internal_frequency_ - internal_frequency_ * 0.5f));
    tmp_qdamp_ = damp * 32768.0f;
}

void SvfStereo::FinishValueSetting()
{
    uint32_t interrupts = save_and_disable_interrupts();

    qdrive_ = tmp_qdrive_;
    qdamp_ = tmp_qdamp_;
    qinternal_frequency_ = tmp_qinternal_frequency_;

    restore_interrupts(interrupts);
}

void SvfStereo::SetType(Type type)
{
    type_ = type;
}

SvfStereo::Type SvfStereo::GetType() const
{
    return type_;
}