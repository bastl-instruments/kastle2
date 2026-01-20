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

#include "ClockSource.hpp"
#include <cstdint>

namespace kastle2
{

/**
 * @class InternalClockSource
 * @ingroup core_clocks
 * @brief Internal clock runs all the time and provides fallback for other clocks.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-05-21
 */

class InternalClockSource : public ClockSource
{
public:
    void Init(float sample_rate) override;
    void Start() override;
    void Stop() override;
    void Resume() override;
    bool IsNowReset() const override;
    void SetSyncJackPlugged(bool jack_plugged) override;
    void SetPot(int32_t pot_value) override;
    void Process(bool raw_sync_input, bool midi_pulse_input) override;
    State GetState() const override;
    bool IsReachingNextCycle() const override;
    void SetTapTicks(uint32_t tap_ticks) override;
    uint32_t GetTargetTicks() const override;
    uint32_t GetCurrentTicks() const override;
    void SaveToMemory() override;
    void LoadFromMemory() override;
    void TapResetsTicks(bool force) override;
    uint32_t GetTotalSteps() override;

private:
    uint32_t target_ticks_ = UINT32_MAX;
    uint32_t current_ticks_ = 0;
    uint32_t total_ticks_ = 0;
};
}
