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
#include "common/core/Kastle2_parameters.hpp"
#include "ClockSource.hpp"

namespace kastle2
{

/**
 * @class MidiClockSource
 * @ingroup core_clocks
 * @brief Handles both USB MIDI and TRS MIDI (on Citadel) clock sources.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-05-21
 */

class MidiClockSource : public ClockSource
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
    uint32_t midi_beat_divider_ = kMidiTempoDividerDefault;
    uint32_t next_midi_beat_divider_ = kMidiTempoDividerDefault;
    uint32_t midi_beat_count_ = 0;
    uint32_t midi_beat_total_ = 0;

    uint32_t current_ticks_ = 0;
    uint32_t target_ticks_ = 0;

    bool now_reset_ = false;
    bool just_started_ = false;

    State state_ = State::UNAVAILABLE;
    bool first_sync_signal = true;

    uint32_t midi_ticks_ = 0;
    uint32_t prev_midi_ticks_ = 0;
    static constexpr uint32_t kUnavailableAfterMultiplier = 4;

    inline bool ClockNotArriving();
    void SetDivider(const uint32_t divider);
};

}
