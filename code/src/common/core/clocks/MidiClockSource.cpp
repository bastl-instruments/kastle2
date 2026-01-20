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

#include "MidiClockSource.hpp"
#include "common/core/Kastle2.hpp"
#include "common/core/Kastle2_parameters.hpp"
#include "common/dsp/math/math_utils.hpp"

namespace kastle2
{

void MidiClockSource::Init([[maybe_unused]] float sample_rate) {}

void MidiClockSource::SetPot(int32_t pot_value)
{
    uint32_t index = map(pot_value, POT_MIN, POT_MAX + 10, 0, kMidiTempoDividers.size());
    uint32_t new_divider = kMidiTempoDividers[index];
    SetDivider(new_divider);
}

void MidiClockSource::SetDivider(const uint32_t divider)
{
    next_midi_beat_divider_ = divider;
}

uint32_t MidiClockSource::GetTotalSteps()
{
    return midi_beat_total_ / midi_beat_divider_;
}

void MidiClockSource::Start()
{
    state_ = State::RUNNING;
    midi_beat_count_ = 0;
    midi_beat_total_ = 0;
    current_ticks_ = 0;
    midi_ticks_ = 0;
    prev_midi_ticks_ = 0;
    first_sync_signal = true;
    just_started_ = true;
    now_reset_ = false;
}

void MidiClockSource::Stop()
{
    state_ = State::STOPPED;
}

void MidiClockSource::Resume()
{
    state_ = State::RUNNING;
}

bool MidiClockSource::IsNowReset() const
{
    return now_reset_;
}

void MidiClockSource::SetSyncJackPlugged([[maybe_unused]] bool jack_plugged)
{
}

bool MidiClockSource::IsReachingNextCycle() const
{
    return current_ticks_ + 3 >= GetTargetTicks();
}

bool MidiClockSource::ClockNotArriving()
{
    return (
        prev_midi_ticks_ > 0 &&
        midi_ticks_ >= kUnavailableAfterMultiplier * prev_midi_ticks_ &&
        midi_ticks_ > 48 // To avoid glitching when clock too fast
    );
}

void MidiClockSource::Process([[maybe_unused]] bool raw_sync_input, bool midi_pulse_input)
{
    now_reset_ = false;
    if (just_started_)
    {
        now_reset_ = true;
        just_started_ = false;

        // We don't want to increment the ticks
        // on the first clock signal etc.
        return;
    };

    // For regular tempo calculation
    if (current_ticks_ < UINT32_MAX)
    {
        ++current_ticks_;
    }

    // Just for detecting whether the clock is running
    if (midi_ticks_ < UINT32_MAX)
    {
        ++midi_ticks_;
    }

    // Are we getting clock?
    if (state_ == State::RUNNING && ClockNotArriving())
    {
        state_ = State::UNAVAILABLE;
        Kastle2::midi.ReportDisconnected();
        return;
    }

    // WE GOT A CLOCK SIGNAL!
    if (midi_pulse_input)
    {
        // Divider change?
        // Doing it here to avoid changing the divider in the middle of the cycle
        if (next_midi_beat_divider_ != midi_beat_divider_)
        {
            midi_beat_divider_ = next_midi_beat_divider_;
            target_ticks_ = midi_ticks_ * midi_beat_divider_;

            if (midi_beat_total_ < UINT32_MAX)
            {
                // Realign the divider itself
                midi_beat_count_ = midi_beat_total_ % midi_beat_divider_;

                // Realign the sequencer position
                Kastle2::base.GetSequencer().RealignTo(midi_beat_total_ / midi_beat_divider_);
            }
        }

        // Increment the ticks and total steps
        ++midi_beat_count_;
        if (midi_beat_total_ < UINT32_MAX)
        {
            ++midi_beat_total_;
        }

        // We reached the next cycle...
        if (midi_beat_count_ >= midi_beat_divider_)
        {
            if (!first_sync_signal)
            {
                prev_midi_ticks_ = midi_ticks_;
                target_ticks_ = current_ticks_;

                // Some devices send clock even if stopped, we need to set it to running
                // only when UNAVAILABLE on startup
                if (state_ == State::UNAVAILABLE)
                {
                    state_ = State::RUNNING;
                }
            }
            midi_beat_count_ = 0;
            current_ticks_ = 0;
        }
        midi_ticks_ = 0;
        first_sync_signal = false;
    }
}

void MidiClockSource::TapResetsTicks([[maybe_unused]] bool force)
{
    // If stopped and no midi clock is arriving, when user taps tempo set to unvailable
    if (state_ == State::STOPPED && ClockNotArriving())
    {
        state_ = State::UNAVAILABLE;
        Kastle2::midi.ReportDisconnected();
        return;
    }

    // Ignore the rest, since we don't want to mess up the sequence
}

void MidiClockSource::SetTapTicks(uint32_t tap_ticks)
{
    if (state_ != State::RUNNING)
    {
        return;
    }

    uint32_t closest_divider = kMidiTempoDividers[0];
    uint32_t min_diff = UINT32_MAX;
    for (auto divider : kMidiTempoDividers)
    {
        // Are we close to tap ticks?
        uint32_t result = diff(prev_midi_ticks_ * divider, tap_ticks);
        if (result < min_diff)
        {
            min_diff = result;
            closest_divider = divider;
        }
    }
    SetDivider(closest_divider);
}

uint32_t MidiClockSource::GetTargetTicks() const
{
    return target_ticks_;
}

uint32_t MidiClockSource::GetCurrentTicks() const
{
    return current_ticks_;
}

ClockSource::State MidiClockSource::GetState() const
{
    return state_;
}

void MidiClockSource::SaveToMemory()
{
    Kastle2::memory.QueueUpdate8(Memory::ADDR_CLOCK_MIDI_DIVIDER, midi_beat_divider_);
}

void MidiClockSource::LoadFromMemory()
{
    uint8_t divider;
    if (Kastle2::memory.Read8(Memory::ADDR_CLOCK_MIDI_DIVIDER, &divider))
    {
        for (auto d : kMidiTempoDividers)
        {
            if (divider == d)
            {
                midi_beat_divider_ = divider;
                next_midi_beat_divider_ = divider;
                break;
            }
        }
    }
}

}
