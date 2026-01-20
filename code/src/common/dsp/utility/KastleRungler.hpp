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

#pragma once

#include <cmath>
#include <cstdint>
#include "common/dsp/math/bit_utils.hpp"

namespace kastle2
{

/**
 * @class KastleRungler
 * @ingroup dsp_utility
 * @brief Pseudo-random voltage generator.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-05-31
 *
 * This generator is inspired by the Rungler circuit by Rob Hordijk and
 * based on the code by Vaclav Pelousek for the original Bastl Kastle.
 *
 * It can produce 8 different voltages (see `kVoltageMap`).
 * Each time the Step() is called the 8-bit shift register is shifted and a new bit is generated.
 *
 * Based on the `bit_in` input signal the new bit:
 * - remains the same
 * - is inverted
 * - is generated randomly
 *
 * The output voltage is based on the current state of 3 bits of internal binary 8-bit shift register.
 * By default it generates voltages in the range 0-255, but it can be shifted eg. to 0-1023 range.
 *
 *
 */
class KastleRungler
{
public:
    enum BitIn
    {
        SAME,
        INVERT,
        RANDOM
    };
    /**
     * @brief Initializes the Rungler with random shift register values.
     * @param shift_result How to shift the result (use `2` for 0-1023 range)
     */
    void Init(int8_t shift_result = kDefaultShiftResult);

    /**
     * @brief Fake reset, Steps() the register with Same to "start from beginning".
     */
    void Reset();

    /**
     * @brief Returns the current output value.
     * @return The current output value.
     */
    uint32_t GetOutput();

    /**
     * @brief Generates the next output value.
     *
     * Based on the `bit_in` input signal the new bit:
     * - remains the same
     * - is inverted
     * - is generated randomly
     *
     * @param bit_in The input signal to generate the new bit.
     * @return The new output value.
     */
    uint32_t Step(BitIn bit_in);

private:
    uint32_t output_ = 0;
    uint8_t shift_register_ = 0;
    int8_t shift_result_ = 0;
    static constexpr uint8_t kVoltageMap[8] = {0, 80, 120, 150, 180, 200, 220, 255};
    static constexpr int8_t kDefaultShiftResult = 2; // 256 => 1024

    size_t steps_ = 0;
    static constexpr size_t kLength = 8;
};
}
