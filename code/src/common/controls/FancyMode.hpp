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
#include <vector>
#include "common/core/Hardware.hpp"
#include "common/core/Kastle2.hpp"
#include "common/core/Kastle2_cc.hpp"
#include "common/utils.hpp"
#include "FancyPot.hpp"

namespace kastle2
{

/**
 * @class FancyMode
 * @ingroup controls
 * @brief High-level abstraction for mode switching, including MIDI support, memory saving/loading, CV with attenuating and more.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-05-29
 */
class FancyMode
{
public:
    static constexpr uint32_t NO_MEMORY = 0xFFFFFFFF; ///< No memory address set, used for default initialization
    static constexpr uint32_t NO_MIDI = 0xFF;         ///< No MIDI used

    /**
     * @enum Source
     * @brief Represents the source of the mode value (internal or MIDI).
     */
    enum class Source
    {
        INTERNAL,   ///< Internal source (from the button)
        MIDI_CC,    ///< MIDI source (from MIDI CCs)
        MIDI_NOTES, ///< MIDI source (from MIDI notes)
        COUNT       ///< Count of sources
    };

    /**
     * @enum InputReading
     * @brief Represents the reading mode of the ADC input.
     */
    enum class InputReading
    {
        NONE,       ///< Ignore mode ADC input
        CONTINUOUS, ///< Read mode ADC input continuously
        TRIGGERED   ///< Read mode ADC input only when requested
    };

    /**
     * @struct Config
     * @brief Configuration for the FancyMode.
     */
    struct Config
    {
        uint32_t memory_addr = NO_MEMORY;                              ///< Memory address to save the mode to
        uint32_t modes_count = 0;                                      ///< Number of modes available
        uint8_t midi_cc = NO_MIDI;                                     ///< MIDI CC for external mode control, 0xFF = no MIDI (default)
        uint8_t midi_attenuator_cc = NO_MIDI;                          ///< MIDI CC for attenuator control, 0xFF = no MIDI (default)
        uint8_t midi_output_cc = NO_MIDI;                              ///< Outputs the value mapped to 0-127
        FancyPot::MidiNoteControl midi_note_control = {};              ///< MIDI note control
        Hardware::AnalogInput adc_input = Hardware::AnalogInput::MODE; ///< ADC input to read the mode from
        Hardware::Pot attenuator_pot = Hardware::Pot::POT_4;           ///< Potentiometer to use as an attenuator for the mode input
        InputReading input_reading = InputReading::CONTINUOUS;         ///< How to read the cv mode input
        InputReading midi_input_reading = InputReading::CONTINUOUS;    ///< How to read the midi mode input
    };

    /**
     * @brief Constructor for FancyMode.
     * @param config The configuration for the FancyMode.
     */
    FancyMode(Config config);

    /**
     * @brief Initializes the FancyMode with the given number of modes.
     * @param modes_count The number of modes available. If 0, the value is taken from config.
     */
    void Init(uint32_t modes_count = 0);

    /**
     * @brief Gets the current config
     * @return The current configuration of the FancyMode.
     */
    Config GetConfig() const
    {
        return config_;
    }

    /**
     * @brief Sets the configuration for the FancyMode.
     * @param config The new configuration to set.
     */
    void SetConfig(const Config &config)
    {
        config_ = config;
    }

    /**
     * @brief If `input_reading` is set to `TRIGGERED`, you need to call this method to read the ADC input.
     */
    void TriggerAdcRead();

    /**
     * @brief If `midi_input_reading` is set to `TRIGGERED`, you need to call this method to read the MIDI input.
     */
    void TriggerMidiRead();

    /**
     * @brief Processes the FancyMode logic, should be called every audio loop.
     */
    void Process();

    /**
     * @brief Sets the number of modes available.
     * @param count The number of modes.
     */
    void SetModesCount(uint32_t count);

    /**
     * @brief Handles incoming MIDI messages.
     * @param msg The MIDI message to handle.
     */
    void MidiCallback(midi::Message *msg);

    /**
     * @brief Reads the current value and updates the mode based on inputs.
     */
    void ReadValue();

    /**
     * @brief Switches to the next mode.
     */
    void Next();

    /**
     * @brief Switches to the previous mode.
     */
    void Prev();

    /**
     * @brief Gets the current mode.
     * @return The current mode index.
     */
    uint32_t GetMode() const;

    /**
     * @brief Recalculates the mode based on inputs and selected mode.
     */
    void RecalculateMode();

    /**
     * @brief Disables the next mode change (eg. when holding MODE button and tweaking values).
     */
    void DisableNextChange();

    /**
     * @brief Disables the next change when any MODE layer pot is moved, or when the current layer time is over the specified number of ticks.
     * @tparam Container Type of container holding FancyPot pointers (e.g., EnumArray)
     * @param all_pots Container with all pots the app uses (the function filters them automatically by the MODE layer)
     * @param over_ticks If set, disables the next change when the current layer time is over this number of ticks
     */
    template <typename Container>
    void DisableNextChangeWhen(const Container &all_pots, uint32_t over_ticks = 0)
    {
        tweak_pots_.clear();
        for (const auto &pot : all_pots)
        {
            if (pot && pot->GetLayer() == Hardware::Layer::MODE)
            {
                tweak_pots_.push_back(pot.get());
            }
        }
        over_ticks_ = over_ticks;
    }

    /**
     * @brief Loads the mode from memory.
     */
    void LoadFromMemory();

    /**
     * @brief Saves the current mode to memory.
     */
    void SaveToMemory();

    /**
     * @brief Sends the current mode as a MIDI CC message mapped to 0-127.
     * @param force If true, sends the MIDI CC even if the value hasn't changed.
     */
    void SendMidi(bool force = false);

private:
    Config config_;                          ///< Configuration for the FancyMode
    Source value_source_ = Source::INTERNAL; ///< Current source of the value (INTERNAL or MIDI)
    uint32_t selected_mode_ = 0;             ///< Current mode index
    uint32_t cc_mode_received_ = 0;          ///< Last received MIDI CC value
    uint32_t cc_mode_ = 0;                   ///< MIDI CC, used for external control
    uint32_t notes_mode_ = 0;                ///< MIDI note, used for external control

    std::unique_ptr<FancyPot> attenuator_; ///< Attenuator for mode control

    uint32_t mode_ = 0;                ///< Output mode, generated based on inputs and selected mode
    bool disable_next_change_ = false; ///< Whether to disable the next change (when changing mode pots etc)

    std::vector<FancyPot *> tweak_pots_; ///< If defined we will disable next change when the pots are moved
    uint32_t over_ticks_ = 0;            ///< If defined we will disable next change when the current layer time is over this number of ticks

    uint32_t adc_input_value_ = 0; ///< Current input value from the ADC, used for mode calculation
};

}
