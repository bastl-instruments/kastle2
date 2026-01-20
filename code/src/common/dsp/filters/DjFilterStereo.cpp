/*
MIT License

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

#include "DjFilterStereo.hpp"

#include "common/dsp/math/math_utils.hpp"

using namespace kastle2;

void DjFilterStereo::Init(float sample_rate)
{
    // Initialize the lowpass and highpass filters with the given sample rate
    lowpass_.Init(sample_rate);
    lowpass_.SetType(SvfStereo::Type::LOWPASS);
    lowpass_.SetFrequency(12000.0);
    lowpass_.SetResonance(0.5f);

    highpass_.Init(sample_rate);
    highpass_.SetType(SvfStereo::Type::HIGHPASS);
    highpass_.SetFrequency(12000.0);
    highpass_.SetResonance(0.5f);

    SetCrossfade(Q15_ZERO);
}

FASTCODE void DjFilterStereo::Process(q15_t input_left, q15_t input_right)
{
    q15_t in_left = input_left / 2; // Lowering the input volume to match the SVF
    q15_t in_right = input_right / 2;
    highpass_.Process(input_left, input_right);
    lowpass_.Process(input_left, input_right);

    q15_t hp_left = highpass_.GetLeft();
    q15_t hp_right = highpass_.GetRight();
    q15_t lp_left = lowpass_.GetLeft();
    q15_t lp_right = lowpass_.GetRight();

    if (zone_ != prev_zone_)
    {
        // A zone change has been detected. Start a new crossfade.
        crossfade_index_ = 0;
        crossfade_from_ = prev_zone_;
    }
    prev_zone_ = zone_;

    if (crossfade_index_ < kCrossfadeLength)
    {
        // using int32_t to avoid overflows
        q15_t new_signal_left = 0;
        q15_t new_signal_right = 0;
        q15_t old_signal_left = 0;
        q15_t old_signal_right = 0;
        switch (zone_)
        {
        case Zone::HIGHPASS:
            new_signal_left = hp_left;
            new_signal_right = hp_right;
            break;
        case Zone::LOWPASS:
            new_signal_left = lp_left;
            new_signal_right = lp_right;
            break;
        case Zone::NONE:
            new_signal_left = in_left;
            new_signal_right = in_right;
            break;
        }
        switch (crossfade_from_)
        {
        case Zone::HIGHPASS:
            old_signal_left = hp_left;
            old_signal_right = hp_right;
            break;
        case Zone::LOWPASS:
            old_signal_left = lp_left;
            old_signal_right = lp_right;
            break;
        case Zone::NONE:
            old_signal_left = in_left;
            old_signal_right = in_right;
            break;
        }

        // We're in the middle of a crossfade. Blend the outputs of the two filters.
        q15_t blend_factor = fraction_to_q15(crossfade_index_, kCrossfadeLength);
        q15_t inv_blend_factor = q15_inv(blend_factor);
        output_left_ = q15_mult_fast(new_signal_left, blend_factor) + q15_mult_fast(old_signal_left, inv_blend_factor);
        output_right_ = q15_mult_fast(new_signal_right, blend_factor) + q15_mult_fast(old_signal_right, inv_blend_factor);
        crossfade_index_++;
    }
    else
    {
        // No crossfade in progress. Output the result of the current zone's filter.
        if (zone_ == Zone::HIGHPASS)
        {
            output_left_ = hp_left;
            output_right_ = hp_right;
        }
        else if (zone_ == Zone::LOWPASS) // zone_ == LOWPASS
        {
            output_left_ = lp_left;
            output_right_ = lp_right;
        }
        else // zone_ == NONE
        {
            output_left_ = in_left;
            output_right_ = in_right;
        }
    }
}

q15_t DjFilterStereo::GetLeft()
{
    return output_left_;
}

q15_t DjFilterStereo::GetRight()
{
    return output_right_;
}

void DjFilterStereo::SetCrossfade(q15_t crossfade)
{
    // Keep some thresholds for the center deadzone
    if (crossfade < (-kCenterDeadzone - kCenterDeadzoneThreshold))
    {
        zone_ = Zone::LOWPASS;
    }

    if (
        crossfade > (-kCenterDeadzone + kCenterDeadzoneThreshold) &&
        crossfade < (kCenterDeadzone - kCenterDeadzoneThreshold))
    {
        zone_ = Zone::NONE;
    }

    if (crossfade > (kCenterDeadzone + kCenterDeadzoneThreshold))
    {
        zone_ = Zone::HIGHPASS;
    }

    q15_t norm;

    switch (zone_)
    {
    case Zone::LOWPASS:
        norm = map(crossfade, Q15_MIN, -kCenterDeadzone, Q15_ZERO, Q15_MAX);
        lowpass_.SetFrequency(curve_map(norm, kLowpassMap));
        break;
    case Zone::HIGHPASS:
        norm = map(crossfade, kCenterDeadzone, Q15_MAX, Q15_ZERO, Q15_MAX);
        highpass_.SetFrequency(curve_map(norm, kHighpassMap));
        break;
    case Zone::NONE:
        break;
    }
}

void DjFilterStereo::SetResonance(float resonance, SvfStereo::ForceValue force)
{
    lowpass_.SetResonance(resonance, force);
    highpass_.SetResonance(resonance, force);
}