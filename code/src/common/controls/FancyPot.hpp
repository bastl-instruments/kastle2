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
#include <limits>
#include "common/core/Hardware.hpp"
#include "common/core/midi/Message.hpp"
#include "common/dsp/utility/AutoFreeze.hpp"

namespace kastle2
{

/**
 * @class FancyPot
 * @ingroup controls
 * @brief High level abstraction over Hardware::Pot, includes MIDI support, freezing, memory saving/loading, and more.
 * @note You should most likely use this for all of your Potentiometer needs.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-05-29
 */

class FancyPot
{
public:
    static constexpr size_t NO_MEMORY = std::numeric_limits<size_t>::max();
    static constexpr uint8_t NO_MIDI = 0xFF;
    static constexpr uint32_t DEFAULT_LAYER_TIME = 64;                  // approx 64ms
    static constexpr uint32_t SHIFT_LAYER_TIME = kShiftShortPressTicks; // for shift layer
    static constexpr uint32_t TWEAK_THRESHOLD = 24;                     // 0.5%;
    static constexpr uint32_t MOVE_THRESHOLD = 409;                     // 10%
    static constexpr uint32_t NO_MAPPING = 0;                           // Don't map by default, use 0 for no mapping

    /**
     * @brief Can be INTERNAL (from the pot) or MIDI (from MIDI CCs)
     */
    enum class Source
    {
        INTERNAL,
        MIDI_CC,
        MIDI_NOTES,
        COUNT
    };

    /**
     * @brief How much the pot has moved, used for HasMoved() checks
     * TWEAK is a tiny wiggle, MOVE is a bigger change like a pot turn.
     */
    enum class Move
    {
        TWEAK, // Just a tiny wiggle
        MOVE,  // A bigger change, like a pot turn
        COUNT
    };

    /**
     * @brief MIDI note control, used for mapping MIDI notes to pot values (and mapped values to get precise values)
     * @note Use NO_MIDI (0xFF) for no MIDI notes.
     */
    struct MidiNoteControl
    {
        uint8_t start = NO_MIDI; // Start note for MIDI note control (0xFF = no MIDI notes)
        uint8_t repeat = 255;    // Virtually no repeat
        uint8_t end = NO_MIDI;   // Excluding the end note (0xFF = no MIDI notes)

        /**
         * @brief Checks if the MIDI note control is valid
         * @return True if valid, false otherwise
         */
        bool IsEnabled() const
        {
            return start != NO_MIDI && end != NO_MIDI && start < end;
        }
    };

    /**
     * @brief Configuration for the FancyPot
     * @note Use Create() to create a FancyPot instance with this configuration.
     */
    struct Config
    {
        Hardware::Pot pot = Hardware::Pot::POT_1;        // Potentiometer to read
        Hardware::Layer layer = Hardware::Layer::NORMAL; // Layer to read the pot from
        int32_t initial_value = POT_HALF;                // Usually POT_HALF
        size_t map_size = NO_MAPPING;               // Use GetMappedValue() to get the value in range 0 to map_size - 1
        uint8_t midi_cc = NO_MIDI;                       // For MIDI control change (0xFF = no MIDI CC)
        uint8_t midi_output_cc = NO_MIDI;                // For MIDI control change output (0xFF = no MIDI CC)
        MidiNoteControl midi_note_control = {};          // MIDI note control, use NO_MIDI (0xFF) for no MIDI notes
        bool deadzone = false;                           // Center deadzone
        bool freeze = false;                             // Freezes the value after a while, useful for getting rid of noise etc.
        size_t memory_addr = NO_MEMORY;             // For saving and loading (0 = no memory)
    };

    /**
     * @brief Creates a FancyPot instance with the given configuration
     * @param config The configuration for the FancyPot
     * @return A unique pointer to the created FancyPot instance
     */
    static std::unique_ptr<FancyPot> Create(Config config);

    /**
     * @brief Constructor for FancyPot, usually called from Create()
     * @param config The configuration for the FancyPot
     * @note Most likely you should use Create() instead of this constructor directly.
     */
    FancyPot(Config config);

    /**
     * @brief Initializes the FancyPot with the given sample rate
     * @param sample_rate The sample rate to use for freezing
     */
    void Init(const float sample_rate);

    /**
     * @brief Processes the FancyPot freezing, should be called every audio loop
     * @note Call in each audio loop!
     */
    void Process();

