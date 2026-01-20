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

#include "common/controls/FancyMode.hpp"
#include "common/core/Hardware.hpp"
#include "common/core/Kastle2.hpp"
#include "common/core/Kastle2_cc.hpp"
#include "common/utils.hpp"

namespace kastle2
{

FancyMode::FancyMode(Config config) : config_(config)
{
}

void FancyMode::Init(uint32_t modes_count)
{
    attenuator_ = FancyPot::Create({
        .pot = config_.attenuator_pot,         ///< Center pot
        .layer = Hardware::Layer::MODE,        ///< When holding MODE button
        .initial_value = POT_MAX,              ///< No attenuation by default
        .midi_cc = config_.midi_attenuator_cc, ///< MIDI CC for attenuator control
    });
    attenuator_->Init(AUDIO_LOOP_RATE);
    value_source_ = Source::INTERNAL; // Default source is internal
    disable_next_change_ = false;     // Allow mode changes by default
    adc_input_value_ = 0;             // Initialize ADC input value
    SetModesCount(modes_count);
    LoadFromMemory();
    RecalculateMode();
}

void FancyMode::Process()
{
    attenuator_->Process();
}

void FancyMode::SetModesCount(uint32_t count)
{
    if (count > 0)
    {
        config_.modes_count = count;
    }
}

void FancyMode::MidiCallback(midi::Message *msg)
{
    attenuator_->MidiCallback(msg);
    if (msg->IsControlChange())
    {
        if (msg->GetData1() == config_.midi_cc)
        {
            cc_mode_received_ = msg->GetData2();
            value_source_ = Source::MIDI_CC; // Set source to MIDI
        }
        else if (msg->GetData1() == cc::RESET_CONTROLLERS)
        {
            value_source_ = Source::INTERNAL; // Reset to internal source
        }
    }
    if (msg->IsNoteOn() && config_.midi_note_control.IsEnabled())
    {
        uint8_t note = msg->GetData1();
        if (note >= config_.midi_note_control.start &&
            note < config_.midi_note_control.end)
        {
            note = (note - config_.midi_note_control.start) % config_.midi_note_control.repeat;
            if (note < config_.modes_count)
            {
                notes_mode_ = note;
                value_source_ = Source::MIDI_NOTES;
                RecalculateMode();
            }
        }
    }
}

void FancyMode::ReadValue()
{
    attenuator_->ReadValue();
    Hardware::Layer current_layer = Kastle2::hw.GetLayer();

    // NORMAL LAYER - normal switching
    if (current_layer == Hardware::Layer::NORMAL)
    {
        if (Kastle2::hw.JustReleased(Hardware::Button::MODE))
        {
            if (!disable_next_change_)
            {
                Next();
            }
            disable_next_change_ = false;
        }
    }

    // MODE LAYER - backward switching
    if (current_layer == Hardware::Layer::MODE)
    {
        if (Kastle2::hw.JustPressed(Hardware::Button::SHIFT))
        {
            Prev();
            disable_next_change_ = true;
        }
    }

    // If it's shift or normal layer, force enable the bank button
    if (current_layer == Hardware::Layer::NORMAL ||
        current_layer == Hardware::Layer::SHIFT)
    {
        if (!Kastle2::hw.Pressed(Hardware::Button::MODE))
        {
            disable_next_change_ = false;
        }
    }

    // Calculate the mode based on the current inputs
    RecalculateMode();

    // When switching back from the settings, disable next change
    // This is to prevent accidental mode changes when returning from settings
    if (Kastle2::hw.GetLayer() != Kastle2::base.GetPrevLayer())
    {
        if (Kastle2::base.GetPrevLayer() == Hardware::Layer::SETTINGS)
        {
            DisableNextChange();
        }
    }

    // Disable bank change also when...
    if (!disable_next_change_ &&
        Kastle2::hw.GetLayer() == Hardware::Layer::MODE)
    {
        // When over timeout
        if (over_ticks_ > 0 && Kastle2::base.GetLayerTimer() > over_ticks_)
        {
            DisableNextChange();
        }

        // When any of the pots has moved
        // This is to prevent accidental mode changes when tweaking pots
        for (const auto &pot : tweak_pots_)
        {
            if (pot->HasMoved(FancyPot::Move::TWEAK))
            {
                DisableNextChange();
                break;
            }
        }
    }
}

void FancyMode::Next()
{
    if (config_.modes_count == 0)
    {
        return; // No modes available
    }
    value_source_ = Source::INTERNAL; // Reset to internal source
    selected_mode_ = (selected_mode_ + 1) % config_.modes_count;
    RecalculateMode();
    SendMidi();
    SaveToMemory();
}

void FancyMode::Prev()
{
    if (config_.modes_count == 0)
    {
        return; // No modes available
    }
    value_source_ = Source::INTERNAL; // Reset to internal source
    selected_mode_ = (config_.modes_count + selected_mode_ - 1) % config_.modes_count;
    RecalculateMode();
    SendMidi();
    SaveToMemory();
}

void FancyMode::SendMidi(bool force)
{
    if (config_.midi_output_cc != NO_MIDI)
    {
        uint8_t output_value = map(mode_, 0, config_.modes_count - 1, 0, 127);
        Kastle2::midi.SendCc(config_.midi_output_cc, output_value, force);
    }
}

uint32_t FancyMode::GetMode() const
{
    return mode_;
}

void FancyMode::TriggerAdcRead()
{
    if (config_.input_reading == InputReading::NONE)
    {
        return;
    }
    adc_input_value_ = apply_pot_mod(Kastle2::hw.GetAnalogValue(config_.adc_input), attenuator_->GetValue());
}

void FancyMode::TriggerMidiRead()
{
    if (config_.midi_input_reading == InputReading::NONE)
    {
        return;
    }
    cc_mode_ = cc_mode_received_;
}

void FancyMode::RecalculateMode()
{
    if (config_.input_reading == InputReading::CONTINUOUS)
    {
        TriggerAdcRead(); // Read ADC input continuously
    }
    if (config_.midi_input_reading == InputReading::CONTINUOUS)
    {
        cc_mode_ = cc_mode_received_; // Update the CC mode continuously
    }
    uint32_t input_value = adc_input_value_;
    uint32_t m = 0;
    switch (value_source_)
    {
    case Source::INTERNAL:
        m = selected_mode_;
        break;
    case Source::MIDI_CC:
        m = map(cc_mode_, 0, 127 + 1, 0, config_.modes_count);
        break;
    case Source::MIDI_NOTES:
        m = notes_mode_;
        break;
    };
    mode_ = (m + map(input_value, 0, ADC_5V + 1, 0, config_.modes_count)) % config_.modes_count;
}

void FancyMode::DisableNextChange()
{
    disable_next_change_ = true;
}

void FancyMode::LoadFromMemory()
{
    if (config_.memory_addr != NO_MEMORY)
    {
        uint8_t read_value = 0;
        if (Kastle2::memory.Read8(config_.memory_addr, &read_value))
        {
            if (read_value < config_.modes_count)
            {
                selected_mode_ = read_value;
            }
            else
            {
                selected_mode_ = 0; // Reset to first mode if read value is out of bounds
            }
        }
    }
}

void FancyMode::SaveToMemory()
{
    if (config_.memory_addr != NO_MEMORY)
    {
        Kastle2::memory.Write8(config_.memory_addr, selected_mode_);
    }
}

}
