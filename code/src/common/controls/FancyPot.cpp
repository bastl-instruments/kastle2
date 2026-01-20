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

#include "FancyPot.hpp"
#include "common/core/Kastle2.hpp"
#include "common/core/Kastle2_cc.hpp"
#include "common/utils.hpp"

namespace kastle2
{

std::unique_ptr<FancyPot> FancyPot::Create(Config config)
{
    return std::make_unique<FancyPot>(config);
}

FancyPot::FancyPot(Config config) : config_(config)
{
    value_source_ = Source::INTERNAL;
    values_[Source::INTERNAL] = config.initial_value;
    values_[Source::MIDI_CC] = 0;
    values_[Source::MIDI_NOTES] = 0;
    mapped_values_[Source::INTERNAL] = 0;
    mapped_values_[Source::MIDI_CC] = 0;
    mapped_values_[Source::MIDI_NOTES] = 0;

    UpdateInternalMappedValue();
    UpdatePreviousFromCurrent();

    move_thresholds_[Move::TWEAK] = TWEAK_THRESHOLD;
    move_thresholds_[Move::MOVE] = MOVE_THRESHOLD;
}

void FancyPot::Init(const float sample_rate)
{
    if (config_.freeze)
    {
        freezer_.Init(sample_rate);
        freezer_.SetThreshold(0b1111); // Ignore smaller changes than 15
        freezer_.SetFreezeTime(0.5f);  // Freeze after 0.5s
    }
    if (UseMemory())
    {
        LoadFromMemory();
    }
}

void FancyPot::Process()
{
    if (config_.freeze)
    {
        freezer_.Process();
    }
}

void FancyPot::ReadValue()
{
    // Reset change flags (or set if forced)
    has_changed_ = force_change_flag_;
    force_change_flag_ = false;

    // Reset pot movement flags
    for (auto movement : EnumRange<Move>())
    {
        has_pot_moved_[movement] = false;
    }

    int32_t read_value = 0;
    size_t layer_time_now = Kastle2::base.GetLayerTimer();

    // Read internal pot value (even if we use MIDI we need to read the pot value for re-activating, change detection etc.)
    if (layer_time_now > GetLayerTime() &&
        Kastle2::hw.ReadPot(&read_value, config_.pot, config_.layer))
    {
        if (config_.deadzone)
        {
            read_value = curve_map(read_value, kMapDeadzone);
        }
        if (config_.freeze)
        {
            freezer_.DoFreeze(read_value);
            values_[Source::INTERNAL] = freezer_.GetValue();
        }
        else
        {
            values_[Source::INTERNAL] = read_value;
        }
        for (auto movement : EnumRange<Move>())
        {
            if (diff(values_[Source::INTERNAL], prev_pot_values_[movement]) > move_thresholds_[movement])
            {
                has_pot_moved_[movement] = true;                        // Mark as moved
                prev_pot_values_[movement] = values_[Source::INTERNAL]; // Update previous value
                if (value_source_ == Source::INTERNAL)
                {
                    has_changed_ = true; // Mark as changed
                }
                // Large movement, set source to INTERNAL
                if (value_source_ != Source::INTERNAL &&
                    movement == Move::MOVE)
                {
                    value_source_ = Source::INTERNAL;
                }
            }
        }
        UpdateInternalMappedValue();
    }
    // save to memory if needed
    if (UseMemory())
    {
        if (Kastle2::hw.GetLayer() != Kastle2::base.GetPrevLayer())
        {
            if (Kastle2::base.GetPrevLayer() == config_.layer &&
                Kastle2::base.GetPrevLayerTimer() > GetLayerTime())
            {
                SaveToMemory();
            }
        }
    }

    // No read necessary, just check for changes
    if (values_[Source::MIDI_CC] != prev_midi_cc_value_)
    {
        if (value_source_ == Source::MIDI_CC)
        {
            has_changed_ = true; // Mark as changed
        }
        prev_midi_cc_value_ = values_[Source::MIDI_CC]; // Update previous value
    }

    if (values_[Source::MIDI_NOTES] != prev_midi_notes_value_)
    {
        if (value_source_ == Source::MIDI_NOTES)
        {
            has_changed_ = true; // Mark as changed
        }
        prev_midi_notes_value_ = values_[Source::MIDI_NOTES]; // Update previous value
    }

    // Send CCs
    if (config_.midi_output_cc != NO_MIDI)
    {
        int32_t value = GetValue();
        value = sticky_map(value, POT_MIN, POT_MAX, 0, 127, prev_midi_sticky_value_);
        Kastle2::midi.SendCc(config_.midi_output_cc, value);
    }
}

void FancyPot::ClearMidi()
{
    value_source_ = Source::INTERNAL;
    ForceChanged();
}

int32_t FancyPot::GetValue() const
{
    return values_[value_source_];
}

int32_t FancyPot::GetMappedValue() const
{
    return mapped_values_[value_source_];
}

void FancyPot::UpdatePreviousFromCurrent()
{
    for (auto change : EnumRange<Move>())
    {
        prev_pot_values_[change] = values_[Source::INTERNAL]; // Update previous pot value
    }
    prev_midi_cc_value_ = values_[Source::MIDI_CC];       // Update previous MIDI CC value
    prev_midi_notes_value_ = values_[Source::MIDI_NOTES]; // Update previous MIDI notes value
}

void FancyPot::UpdateInternalMappedValue()
{
    if (config_.map_size > 0)
    {
        mapped_values_[Source::INTERNAL] = sticky_map(values_[Source::INTERNAL], 0, POT_MAX, 0, config_.map_size - 1, prev_internal_map_sticky_value_);
    }
}

void FancyPot::ForceValue(const int32_t value, const bool force_changed)
{
    for (auto &v : values_)
    {
        v = constrain(value, POT_MIN, POT_MAX);
    }
    UpdateInternalMappedValue();
    UpdatePreviousFromCurrent();
    if (force_changed)
    {
        ForceChanged();
    }
}

void FancyPot::ForceChanged()
{
    force_change_flag_ = true;
}

bool FancyPot::HasChanged() const
{
    return has_changed_;
}

bool FancyPot::HasMoved(const Move how_much) const
{
    return has_pot_moved_[how_much];
}

void FancyPot::MidiCallback(midi::Message *msg)
{
    if (msg->IsControlChange())
    {
        // Reset on MIDI disconnect or reset command
        if (msg->GetData1() == cc::RESET_CONTROLLERS)
        {
            ClearMidi();
            return;
        }
        // Handle MIDI CC changes
        if (msg->GetData1() == config_.midi_cc)
        {
            values_[Source::MIDI_CC] = msg->GetData2() << 5; // Convert 7-bit to 12-bit
            if (config_.map_size > 0)
            {
                mapped_values_[Source::MIDI_CC] = map(msg->GetData2(), 0, 128, 0, config_.map_size);
            }
            value_source_ = Source::MIDI_CC;
        }
    }

    // Handle MIDI notes (if configured)
    if (msg->IsNoteOn() && config_.midi_note_control.IsEnabled())
    {
        uint8_t note = msg->GetData1();
        if (note >= config_.midi_note_control.start &&
            note < config_.midi_note_control.end)
        {
            // Calculate the offset note based on the start and repeat values
            uint8_t offset_note = (note - config_.midi_note_control.start) % config_.midi_note_control.repeat;

            // Map MIDI note to pot value
            values_[Source::MIDI_NOTES] = map(note, offset_note, config_.midi_note_control.repeat - 1, POT_MIN, POT_MAX);
            if (config_.map_size > 0 && offset_note < config_.map_size)
            {
                mapped_values_[Source::MIDI_NOTES] = offset_note; // Store mapped value
            }
            value_source_ = Source::MIDI_NOTES;
        }
    }
}

void FancyPot::LoadFromMemory()
{
    if (UseMemory())
    {
        uint8_t read_value = 0;
        if (Kastle2::memory.Read8(config_.memory_addr, &read_value))
        {
            values_[Source::INTERNAL] = mem_to_pot(read_value);
            UpdatePreviousFromCurrent();
            UpdateInternalMappedValue();
        }
    }
}

void FancyPot::SaveToMemory()
{
    if (UseMemory())
    {
        Kastle2::memory.QueueUpdate8(config_.memory_addr,
                                     pot_to_mem(values_[Source::INTERNAL]));
    }
}

}