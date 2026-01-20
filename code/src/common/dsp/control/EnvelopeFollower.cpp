/*
MIT License

Copyright (c) 2016 Lennart Schierling (Bastl Instruments)
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

#include "EnvelopeFollower.hpp"

using namespace kastle2;

void EnvelopeFollower::Init(float update_rate)
{
    attack_ = Q15_ZERO;
    release_ = Q15_MAX;
    current_value_ = Q15_ZERO;
    new_value_ = Q15_ZERO;
    update_rate_ = update_rate;
}

FASTCODE void EnvelopeFollower::Track(q15_t in)
{
    new_value_ = q15_abs(in);

    // smooth envelope with attack and release
    if (new_value_ > current_value_)
    {
        current_value_ = q15_add(new_value_, q15_mult(attack_, q15_sub(current_value_, new_value_)));
    }
    else
    {
        current_value_ = q15_add(new_value_, q15_mult(release_, q15_sub(current_value_, new_value_)));
    }
}

FASTCODE void EnvelopeFollower::UpdatePeak(q15_t in)
{
    new_value_ = q15_abs(in);
}

FASTCODE q15_t EnvelopeFollower::CalculateEnvelope()
{
    // smooth envelope with attack and release
    if (new_value_ > current_value_)
    {
        current_value_ = q15_add(new_value_, q15_mult(attack_, q15_sub(current_value_, new_value_)));
    }
    else
    {
        current_value_ = q15_add(new_value_, q15_mult(release_, q15_sub(current_value_, new_value_)));
    }

    return current_value_;
}

FASTCODE void EnvelopeFollower::TrackBlock(q15_t *buffer, size_t block_size)
{
    q15_t max = 0;

    // find maximum value
    for (size_t i = 0; i < block_size; i++)
    {
        q15_t a = q15_abs(buffer[i]);
        if (a > max)
        {
            max = a;
        }
    }

    Track(max);
}

FASTCODE q15_t EnvelopeFollower::GetEnvelope() const
{
    return current_value_;
}

void EnvelopeFollower::SetAttackTime(float attack_in_seconds)
{
    attack_ = float_to_q15(expf(-1.0f / (update_rate_ * attack_in_seconds)));
}

void EnvelopeFollower::SetReleaseTime(float release_in_seconds)
{
    release_ = float_to_q15(expf(-1.0f / (update_rate_ * release_in_seconds)));
}
