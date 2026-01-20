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

namespace kastle2
{

/**
 * @class EdgeDetector
 * @ingroup dsp_utility
 * @brief Detects rising or falling edges in a boolean signal
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-05-22
 *
 * The EdgeDetector utility class detects rising or falling edges in boolean signals.
 * It can be used to simplify code patterns like:
 *
 *     if (signal && !prev_signal) {
 *         // Handle rising edge
 *     }
 *     prev_signal = signal;
 *
 * Use cases include detecting button presses or clock triggers.
 */
class EdgeDetector
{
public:
    /**
     * @brief Defines the type of edge to detect
     */
    enum class Type
    {
        RISING, ///< Detect change from false to true
        FALLING ///< Detect change from true to false
    };

    /**
     * @brief Constructor
     * @param type The type of edge to detect (RISING or FALLING)
     */
    EdgeDetector(Type type);

    /**
     * @brief Detects an edge in the input signal
     * @param value The current input value
     * @return true if the specified edge was detected, false otherwise
     */
    bool Process(bool value)
    {
        bool detected = false;

        if (type_ == Type::RISING)
        {
            detected = value && !prev_value_;
        }
        else // Type::FALLING
        {
            detected = !value && prev_value_;
        }

        prev_value_ = value;
        return detected;
    }

private:
    Type type_;       ///< Type of edge to detect
    bool prev_value_; ///< Previous input value
};

inline EdgeDetector::EdgeDetector(Type type)
    : type_(type), prev_value_(type == Type::RISING ? false : true) // Initialize based on edge type
{
}

} // namespace kastle2
