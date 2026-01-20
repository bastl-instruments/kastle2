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

#include "common/EnumTools.hpp"
#include "common/core/clocks/ClockSource.hpp"
#include "common/core/midi/Message.hpp"
#include "common/dsp/math/RunningAverage.hpp"
#include "common/dsp/utility/EdgeDetector.hpp"

namespace kastle2
{

/**
 * @class Clock
 * @ingroup core
 * @brief Wrapper for different clock implementations, handles switching between them and provides a unified interface.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-05-21
 */

class Clock
{

public:
    /**
     * @brief Various clock sources with their implementations.
     * @note The order is the priority of the clocks (MIDI > EXTERNAL > INTERNAL)
     */
    enum class Sync
    {
        MIDI,     ///< kastle2::MidiClockSource
        EXTERNAL, ///< kastle2::ExternalClockSource
        INTERNAL, ///< kastle2::InternalClockSource
        COUNT,
    };

    /**
     * @brief Tap tempo state handling.
     */
    enum class TapState
    {
        NONE,                   ///< Has not been tapped yet (or timeouted)
        WAITING_FOR_SECOND_TAP, ///< User has tapped once and is waiting for the second tap
        ACTIVE,                 ///< User is tapping the tempo
    };

    /**
     * @brief We kinda misuse the FancyPot when switching states - need to handle it properly.
     */
    enum class PotState
    {
        UNDEFINED,          ///< Pot value is not set yet
        REQUIRES_THRESHOLD, ///< Pot is frozen
        ACTIVE              ///< Pot can be moved freely
    };

    /**
     * @brief Initializes the clock with the given sample rate (usually the audio loop rate).
     * @param sample_rate The audio loop sample rate.
     */
    void Init(float sample_rate);

    /**
     * @brief Gets the target ticks of the clock.
     * @return The target ticks of the clock in `sample_rate` ticks.
     */
    inline uint32_t GetTargetTicks() const
    {
        return clocks_[sync_type_]->GetTargetTicks();
    }

    /**
     * @brief Gets the averaged target ticks of the clock - hides the noise.
     * @return The averaged target ticks of the clock in `sample_rate` ticks.
     */
    inline uint32_t GetMedianTargetTicks() const
    {
        if (!avg_target_ticks_.IsEmpty())
        {
            return avg_target_ticks_.GetMedian();
        }
        return GetTargetTicks();
    }

    /**
     * @brief Gets the averaged target ticks of the clock - hides the noise.
     * @return The averaged target ticks of the clock in `sample_rate` ticks.
     */
    inline uint32_t GetAverageTargetTicks() const
    {
        if (!avg_target_ticks_.IsEmpty())
        {
            return avg_target_ticks_.GetAverage();
        }
        return GetTargetTicks();
    }

    /**
     * @brief Gets the current ticks of the clock.
     * @return The current ticks of the clock in `sample_rate` ticks.
     */
    inline uint32_t GetCurrentTicks() const
    {
        return clocks_[sync_type_]->GetCurrentTicks();
    }

    /**
     * @brief Returns true a tiny bit before the next cycle. Necessary for the sequencer to update CV slightly before the next cycle.
     * @return True is returned few ms before the nexct clock.
     */
    inline bool IsReachingNextCycle() const
    {
        return clocks_[sync_type_]->IsReachingNextCycle();
    }

    /**
     * @brief Has been the clock just reset? Use it in audio loop, the value is cleared each Process().
     * @return Use the return value for sequencer resetting etc.
     */
    inline bool IsNowReset() const
    {
        return clocks_[sync_type_]->IsNowReset() || now_reset_;
    }

    /**
     * @brief Running, stopped. Shouldn't be UNAVAILABLE since internal clock is always available.
     * @return The current state of the clock.
     */
    inline ClockSource::State GetState() const
    {
        return clocks_[sync_type_]->GetState();
    }

    /**
     * @brief Stops the clock
     */
    inline void Stop()
    {
        clocks_[sync_type_]->Stop();
    }

    /**
     * @brief Starts the clock and resets the counters (will start on beat)
     */
    inline void Start()
    {
        clocks_[sync_type_]->Start();
    }

    /**
     * @brief Starts the clock without resetting counters (will not start on beat)
     */
    inline void Resume()
    {
        clocks_[sync_type_]->Resume();
    }

    /**
     * @brief Should clock output out be enabled?
     * @return False when tapping clock, true otherwise.
     * @note This is used to disable the clock output when tapping the tempo.
     */
    bool IsOutputEnabled() const;

