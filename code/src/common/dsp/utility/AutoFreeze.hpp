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

#pragma once

#include <common/dsp/math/math_utils.hpp>
#include <cmath>

namespace kastle2
{

/**
 * @class AutoFreeze
 * @ingroup dsp_utility
 * @brief Value filter that freezes the value if it hasn't moved over a threshold in a certain time.
 *        When frozen, the value needs to change above the threshold to unfreeze again.
 * @author Marek Mach (Bastl Instruments)
 * @date 2024-09-10
 */

template <typename T>
class AutoFreeze
{
public:

    /**
     * @brief Initializes the AutoFreeze object and all it's variables
     * @param update_rate With what frequency will the Process() function be called
     */
    void Init(const float update_rate)
    {
        time_per_tick_ = (1000.f / update_rate) * 1000.f;
        freeze_time_ = 0;
        freeze_timer_ = 0;
        value_in_ = 0;
        value_out_ = 0;
        threshold_ = 0;
    }

    /**
     * @brief Sets how long the value has to stay within the threshold bounds to stop updating the output value
     * @param time The time until pot gets frozen in seconds
     */
    void SetFreezeTime(const float time)
    {
        int64_t tmp = static_cast<uint32_t>(time_per_tick_ * time);
        // constrain the range
        if (tmp < 1)
        {
            freeze_time_ = 1;
            return;
        }
        else if (tmp > UINT32_MAX)
        {
            freeze_time_ = UINT32_MAX;
            return;
        }
        freeze_time_ = tmp;
    }

    /**
     * @brief Sets the maximum change to still be considered as "not changing" to allow freezing. Going above it resets the counter and unfreezes
     * @param threshold Sets the bounds in format (the variable value has to change more than the set amount in either direction)
     */
    void SetThreshold(const T threshold)
    {
        threshold_ = threshold;
    }

    /**
     * @brief Sets the input variable value
     * @param value the input value
     */
    void SetValue(const T value)
    {
        // check for a big enough change
        bool crossed = false;
        if (value > value_in_)
        {
            if (value - value_in_ > threshold_)
            {
                crossed = true;
            }
        }
        else if (value < value_in_)
        {
            if (value_in_ - value > threshold_)
            {
                crossed = true;
            }
        }

        if (crossed)
        {
            freeze_timer_ = 0;
        }

        if (crossed || freeze_timer_ < freeze_time_)
        {
            value_in_ = value;
        }
    }

    /**
     * @brief Returns the either frozen or continuously updated value
     * @return Set variable either frozen or not
     */
    T GetValue()
    {
        if (freeze_timer_ < freeze_time_)
        {
            value_out_ = value_in_;
            return value_in_;
        }
        return value_out_;
    }

    /**
     * @brief Sets the raw value and returns the processed value (either frozen or not frozen)
     * @param value The Raw input value
     * @return The processed value
     */
    T DoFreeze(const T value)
    {
        SetValue(value);
        return GetValue();
    }

    /**
     * @brief Needs to get called with the frequency set in Init() to keep the freeze timing correct.
     */
    void Process()
    {
        if (freeze_timer_ < UINT32_MAX)
        {
            freeze_timer_++;
        }
    }

private:
    float time_per_tick_ = 0.f;
    uint32_t freeze_time_ = 0;
    uint32_t freeze_timer_ = 0;
    T value_in_ = 0;
    T value_out_ = 0;
    T threshold_ = 0;
};
}
