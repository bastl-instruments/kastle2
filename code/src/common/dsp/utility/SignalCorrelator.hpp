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

#pragma once

#include <cstdint>
#include <cstring>
#include <array>
#include <algorithm>
#include <memory>
#include "common/dsp/math/qmath.hpp"

namespace kastle2
{
/**
 * @class SignalCorrelator
 * @ingroup controls
 * @brief A utility class used to detect if two signals are nearly identical. Used for self-patch detection and similar tasks.
 * @author Marek Mach (Bastl Instruments)
 * @date 2025-09-25
 */
class SignalCorrelator
{
public:

    enum class Expand
    {
        TRUE,
        FALSE
    };

    /**
     * @brief Constructor for SignalCorrelator.
     * @param size The size of the internal buffer.
     */
    SignalCorrelator(uint32_t size) : size_(size), 
                                      buffer1_(std::make_unique<int16_t[]>(size)),
                                      buffer2_(std::make_unique<int16_t[]>(size))
    {
        std::fill_n(buffer1_.get(), size_, 0);
        std::fill_n(buffer2_.get(), size_, 0);
    }

    /**
     * @brief Adds a new sample to the correlator.
     * @param sample1 The first signal sample.
     * @param sample2 The second signal sample.
     */
    void AddSample(int16_t sample1, int16_t sample2);
    
    /**
     * @brief Finds if the two signals are correlated within a given threshold.
     * @param threshold The threshold for correlation. A value from 0 to 1 in q15_t format.
     * @param expand If true, scales the signals to their maximum before correlation check. Used to avoid issues with long term amplitude changes.
     * @return True if the signals are correlated, false otherwise.
     */
    bool FindCorrelation(int16_t threshold, Expand expand = Expand::FALSE);

private:
    uint32_t size_;
    std::unique_ptr<int16_t[]> buffer1_;
    std::unique_ptr<int16_t[]> buffer2_;
    int32_t arr_index_ = 0;
    int32_t arr_index_margin_ = 3; // margin around the write index to ignore in correlation
};

} // namespace kastle2
