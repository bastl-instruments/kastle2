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

#include "InternalClockSource.hpp"
#include "common/core/Kastle2.hpp"
#include "common/core/Kastle2_parameters.hpp"
#include "common/dsp/math/math_utils.hpp"

namespace kastle2
{

void InternalClockSource::Init([[maybe_unused]] float sample_rate)
{
    target_ticks_ = UINT32_MAX;
    current_ticks_ = 0;
    total_ticks_ = 0;
}

void InternalClockSource::Start()
{
}

void InternalClockSource::Stop()
{
}

void InternalClockSource::Resume()
{
}

bool InternalClockSource::IsNowReset() const
{
    return false;
}

void InternalClockSource::SetSyncJackPlugged([[maybe_unused]] bool jack_plugged)
{
}

void InternalClockSource::SetPot(int32_t pot_value)
{
    target_ticks_ = curve_map(pot_value, kTempoMap);
}

void InternalClockSource::Process([[maybe_unused]] bool raw_sync_input, [[maybe_unused]] bool midi_pulse_input)
{
    ++current_ticks_;
    ++total_ticks_; // this will overflow but we don't care much
    if (current_ticks_ >= target_ticks_)
    {
        current_ticks_ = 0;
    }
}

ClockSource::State InternalClockSource::GetState() const
{
    return State::RUNNING;
}

bool InternalClockSource::IsReachingNextCycle() const
{
    return current_ticks_ + 2 >= target_ticks_;
}

void InternalClockSource::SetTapTicks(uint32_t tap_ticks)
{
    target_ticks_ = tap_ticks;
}

void InternalClockSource::TapResetsTicks(bool force)
{
    // Only if big enough difference to prevent double taps
    if (force || current_ticks_ > kTapTempoJustHappenedTicks)
    {
        current_ticks_ = 0;
    }
}

uint32_t InternalClockSource::GetTargetTicks() const
{
    return target_ticks_;
}

uint32_t InternalClockSource::GetCurrentTicks() const
{
    return current_ticks_;
}

uint32_t InternalClockSource::GetTotalSteps()
{
    return total_ticks_;
}

void InternalClockSource::SaveToMemory()
{
    Kastle2::memory.QueueUpdate32(Memory::ADDR_CLOCK_TICKS, target_ticks_);
}

void InternalClockSource::LoadFromMemory()
{
    uint32_t ticks;
    if (Kastle2::memory.Read32(Memory::ADDR_CLOCK_TICKS, &ticks))
    {
        if (ticks > 0)
        {
            target_ticks_ = ticks;
        }
    }
}

}
