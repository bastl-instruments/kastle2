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

#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/math/qmath.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class BitCrusher
 * @ingroup dsp_effects
 * @brief Bit crusher effect
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-05-31
 *
 * Reduces the bit depth of the input signal and downsamples it.
 */
class BitCrusher
{
public:
    void Init()
    {
        SetBitDepth(8);
        SetSampleRate(Q15_HALF);
        counter_ = 0;
        last_sample_ = 0;
        next_sample_ = 0;
    }

    /**
     * @brief Set bit depth of the effect
     * @param bit_depth Bit depth (1 - 16)
     */
    void SetBitDepth(uint32_t bit_depth)
    {
        bit_depth = constrain(bit_depth, kMinBitDepth, kMaxBitDepth);
        shift_ = kMaxBitDepth - bit_depth;
    }

    /**
     * @brief Sample rate 1 (Q31_MAX) means no downsampling
     * @param sample_rate Sample rate in Q15 format (0 - Q31_MAX)
     */
    void SetSampleRate(q15_t sample_rate)
    {
        sample_rate = constrain(sample_rate, kMinSampleRate, kMaxSampleRate);
        sample_rate_ = sample_rate;
    }

    /**
     * @brief Applies bit crushing and downsampling to the sample
     * @param sample Nice and clean sample
     * @return Crushed sample
     */
    FASTCODE q15_t Process(q15_t sample)
    {
        counter_ += sample_rate_;
        if (counter_ >= Q15_MAX)
        {
            counter_ -= Q15_MAX;

            int32_t shift_up = shift_;

            // Loudness compensation, so it doesn't blow up the volume too much when in lowest bit depths
            if (shift_ > 12)
            {
                shift_up -= 1;
            }
            if (shift_ > 13)
            {
                shift_up -= 1;
            }

            last_sample_ = next_sample_;
            next_sample_ = (sample >> shift_) << shift_up;
        }

        // Linear interpolation between last_sample_ and next_sample_
        return q15_add(last_sample_, q15_mult((next_sample_ - last_sample_), counter_));
    }

    static constexpr uint32_t kMinBitDepth = 1;
    static constexpr uint32_t kMaxBitDepth = 16;

    static constexpr q15_t kMaxSampleRate = Q15_MAX;      // 44kHz, no downsampling
    static constexpr q15_t kMinSampleRate = Q15_MAX / 64; // Approx 687.5 Hz for 44kHz sample rate

private:
    int32_t shift_ = 0;
    q31_t sample_rate_ = 0;
    int32_t counter_ = 0;
    q15_t last_sample_ = 0;
    q15_t next_sample_ = 0;
};
}
