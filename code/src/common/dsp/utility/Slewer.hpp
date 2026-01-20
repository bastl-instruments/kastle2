/*
MIT License

Copyright (c) 2025 Vaclav Mach (Bastl Instruments)

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

namespace kastle2
{

/**
 * @class Slewer
 * @ingroup dsp_utility
 * @brief Simple linear slew limiter for smoothing int32_t values with configurable speed
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-06-26
 */
class Slewer
{
public:
    /**
     * @brief Initialize the slewer
     */
    void Init()
    {
        current_value_ = 0;
        target_value_ = 0;
        speed_ = 1;
    }

    /**
     * @brief Set the slewing speed
     * @param speed The step size per Process() call. Higher values = faster slewing.
     *              Must be > 0. Default is 1.
     * @note Recommended values are usually between 1 and 10, depending on interval between Process() calls.
     */
    void SetSpeed(int32_t speed)
    {
        speed_ = speed > 0 ? speed : 1;
    }

    /**
     * @brief Updates the value the output will slew towards
     * @param value The target value in int32_t format
     */
    void SetValue(int32_t value)
    {
        target_value_ = value;
    }

    /**
     * @brief Updates the smooth output value
     * @return Smooth value in int32_t format
     */
    int32_t Process()
    {
        if (current_value_ != target_value_)
        {
            int32_t difference = target_value_ - current_value_;

            if (difference > 0)
            {
                // Moving towards higher value
                if (difference <= speed_)
                {
                    current_value_ = target_value_;
                }
                else
                {
                    current_value_ += speed_;
                }
            }
            else
            {
                // Moving towards lower value
                if (-difference <= speed_)
                {
                    current_value_ = target_value_;
                }
                else
                {
                    current_value_ -= speed_;
                }
            }
        }
        return current_value_;
    }

    /**
     * @brief Get the current value without processing
     * @return Current smooth value
     */
    int32_t GetValue() const
    {
        return current_value_;
    }

    /**
     * @brief Check if the slewer has reached its target
     * @return True if current value equals target value
     */
    bool IsAtTarget() const
    {
        return current_value_ == target_value_;
    }

    /**
     * @brief Force the current value to the target immediately
     */
    void Jump()
    {
        current_value_ = target_value_;
    }

private:
    int32_t current_value_ = 0;
    int32_t target_value_ = 0;
    int32_t speed_ = 1;
};
}
