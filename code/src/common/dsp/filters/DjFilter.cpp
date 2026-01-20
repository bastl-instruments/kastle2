/*
MIT License

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

#include "DjFilter.hpp"

#include "common/dsp/math/math_utils.hpp"

using namespace kastle2;

void DjFilter::Init(float sample_rate)
{
    // Initialize the lowpass and highpass filters with the given sample rate
    lowpass_.Init(sample_rate);
    lowpass_.SetType(Svf::Type::LOWPASS);
    lowpass_.SetFrequency(12000.0);
    lowpass_.SetResonance(0);

    highpass_.Init(sample_rate);
    highpass_.SetType(Svf::Type::HIGHPASS);
    highpass_.SetFrequency(12000.0);
    highpass_.SetResonance(0);

    SetCrossfade(Q15_ZERO);
}

q15_t DjFilter::Process(q15_t input)
{
    q15_t in = input / 2; // Lowering the input volume to match the SVF
    q15_t output = 0;
    q15_t hp = highpass_.Process(input);
    q15_t lp = lowpass_.Process(input);

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
        q15_t new_signal = 0;
        q15_t old_signal = 0;
        switch (zone_)
        {
        case Zone::HIGHPASS:
            new_signal = hp;
            break;
        case Zone::LOWPASS:
            new_signal = lp;
            break;
        case Zone::NONE:
            new_signal = in;
            break;
        }
        switch (crossfade_from_)
        {
        case Zone::HIGHPASS:
            old_signal = hp;
            break;
        case Zone::LOWPASS:
            old_signal = lp;
            break;
        case Zone::NONE:
            old_signal = in;
            break;
        }

        // We're in the middle of a crossfade. Blend the outputs of the two filters.
        q15_t blend_factor = fraction_to_q15(crossfade_index_, kCrossfadeLength);
        output = q15_add(q15_mult(new_signal, blend_factor), q15_mult(old_signal, q15_inv(blend_factor)));
        crossfade_index_++;
    }
    else
    {
        // No crossfade in progress. Output the result of the current zone's filter.
        if (zone_ == Zone::HIGHPASS)
        {
            output = hp;
        }
        else if (zone_ == Zone::LOWPASS) // zone_ == LOWPASS
        {
            output = lp;
        }
        else // zone_ == NONE
        {
            output = in;
        }
    }

    return output;
}

void DjFilter::SetCrossfade(q15_t crossfade)
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

void DjFilter::SetResonance(float resonance, Svf::ForceValue force)
{
    lowpass_.SetResonance(resonance, force);
    highpass_.SetResonance(resonance, force);
}