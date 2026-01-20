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

#include "common/dsp/utility/EdgeDetector.hpp"
#include "ClockSource.hpp"
#include <cstdint>

namespace kastle2
{

/**
 * @class ExternalClockSource
 * @ingroup core_clocks
 * @brief External clock handling using Sync In jack or patchbay.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-05-21
 */

class ExternalClockSource : public ClockSource
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
    void SetExtDividerMultiplier(uint8_t ext_divider, uint8_t ext_multiplier);

    bool now_reset_ = false;

    uint32_t target_ticks_ = 0;
    uint32_t current_ticks_ = 0;
    State state_ = State::UNAVAILABLE;
    uint32_t ext_ticks_ = 0;
    uint32_t prev_ext_ticks_ = 0;
    uint32_t total_input_trigs_ = 0;

    bool first_analog_sync_ = true;

    bool sync_jack_plugged_ = false;
    EdgeDetector sync_input_ = EdgeDetector(EdgeDetector::Type::RISING);

    uint8_t ext_divider_ = 0;
    uint8_t ext_multiplier_ = 0;

    uint32_t current_division_ = 0;
    uint32_t current_multiplication_ = 0;
};

}
