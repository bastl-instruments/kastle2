/*
MIT License

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

#include "SoftClipper.hpp"
#include "common/utils.hpp"
#include "lookup_tanh.hpp"

using namespace kastle2;

void SoftClipper::Init(float sample_rate)
{
    sample_rate_ = sample_rate;
    compensation_ = true;
    drive_ = 0;
}

FASTCODE q15_t SoftClipper::Process(q15_t input)
{
    int32_t val = q15_mult(input, 10431); // divide by pi (magic number = 1 / pi)

    // Should be 15, it's 9 to get some drive
    val = val + ((val * drive_) >> 9);
    bool negative = val < Q15_ZERO;

    // Clip result and make all positive
    q15_t output = q15_saturate(q31_abs(val));

    // Use tanh look up table
    output = tanh_lut[output];

    // return signedness
    if (negative)
        output = -output;

    // The more drive, the less volume
    if (compensation_)
    {
        output = q15_mult(output, Q15_MAX - (drive_ >> 1));
    }

    // Return value
    return output;
}

void SoftClipper::DisableVolumeCompensation(bool disable)
{
    compensation_ = !disable;
}

void SoftClipper::SetDrive(q15_t drive)
{
    drive_ = drive;
}

q15_t SoftClipper::GetDrive() const
{
    return drive_;
}
