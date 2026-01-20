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

#include "StereoDelay.hpp"
#include "common/dsp/math/math_utils.hpp"

using namespace kastle2;

void StereoDelay::Init(float sample_rate)
{
    sample_rate_ = sample_rate;
    filter_enabled_ = false;
    filter_l_.Init(sample_rate);
    filter_r_.Init(sample_rate);

    delay_l_ = std::make_unique<AdvancedDynamicDelayLine<q15least_t>>(max_delay_);
    delay_r_ = std::make_unique<AdvancedDynamicDelayLine<q15least_t>>(max_delay_);

    SetDelay(max_delay_ / 2, max_delay_ / 2);
    SetFeedback(q15(0.5f));
    SetWet(q15(0.5f));
}

void StereoDelay::SetFilterEnabled(bool filter_enabled)
{
    filter_enabled_ = filter_enabled;
}

FASTCODE StereoDelay::Output StereoDelay::Process(q15_t left, q15_t right)
{
    // read delay
    q15_t delayed_l = delay_l_->Read();
    q15_t delayed_r = delay_r_->Read();

    // write again
    delay_l_->Write(q15_add(q15_mult(delayed_l, feedback_l_), left));
    delay_r_->Write(q15_add(q15_mult(delayed_r, feedback_r_), right));

    // apply filter
    if (filter_enabled_)
    {
        delayed_l = filter_l_.Process(delayed_l);
        delayed_r = filter_r_.Process(delayed_r);
    }

    q15_t dry = q15_inv(wet_);

    // apply dry/wet
    out_.left = q15_add(q15_mult(left, dry), q15_mult(delayed_l, wet_));
    out_.right = q15_add(q15_mult(right, dry), q15_mult(delayed_r, wet_));
    return out_;
}

StereoDelay::Output StereoDelay::GetOutput() const
{
    return out_;
}

void StereoDelay::SetDelay(size_t delay_left, size_t delay_right)
{
    delay_l_->SetDelay(delay_left);
    delay_r_->SetDelay(delay_right);
}

void StereoDelay::SetFeedback(q15_t feedback)
{
    SetFeedback(feedback, feedback);
}

void StereoDelay::SetFeedback(q15_t feedback_left, q15_t feedback_right)
{
    feedback_l_ = feedback_left;
    feedback_r_ = feedback_right;
}

void StereoDelay::SetWet(q15_t wet)
{
    wet_ = wet;
}

void StereoDelay::SetFilterResonance(float resonance)
{
    filter_l_.SetResonance(resonance);
    filter_r_.SetResonance(resonance);
}

void StereoDelay::SetFilterCrossfade(q15_t crossfade)
{
    filter_l_.SetCrossfade(crossfade);
    filter_r_.SetCrossfade(crossfade);
}
