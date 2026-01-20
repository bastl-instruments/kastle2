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

#include "HardClipper.hpp"

using namespace kastle2;

void HardClipper::Init(float sample_rate)
{
    sample_rate_ = sample_rate;
    filter_.Init(sample_rate);
    filter_.SetType(Svf::Type::LOWPASS);
    filter_.SetFrequency(200.0f);
    filter_.SetResonance(0.9f);
}

FASTCODE q15_t HardClipper::Process(q15_t input)
{
    int32_t val = input;

    // Should be 15, it's 10 to get some drive
    val = val + ((val * drive_) >> 10);

    // Clip result
    q15_t output = q15_saturate(val);

    // The more drive, the less volume
    if (compensation_)
    {
        output = q15_mult(output, Q15_MAX - (drive_ >> 1));
    }

    // Bass boost
  //  q15_t filtered = filter_.Process(output);
   // output = q15_add(output, q15_mult(filtered, drive_));

    // Return value
    return output;
}

void HardClipper::DisableVolumeCompensation(bool disable)
{
    compensation_ = !disable;
}

void HardClipper::SetDrive(q15_t drive)
{
    drive_ = drive;
}

q15_t HardClipper::GetDrive() const
{
    return drive_;
}