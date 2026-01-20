/*
MIT License

Copyright (c) 2020 Electrosmith, Corp
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

#pragma once

#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

/**
 * @class WhiteNoise
 * @ingroup dsp_synthesis
 * @brief White noise generator with custom seed, default seed is 1.
 * @author Marek Mach (Bastl Instruments)
 * @date 2025-08-25
 *
 * Based on the DaisySP White Noise, Copyright (c) 2020 Electrosmith, Corp
 */
class WhiteNoise
{
public:
    /**
     * @brief Sets the seed for the white noise generator.
     * @param seed The seed value to initialize the random number generator.
     */
    void Seed(const uint32_t seed)
    {
        rand_seed_ = seed;
        if (rand_seed_ == 0)
        {
            rand_seed_ = 1; // cannot be zero
        }
    }

    /**
     * @brief Processes and generates the next white noise sample.
     * @return The generated white noise sample as a q15_t value.
     */
    inline q15_t Process()
    {
        rand_seed_ *= 16807;
        return q31_to_q15(rand_seed_);
    }

private:
    uint32_t rand_seed_ = 1;
};

} // namespace kastle2
