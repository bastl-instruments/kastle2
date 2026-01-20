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

#include "ExternalClockSource.hpp"
#include "common/core/Kastle2.hpp"
#include "common/core/Kastle2_parameters.hpp"
#include "common/dsp/math/math_utils.hpp"

namespace kastle2
{

void ExternalClockSource::Init([[maybe_unused]] float sample_rate) {}

void ExternalClockSource::Start()
{
    state_ = State::RUNNING;
    current_ticks_ = 0;
    current_division_ = 0;
    current_multiplication_ = 0;
    total_input_trigs_ = 0;
}

void ExternalClockSource::Stop()
{
    state_ = State::STOPPED;
}

void ExternalClockSource::Resume()
{
    state_ = State::RUNNING;
}

bool ExternalClockSource::IsNowReset() const
{
    return now_reset_;
}

void ExternalClockSource::SetPot(int32_t pot_value)
{
    int32_t val = diff(pot_value, POT_HALF);
    uint32_t index = map(val, POT_MIN, POT_HALF + 10, 0, kDividersMultipliers.size());
    uint32_t divmult = kDividersMultipliers[index];
    if (pot_value < POT_HALF)
    {
        SetExtDividerMultiplier(divmult, 1);
    }
    else
    {
        SetExtDividerMultiplier(1, divmult);
    }
}

bool ExternalClockSource::IsReachingNextCycle() const
{
    return current_ticks_ + 3 >= GetTargetTicks();
}

void ExternalClockSource::SetSyncJackPlugged(bool jack_plugged)
{
    sync_jack_plugged_ = jack_plugged;

    // On Citadel there is no patchbay - so we precisely now if the jack is plugged or not
    // When jack is unplugged, we quickly set it to unavailable and don't need to wait
    if (Kastle2::hw.GetVersion() == Hardware::Version::CITADEL)
    {
        if (!jack_plugged)
        {
            state_ = State::UNAVAILABLE;
        }
    }
}

uint32_t ExternalClockSource::GetTotalSteps()
{
    // this will overflow but idk is that a problem?
    return total_input_trigs_ * ext_multiplier_ / ext_divider_;
}

void ExternalClockSource::Process(bool raw_sync_input, [[maybe_unused]] bool midi_pulse_input)
{
    now_reset_ = false;
    if (ext_ticks_ < UINT32_MAX)
    {
        ++ext_ticks_;
    }
    if (current_ticks_ < UINT32_MAX)
    {
        ++current_ticks_;
    }

    if (sync_input_.Process(raw_sync_input))
    {
        if (total_input_trigs_ < UINT32_MAX)
        {
            ++total_input_trigs_;
        }
        if (!first_analog_sync_)
        {
            if ((state_ == State::RUNNING) && ext_ticks_ < kExtClockProbablyStopped)
            {
                ++current_division_;
                if (current_division_ >= ext_divider_)
                {
                    target_ticks_ = ext_ticks_ * ext_divider_ / ext_multiplier_;
                    current_division_ = 0;
                    current_ticks_ = 0;
                    current_multiplication_ = 0;
                }
            }
            else
            {
                Start();
                now_reset_ = true;
            }
        }
        else
        {
            now_reset_ = true;
        }
        prev_ext_ticks_ = ext_ticks_;
        ext_ticks_ = 0;
        first_analog_sync_ = false;
    }

    if (current_ticks_ >= target_ticks_ && (current_multiplication_ + 1) < ext_multiplier_)
    {
        ++current_multiplication_;
        current_ticks_ = 0;
    }

    if (!sync_jack_plugged_)
    {
        if (ext_ticks_ > kExtClockProbablyStopped)
        {
            state_ = State::UNAVAILABLE;
        }
    }
    else
    {
        if (ext_ticks_ > kExtClockProbablyStopped || ext_ticks_ > prev_ext_ticks_ * 2)
        {
            state_ = State::STOPPED;
        }
    }
}

void ExternalClockSource::SetTapTicks(uint32_t tap_ticks)
{
    uint32_t real_ext_ticks = target_ticks_ * ext_multiplier_ / ext_divider_;
    size_t index = 0;
    int32_t d = diff(real_ext_ticks, tap_ticks);

    if (tap_ticks == real_ext_ticks)
    {
        ext_divider_ = 1;
        ext_multiplier_ = 1;
    }
    else if (tap_ticks > real_ext_ticks)
    {
        for (size_t i = 0; i < kDividersMultipliers.size(); i++)
        {
            int32_t result = diff(real_ext_ticks, tap_ticks / kDividersMultipliers[i]);
            if (result < d)
            {
                d = result;
                index = i;
            }
        }
        ext_divider_ = kDividersMultipliers[index];
        ext_multiplier_ = 1;
        target_ticks_ = real_ext_ticks * ext_divider_;
        current_division_ = ext_divider_ - 1;
        current_multiplication_ = 0;
    }
    else
    {
        for (size_t i = 0; i < kDividersMultipliers.size(); i++)
        {
            int32_t result = diff(real_ext_ticks, tap_ticks * kDividersMultipliers[i]);
            if (result < d)
            {
                d = result;
                index = i;
            }
        }
        ext_divider_ = 1;
        ext_multiplier_ = kDividersMultipliers[index];
        target_ticks_ = real_ext_ticks / ext_multiplier_;
    }
}

void ExternalClockSource::TapResetsTicks(bool force)
{
    // Only if big enough difference to prevent double taps
    if (force || current_ticks_ > kTapTempoJustHappenedTicks)
    {
        current_ticks_ = 0;
    }
}

uint32_t ExternalClockSource::GetTargetTicks() const
{
    return target_ticks_;
}

uint32_t ExternalClockSource::GetCurrentTicks() const
{
    return current_ticks_;
}

void ExternalClockSource::SaveToMemory()
{
    Kastle2::memory.QueueUpdate8(Memory::ADDR_CLOCK_DIVIDER, ext_divider_);
    Kastle2::memory.QueueUpdate8(Memory::ADDR_CLOCK_MULTIPLIER, ext_multiplier_);
}

void ExternalClockSource::LoadFromMemory()
{
    uint8_t divider;
    uint8_t multiplier;

    if (Kastle2::memory.Read8(Memory::ADDR_CLOCK_DIVIDER, &divider))
    {
        // Find whether it exists
        for (auto d : kDividersMultipliers)
        {
            if (divider == d)
            {
                ext_divider_ = divider;
                break;
            }
        }
    }

    if (Kastle2::memory.Read8(Memory::ADDR_CLOCK_MULTIPLIER, &multiplier))
    {
        // Find whether it exists
        for (auto m : kDividersMultipliers)
        {
            if (multiplier == m)
            {
                ext_multiplier_ = multiplier;
                break;
            }
        }
    }

    current_division_ = 0;
    current_multiplication_ = 0;
}

ClockSource::State ExternalClockSource::GetState() const
{
    return state_;
}

void ExternalClockSource::SetExtDividerMultiplier(uint8_t ext_divider, uint8_t ext_multiplier)
{
    if (ext_divider == ext_divider_ && ext_multiplier == ext_multiplier_)
    {
        return;
    }
    ext_divider_ = ext_divider;
    ext_multiplier_ = ext_multiplier;
    current_division_ = current_division_ % ext_divider_;
    current_multiplication_ = current_multiplication_ % ext_multiplier_;
    // Realign the sequencer
    Kastle2::base.GetSequencer().RealignTo(total_input_trigs_ * ext_multiplier_ / ext_divider_);
}

}
