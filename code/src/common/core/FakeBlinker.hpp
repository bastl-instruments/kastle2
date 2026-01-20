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

#include "common/core/Clock.hpp"
#include "common/dsp/control/Lfo.hpp"
#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

/**
 * @class Hardware
 * @ingroup core
 * @brief Utility class to fake LED blinking for tempo and LFO indicators.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-10-24
 */
class FakeBlinker
{

public:
    /**
     * @brief Initializes the FakeBlinker state.
     */
    void Init()
    {
        enabled_ = false;
        state_ = false;
    }

    /**
     * @brief Toggles the internal state for fake blinking.
     * @note Should be right after the LEDs are updated.
     */
    void Process()
    {
        state_ = !state_;
    }

    /**
     * @brief Gets the current state of the tempo LED.
     * @param clock Reference to the Clock object.
     * @return true if the LED should be on, false otherwise.
     */
    bool GetTempoLedState(const Clock &clock) const
    {
        bool fake_blinking = enabled_ && clock.GetTargetTicks() < tempo_threshold_;
        // Fake the blinking based on the internal state
        if (fake_blinking)
        {
            return state_;
        }
        // Normal operation
        return clock.GetHalfDutyOutput() || (clock.GetTargetTicks() == 0);
    }

    /**
     * @brief Gets the brightness value for the LFO LED.
     * @param lfo Reference to the Lfo object.
     * @return Brightness value (0-255).
     */
    uint8_t GetLfoLedBrightness(const Lfo &lfo) const
    {

        // Scale down to 8 bits and shift to 0-255
        int32_t lfo_value = (lfo.GetTriangleOut() >> 24) + 128;

        // When LFO is too fast, we need to fake blinking because of the interferences
        bool fake_blinking = enabled_ && lfo.GetPhaseIncrement() > lfo_threshold_;
        if (fake_blinking)
        {
            // Fake the blinking so it's actually somehow nice
            return (state_) ? 200 : 20;
        }
        return lfo_value;
    }

    /**
     * @brief Enables or disables fake blinking.
     * @param enabled Set to true to enable fake blinking.
     * @note Default: false.
     */
    void SetEnabled(bool enabled)
    {
        enabled_ = enabled;
    }

    /**
     * @brief Sets the clock cycles under which fake blinking is used.
     * @param clock_cycles Threshold in clock cycles.
     * @note Default: 60 clock cycles.
     */
    void SetTempoThreshold(size_t clock_cycles)
    {
        tempo_threshold_ = clock_cycles;
    }

    /**
     * @brief Sets the LFO phase increment threshold above which fake blinking is used.
     * @param lfo_phase_inc LFO phase increment threshold.
     * @note Default: q31(0.03f).
     */
    void SetLfoThreshold(q31_t lfo_phase_inc)
    {
        lfo_threshold_ = lfo_phase_inc;
    }

private:
    bool enabled_ = false;
    bool state_ = false;
    size_t tempo_threshold_ = 60;
    q31_t lfo_threshold_ = q31(0.03f);
};
}