    /**
     * @brief Clock just reached the next cycle.
     * @return True if the clock just reached the next cycle.
     * @note Use this in the audio loop.
     */
    bool IsNowTrigger() const;

    /**
     * @brief Gets the sync out jack (and patchbay) output state.
     * @return Short pulse for sync out jack and patchbay.
     */
    bool GetSyncOutput() const;

    /**
     * @brief Gets the 50% duty output state (eg. for LED indicator).
     * @return 50% duty square wave.
     */
    bool GetHalfDutyOutput() const;

    /**
     * @brief Clears the tap tempo values and resets the tap state.
     */
    void ClearTaps();

    /**
     * @brief Sets the analog sync in jack plugged state.
     * @param jack_plugged Passes the value to all clocks.
     */
    void SetSyncJackPlugged(bool jack_plugged);

    /**
     * @brief The main processing function for the clock.
     * @param raw_tap_input Raw tap input (handled by this class).
     * @param raw_sync_input Raw sync input (passed to the clock implementation).
     * @param midi_pulse_input Incoming MIDI pulse input (passed to the clock implementation).
     */
    bool Process(bool raw_tap_input, bool raw_sync_input, bool midi_pulse_input);

    /**
     * @brief Checks if the MIDI source is either same as the current one or not set yet.
     * @param source The MIDI source to check.
     * @return True if the source matches or is not set yet, false otherwise.
     */
    bool CheckMidiSource(midi::Message::Source source);

    /**
     * @brief Sets the raw potentiometer value.
     * @param pot_value Passed to the clock implementations.
     */
    void SetPot(int32_t pot_value);

    /**
     * @brief Which clock source is currently used.
     * @return The current sync type.
     */
    inline Sync GetSyncType() const
    {
        return sync_type_;
    }

    /**
     * @brief State of the clock between 0-100% based on the current and target ticks.
     * @return Percentage of the current clock state (0-100).
     */
    inline uint32_t GetPercentageState() const
    {
        return (GetCurrentTicks() * 100) / GetTargetTicks();
    }

    /**
     * @brief Sends a reset signal over USB MIDI.
     */
    void NextCycleReset();

    /**
     * @brief Loads the clock state from memory.
     * @note This is usually called on startup to restore the clock state.
     */
    void LoadFromMemory();

    /**
     * @brief Saves the current state of the clock to memory.
     */
    void SaveToMemory();

private:
    /**
     * @brief Sets the sync type and resets the average target ticks. Internal, `Clock` does the switching automatically.
     * @param sync_type The new sync type to set.
     */
    void SetSyncType(Sync sync_type);

    /**
     * @brief Processes the tap tempo logic.
     * @param raw_tap_input The raw tap input (handled by this class).
     * @note The tapping is "multiplied" by `kTapTempoMultiplier` - runs x-times faster than the tapping.
     */
    void ProcessTapTempo(bool raw_tap_input);

    void HandleMidiOutClock();

    // Shared stuff
    bool half_duty_state_ = false;
    bool sync_out_state_ = false;

    // Pot change detection
    PotState pot_state_ = PotState::UNDEFINED;
    int32_t prev_pot_value_ = 0;

    // Clocks
    Sync sync_type_ = Sync::COUNT; ///< Default is COUNT, will be set in Init()
    EnumArray<Sync, std::unique_ptr<ClockSource>> clocks_;

    // Reset state (for sequencer aligning etc.)
    bool now_reset_ = false;

    // Averaging (for delays etc)
    RunningAverage<uint32_t> avg_target_ticks_ = RunningAverage<uint32_t>(8);

    // Tap tempo stuff
    TapState tap_state_ = TapState::NONE;
    uint32_t tap_ticks_ = 0;
    EdgeDetector tap_edge_ = EdgeDetector(EdgeDetector::Type::RISING);
    RunningAverage<uint32_t> tap_tempo_values_ = RunningAverage<uint32_t>(8);

    static constexpr int32_t kPotChangeThreshold = 32; ///< The threshold for pot change detection.

    // Midi source stuff
    midi::Message::Source midi_source_ = midi::Message::Source::NONE;

    // Midi out stuff
    static constexpr size_t kOutputMidiPulseMultiplier = 6;
    size_t midi_pulses_sent_ = 0;
    bool next_cycle_reset_ = false;
    ClockSource::State midi_prev_clock_state_ = ClockSource::State::UNAVAILABLE;
    Sync prev_sync_type_ = Sync::INTERNAL;
};

}
