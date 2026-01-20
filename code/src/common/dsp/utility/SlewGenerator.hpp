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
#include <cstddef>
#include <cstdint>
#include "common/dsp/math/qmath.hpp"
#include "lookup_SlewGenerator.hpp"

namespace kastle2
{

/**
 * @class SlewGenerator
 * @ingroup dsp_utility
 * @brief Exponential slew generator based on a lookup table
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-05-30
 */
class SlewGenerator
{
public:
    /**
     * @brief Prepares the slew generator and resets all values
     */
    SlewGenerator() : current_value_(0), target_value_(0), step_index_(SLEW_GENERATOR_TABLE_SIZE) {}

    /**
     * @brief Updates the value the output will slew towards
     * @param value The un-smooth value in int32_t format
     */
    void SetValue(const int32_t value)
    {
        target_value_ = value;
        step_index_ = 0;
    }
    /**
     * @brief Updates the smooth output value
     * @return Smooth value in int32_t format
     */
    int32_t Process()
    {
        if (step_index_ < SLEW_GENERATOR_TABLE_SIZE)
        {
            // Apply exponential decay from the lookup table
            int32_t delta = static_cast<int64_t>(target_value_ - current_value_) * slew_generator_table[step_index_] / Q31_MAX;
            current_value_ += delta;
            ++step_index_;
        }
        return current_value_;
    }

private:
    int32_t current_value_ = 0;
    int32_t target_value_ = 0;
    int step_index_ = 0;
};
}
