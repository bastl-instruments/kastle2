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

#include <array>

#include "common/core/Hardware.hpp"
#include "common/core/Kastle2.hpp"

namespace kastle2
{

/**
 * @class DirtyInputsHandler
 * @ingroup controls
 * @brief For handling ADC inputs you want to be 100% sure they will have the latest values (usually for S&H).
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-04-29
 */

template <size_t N>
class DirtyInputsHandler
{
public:
    /**
     * @brief Constructor for the DirtyInputsHandler.
     * @param inputs The array of inputs to handle.
     */
    DirtyInputsHandler(std::array<Hardware::AnalogInput, N> inputs) : inputs_(inputs) {}

    /**
     * @brief Trigger received, we need to flag inputs as dirty.
     */
    void Trigger()
    {
        trigger_flag_ = true;
        for (Hardware::AnalogInput input : inputs_)
        {
            Kastle2::hw.SetAnalogInputDirty(input);
        }
    }

    /**
     * @brief Are we waiting for the trigger to happen soon?
     */
    bool IsBusy() const
    {
        return trigger_flag_;
    }

    /**
     * @brief Filtered output.
     */
    bool GetTrigger()
    {
        // First check trigger flag
        if (!trigger_flag_)
        {
            return false;
        }
        // Check whether all inputs OK
        const bool inputs_ok = std::all_of(inputs_.begin(), inputs_.end(), [](const Hardware::AnalogInput input)
                                           { return !Kastle2::hw.GetAnalogInputDirty(input); });
        if (!inputs_ok)
        {
            return false;
        }

        // OK, can do trigger
        trigger_flag_ = false;
        return true;
    }

private:
    std::array<Hardware::AnalogInput, N> inputs_; ///< The array of inputs to handle.
    bool trigger_flag_ = false;                   ///< The trigger flag.
};

}
