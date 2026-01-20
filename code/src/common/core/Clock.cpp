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

#include "Clock.hpp"

#include <cstdint>
#include <memory>
#include "common/core/Kastle2.hpp"
#include "common/core/Kastle2_parameters.hpp"
#include "common/dsp/math/math_utils.hpp"

#include "common/core/clocks/ExternalClockSource.hpp"
#include "common/core/clocks/InternalClockSource.hpp"
#include "common/core/clocks/MidiClockSource.hpp"

namespace kastle2
{

void Clock::Init(float sample_rate)
{
    clocks_[Sync::INTERNAL] = std::make_unique<InternalClockSource>();
    clocks_[Sync::EXTERNAL] = std::make_unique<ExternalClockSource>();
    clocks_[Sync::MIDI] = std::make_unique<MidiClockSource>();

    for (auto &clock_impl : clocks_)
    {
        clock_impl->Init(sample_rate);
    }
    SetSyncType(Sync::INTERNAL);
}

bool Clock::IsOutputEnabled() const
{
    return ((tap_state_ == TapState::ACTIVE ||
             tap_state_ == TapState::NONE) &&
            GetState() == ClockSource::State::RUNNING);
}

bool Clock::IsNowTrigger() const
{
    return GetCurrentTicks() == 0 && IsOutputEnabled();
}

bool Clock::GetSyncOutput() const
{
    return sync_out_state_ && IsOutputEnabled();
}

bool Clock::GetHalfDutyOutput() const
{
    return half_duty_state_ && IsOutputEnabled();
}

void Clock::ClearTaps()
{
    tap_tempo_values_.Reset();
    tap_state_ = TapState::NONE;
}

void Clock::SetSyncJackPlugged(bool jack_plugged)
{
    for (auto &clock_impl : clocks_)
    {
        clock_impl->SetSyncJackPlugged(jack_plugged);
    }
}

bool Clock::Process(bool raw_tap_input, bool raw_sync_input, bool midi_pulse_input)
{
    now_reset_ = false;

    for (auto &clock_impl : clocks_)
    {
        clock_impl->Process(raw_sync_input, midi_pulse_input);
    }

    for (Sync type : EnumRange<Sync>())
    {
        if (clocks_[type]->GetState() != ClockSource::State::UNAVAILABLE)
        {
            SetSyncType(type);
            break;
        }
    }

    // Reset sequencer on Sync type change
    if (prev_sync_type_ != sync_type_)
    {
        next_cycle_reset_ = true;
    }
    prev_sync_type_ = sync_type_;

    if (midi_source_ != midi::Message::Source::NONE && sync_type_ != Sync::MIDI)
    {
        midi_source_ = midi::Message::Source::NONE;
    }

    ProcessTapTempo(raw_tap_input);

    bool now_trigger = IsNowTrigger();
    if (now_trigger)
    {
        half_duty_state_ = true;
        sync_out_state_ = true;
        avg_target_ticks_.Add(GetTargetTicks());
    }
    if (GetCurrentTicks() >= GetTargetTicks() / 2)
    {
        half_duty_state_ = false;
    }
    if (GetCurrentTicks() >= kSyncOutTicks)
    {
        sync_out_state_ = false;
    }

    if (now_trigger && next_cycle_reset_)
    {
        now_reset_ = true;
        next_cycle_reset_ = false;
    }

    HandleMidiOutClock();

    return now_trigger;
}

bool Clock::CheckMidiSource(midi::Message::Source source)
{
    if (midi_source_ == midi::Message::Source::NONE ||
        midi_source_ == source)
    {
        midi_source_ = source;
        return true;
    }
    return false;
}

void Clock::SetPot(int32_t pot_value)
{
    if (pot_state_ == PotState::UNDEFINED)
    {
        prev_pot_value_ = pot_value;
        pot_state_ = PotState::REQUIRES_THRESHOLD;
    }

    if (pot_state_ == PotState::REQUIRES_THRESHOLD)
    {
        if (diff(pot_value, prev_pot_value_) < kPotChangeThreshold)
        {
            return;
        }
    }
    if (pot_value == prev_pot_value_)
    {
        return;
    }
    clocks_[sync_type_]->SetPot(pot_value);
    prev_pot_value_ = pot_value;
    pot_state_ = PotState::ACTIVE;
}

void Clock::LoadFromMemory()
{
    for (auto &clock_impl : clocks_)
    {
        clock_impl->LoadFromMemory();
    }
}

void Clock::SaveToMemory()
{
    for (auto &clock_impl : clocks_)
    {
        clock_impl->SaveToMemory();
    }
}

void Clock::SetSyncType(Sync sync_type)
{
    if (sync_type_ == sync_type)
    {
        return;
    }
    sync_type_ = sync_type;
    avg_target_ticks_.Reset();
    if (pot_state_ == PotState::ACTIVE)
    {
        pot_state_ = PotState::REQUIRES_THRESHOLD;
    }
}

void Clock::ProcessTapTempo(bool raw_tap_input)
{
    if (tap_ticks_ < UINT32_MAX)
    {
        ++tap_ticks_;
    }

    if (tap_state_ == TapState::WAITING_FOR_SECOND_TAP && tap_ticks_ > kTapTempoMaxTicks)
    {
        ClearTaps();
    }

    if (tap_edge_.Process(raw_tap_input))
    {
        // Prevent boucing etc.Add commentMore actions
        if (tap_ticks_ > kTapTempoMinTicks)
        {
            clocks_[sync_type_]->TapResetsTicks(tap_state_ == TapState::WAITING_FOR_SECOND_TAP);

            // Are we in allowed range?
            if (tap_ticks_ <= kTapTempoMaxTicks && tap_state_ != TapState::NONE)
            {
                // If so, add to the measurements
                tap_tempo_values_.Add(tap_ticks_);
                tap_state_ = TapState::ACTIVE;
                uint32_t result = tap_tempo_values_.GetAverage() / kTapTempoMultiplier;
                clocks_[sync_type_]->SetTapTicks(result);
                pot_state_ = PotState::REQUIRES_THRESHOLD;
            }
            else
            {
                // If not, reset the measurements
                ClearTaps();
                tap_state_ = TapState::WAITING_FOR_SECOND_TAP;
            }
            tap_ticks_ = 0;
        }
    }
}

void Clock::NextCycleReset()
{
    next_cycle_reset_ = true;
}

void Clock::HandleMidiOutClock()
{
    // Don't send MIDI clock if we receive MIDI clock
    // Todo: MIDI clock pass-through?
    if (sync_type_ == Sync::MIDI)
    {
        return;
    }

    // Send start/stop messages if the clock state changes
    if (sync_type_ == Sync::EXTERNAL)
    {
        ClockSource::State clock_state = clocks_[Sync::EXTERNAL]->GetState();
        if (clock_state == ClockSource::State::RUNNING &&
            midi_prev_clock_state_ != ClockSource::State::RUNNING)
        {
            Kastle2::midi.SendClockStart();
        }
        if (clock_state == ClockSource::State::STOPPED &&
            midi_prev_clock_state_ != ClockSource::State::STOPPED)
        {
            Kastle2::midi.SendClockStop();
        }
        midi_prev_clock_state_ = clock_state;
    }

    if (IsNowTrigger())
    {
        midi_pulses_sent_ = 0;
        if (IsNowReset())
        {
            Kastle2::midi.SendClockReset();
        }
    }

    // Get the divided target MIDI ticks
    uint32_t target_midi_ticks = GetTargetTicks() / kOutputMidiPulseMultiplier;
    uint32_t current_ticks = GetCurrentTicks();

    // Check if it's time to send the next MIDI clock pulse
    if (current_ticks >= midi_pulses_sent_ * target_midi_ticks &&
        midi_pulses_sent_ < kOutputMidiPulseMultiplier)
    {
        Kastle2::midi.SendClockPulse();
        midi_pulses_sent_++;
    }
}

}
