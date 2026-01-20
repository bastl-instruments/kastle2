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

#include "KastleRungler.hpp"

using namespace kastle2;

void KastleRungler::Init(int8_t shift_result)
{
    shift_register_ = rand() & 0xFF;
    shift_result_ = shift_result;
    steps_ = 0;
}

uint32_t KastleRungler::GetOutput()
{
    return output_;
}

void KastleRungler::Reset()
{
    // Shift the register to the right by the number of steps to "start from beginning"
    uint8_t how_many_shift = kLength - 1 - (steps_ % kLength);
    for (size_t i = 0; i < how_many_shift; i++)
    {
        Step(SAME);
    }
    steps_ = kLength - 1; // 0 doesn't work here, because we are increasing it in Step() right away
}

uint32_t KastleRungler::Step(BitIn bit_in)
{
    // Increase step counter
    steps_++;

    // Read last bit
    bool new_bit = bit_read(shift_register_, 7);

    // Shift left
    shift_register_ = shift_register_ << 1;

    // Write new bit
    switch (bit_in)
    {
    case INVERT:
        new_bit = !new_bit;
        break;
    case RANDOM:
        new_bit = rand() & 1;
        break;
    }
    bit_write(shift_register_, 0, new_bit);

    // Make output index
    uint8_t map_index = 0;
    bit_write(map_index, 0, bit_read(shift_register_, 0));
    bit_write(map_index, 1, bit_read(shift_register_, 3));
    bit_write(map_index, 2, bit_read(shift_register_, 5));

    // Use the value from PWM map as the result
    output_ = kVoltageMap[map_index] << shift_result_;
    return output_;
}