    /**
     * @brief Reads the current value
     * @note Call this at the beginning of your UI loop!
     */
    void ReadValue();

    /**
     * @brief Forces the current value to be set into all value sources.
     * @param value The value to force
     * @param force_changed Whether to force the change flag to be set
     * @warning Be careful with this! You shouldn't normally use it.
     */
    void ForceValue(const int32_t value, const bool force_changed = false);

    /**
     * @brief Handles incoming MIDI message (= parses CCs from it)
     * @param msg The MIDI message to handle
     */
    void MidiCallback(midi::Message *msg);

    /**
     * @brief Clears the MIDI value and resets the source to INTERNAL
     */
    void ClearMidi();

    /**
     * @brief Returns the current value in range `POT_MIN` to `POT_MAX`
     * @return The current value in range `POT_MIN` to `POT_MAX`
     */
    int32_t GetValue() const;

    /**
     * @brief Returns the mapped value in range `0` to `map_size - 1`
     * @note Make sure to set `map_size` in the Config, otherwise it will return `0`.
     * @return The mapped value in range `0` to `map_size - 1`
     */
    int32_t GetMappedValue() const;

    /**
     * @brief Returns the current source of the value (INTERNAL or MIDI)
     * @return The current source of the value
     */
    Source GetSource() const
    {
        return value_source_;
    }

    /**
     * @brief Returns the defined HW Pot
     * @return HW Pot
     */
    Hardware::Pot GetPot() const
    {
        return config_.pot;
    }

    /**
     * @brief Whether the actual Potentiometer moved (ignores MIDI changes)
     * @param how_much How much the pot has moved
     * @return Pot actually moved.
     * @note This value is reset at each ReadValue() call.
     */
    bool HasMoved(const Move how_much = Move::TWEAK) const;

    /**
     * @brief Whether the value has changed, either from MIDI (any change) or actual Potentiometer (over a tweak threshold)
     * @return The value changed.
     * @note This value is reset at each ReadValue() call.
     */
    bool HasChanged() const;

    /**
     * @brief Forces the change flag to be set.
     * @note Works for the next ReadValue() call.
     * @note This is useful when you want to force the app update even if the value didn't change.
     */
    void ForceChanged();

    /**
     * @brief Returns the layer this pot is configured for
     * @return The layer this pot is configured for
     */
    Hardware::Layer GetLayer() const
    {
        return config_.layer;
    }

    /**
     * @brief Saves the current value to memory (if configured)
     */
    void SaveToMemory();

    /**
     * @brief Loads the value from memory (if configured)
     */
    void LoadFromMemory();

private:
    /**
     * @brief Updates the previous values from the current values
     */
    inline void UpdatePreviousFromCurrent();

    /**
     * @brief Using memory for this pot?
     * @return True if memory is used, false otherwise
     */
    inline bool UseMemory() const
    {
        return config_.memory_addr != NO_MEMORY;
    }

    /**
     * @brief How much we should stay in the current layer for the pot to be allowed to read values?
     * @return The time for the current layer in `sample_rate` ticks as defined in `Init()`
     */
    inline size_t GetLayerTime() const
    {
        return (config_.layer == Hardware::Layer::SHIFT) ? SHIFT_LAYER_TIME : DEFAULT_LAYER_TIME;
    }

    /**
     * @brief Updates the mapped value from the internal value
     */
    void UpdateInternalMappedValue();

    // Configuration
    Config config_;

    // Change flags
    bool force_change_flag_ = false;
    bool has_changed_ = false;

    // Potentiometer movement flags and thresholds
    EnumArray<Move, bool> has_pot_moved_;
    EnumArray<Move, uint32_t> move_thresholds_;

    // Values
    Source value_source_ = Source::INTERNAL;
    EnumArray<Source, int32_t> values_;
    EnumArray<Source, int32_t> mapped_values_;
    EnumArray<Move, int32_t> prev_pot_values_;
    int32_t prev_midi_cc_value_ = INT32_MAX;
    int32_t prev_midi_notes_value_ = INT32_MAX;
    int32_t prev_midi_sticky_value_ = INT32_MAX;

    int32_t prev_internal_map_sticky_value_ = INT32_MAX;

    // Freezing after some time
    AutoFreeze<int32_t> freezer_;
};

}
