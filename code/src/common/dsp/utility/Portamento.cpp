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

#include "Portamento.hpp"

using namespace kastle2;

void Portamento::Init(float update_rate)
{
    speed_ = 0.f;
    current_value_ = 0.f;
    new_value_ = 0.f;
    timed_loop_calls_ = 0;
    update_rate_ = update_rate;
}

float Portamento::Track(float in)
{
    new_value_ = in;

    while (timed_loop_calls_ > 0)
    {
        current_value_ = new_value_ + (speed_ * (current_value_ - new_value_));
        timed_loop_calls_--;
    }

    return current_value_;
}

float Portamento::GetOut() const
{
    return current_value_;
}

void Portamento::SetSpeed(float speed)
{
    speed_ = std::pow(0.01f, 1.0f / (update_rate_ * speed));
    // This line is different from the one in the envelope follower, it's taken from musicdsp.org
    // where the author was saying it is a simplified equivalent of the original calculation. It
    // most definitely isn't tho, with the result being wildly different. There is an additional
    // comment saying it is in fact not the same, and that it calculates from 100% to 1% instead
    // of 100% to 36.7% that the original does. I can't really make much sense of it honestly but
    // this calculation seems to be actually correct unlike the original one.
    // https://www.musicdsp.org/en/latest/Analysis/136-envelope-follower-with-different-attack-and-release.html
}

void Portamento::TimeTick()
{
    timed_loop_calls_++;
}
