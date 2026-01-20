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

#include <cstddef>
#include "common/dsp/math/Fraction.hpp"
#include "common/dsp/math/qmath.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class Lfo
 * @ingroup dsp_control
 * @brief Low frequency sawtooth and square oscillator syncable to the external clock
 * @author Marek Mach (Bastl Instruments)
 * @date 2024-06-17
 *
 * @note Features "snap free" syncing to clock where the Lfo drifts to sync instead.
 *
 * You either SetFrequency or SetRatio + SetClockTicks. The change will set the `synced_` parameter.
 * 1:256 means the LFO takes 256 clock cycles to complete one cycle.
 *
 */
class Lfo
{

public:
    /**
     * @brief Initializes the oscillator.
     */
    void Init(const float sample_rate);

    /**
     * @brief Sets the frequency of the oscillator.
     * @param frequency The frequency in Hz.
     */
    void SetFrequency(const float frequency);

    /**
     * @brief Set the number of ticks per one clock cycle for synchronized mode
     * @param clock_ticks The number of ticks
     */
    void SetClockTicks(const uint32_t clock_ticks);

    /**
     * @brief Set the ratio by which clock be multiplied for the synced LFO
     *        special ratios are supported (2/3, 3/4, etc.)
     * @param ratio A fraction setting both divider and a multiplier
     */
    void SetRatio(const Fraction &ratio);

    /**
     * @brief Resets the LFO phase to the beginning, basically a sync function
     */
    void Reset();

    /**
     * @brief Changes the phase of the lfo to a given value
     * @param phase The phase value in q31_t, from Q31_MIN to Q31_MAX
     */
    void SetPhase(const q31_t phase);

    /**
     * @brief Resets the LFO phase if it is in sync mode and is supposed to be at the end of it's period
     */
    FASTCODE void SyncWithClock();

    /**
     * @brief Returns if the LFO is in the sync mode
     *        (enabled by calling SetRatio(), disabled by SetFrequency())
     * @return True if it is, false if it's in free-running mode
     */
    bool IsSynced() const;

    /**
     * @brief Disables slowing down the LFO to sync, making it always speed up
     *
     * @param disable True to disable the slowing down, false to enable it
     */
    void DisableSlowingDown(const bool disable);

    /**
     * @brief Returns true if this is the last sample before restartng the period cycle
     * @return True if this is the last sample before restarting the period cycle
     */
    bool inline IsLastSample() const
    {
        return phase_ >= Q31_MAX - phase_inc_;
    }

    /**
     * @brief Stops the lfo
     */
    void inline Stop()
    {
        stopped_ = true;
    }

    /**
     * @brief Starts the lfo from the beginning
     */
    void Start();

    /**
     * @brief Resumes the lfo from the current phase
     */
    void inline Resume()
    {
        stopped_ = false;
    }

    /**
     * @brief Stops the lfo after it finishes the current cycle
     */
    void inline StopAfterCycle()
    {
        stop_after_cycle_ = true;
    }

    /**
     * @brief Returns true if the lfo is stopped
     * @return True if the lfo is stopped
     */
    bool inline IsStopped() const
    {
        return stopped_;
    }

    /**
     * @brief Returns the phase increment value per each Process() tick
     * @return Phase increment in q31_t
     */
    q31_t inline GetPhaseIncrement() const
    {
        return phase_inc_;
    }

    /**
     * @brief Increments the phase and returns the triangle output
     * @return Value from 0 to 1 in Q31, the triangle output
     */
    FASTCODE q31_t Process();

    /**
     * @brief Returns the current triangle waveform value
     * @return Value from 0 to 1 in Q31, the triangle output
     */
    FASTCODE q31_t GetTriangleOut() const;

    /**
     * @brief Returns the current square waveform value
     * @return Value of either 1 or 0, the square output
     */
    FASTCODE bool GetSquareOut() const;

private:
    FASTCODE void ClockedUpdate();

    // Don't sync LFO if too fast
    static constexpr uint32_t kMinSyncedTicks = 30;

    float sample_rate_ = 0.0f;
    q31_t native_frequency_ = Q31_ZERO;
    uint32_t clock_ticks_ = 0;
    Fraction ratio_ = {1, 1};
    q31_t phase_ = Q31_ZERO;
    q31_t phase_synced_ = Q31_ZERO;
    q31_t phase_inc_ = Q31_ZERO;
    q31_t synced_phase_offset_ = Q31_ZERO;
    bool synced_ = false;
    bool disable_lfo_slowdown_ = false;
    uint32_t reset_counter_ = 0;
    uint32_t ticks_prev_ = 0;
    Fraction ratio_prev_ = {1, 1};
    bool stopped_ = false;
    bool stop_after_cycle_ = false;
};
}
