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

namespace kastle2
{

/**
 * @class ClockSource
 * @ingroup core_clocks
 * @brief Interface for different clock implementations. Doesn't contain any logic.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-05-21
 */

class ClockSource
{
public:
    /**
     * @brief For switching between different clock implementations.
     */
    enum class State
    {
        UNAVAILABLE, // Usually on startup
        RUNNING,     // When first incoming pulse is received
        STOPPED,     // When not running, but we want to keep this as the source
    };

    /**
     * @brief Initializes the clock source with the given sample rate.
     * @param sample_rate Usually the audio loop rate.
     */
    virtual void Init(float sample_rate) = 0;

    /**
     * @brief Starts the clock and resets the dividers etc. (will start on beat)
     */
    virtual void Start() = 0;

    /**
     * @brief Stops the clock.
     */
    virtual void Stop() = 0;

    /**
     * @brief Resumes the clock without any divider resetting etc. (will not start on beat)
     */
    virtual void Resume() = 0;

    /**
     * @brief Has been the clock just reset? Use it in audio loop, the value is cleared each Process().
     * @return Use the return value for sequencer resetting etc.
     */
    virtual bool IsNowReset() const = 0;

    /**
     * @brief Set the pot value. The clock itself will decide what to do with it based on the current configuration.
     * @param pot_value The pot value to set (0-4095).
     */
    virtual void SetPot(int32_t pot_value) = 0;

    /**
     * @brief Sets the analog sync in jack plugged state.
     * @param jack_plugged If you don't use it in the code, just ignore it and use `[[maybe_unused]]` attribute.
     */
    virtual void SetSyncJackPlugged(bool jack_plugged) = 0;

    /**
     * @brief Processes the clock source. Should be called in each audio loop.
     * @param raw_sync_input Raw sync input (the implementation needs to handle the edge detection).
     * @param midi_pulse_input Incoming MIDI pulse input from the MIDI clock.
     */
    virtual void Process(bool raw_sync_input, bool midi_pulse_input) = 0;

    /**
     * @brief Sets the desired tempo in ticks per cycle.
     * @param tap_ticks Desired tempo in `sample_rate` ticks (how many times it should tick per cycle).
     */
    virtual void SetTapTicks(uint32_t tap_ticks) = 0;

    /**
     * @brief Gets the target ticks of the clock.
     * @return The target ticks of the clock in `sample_rate` ticks.
     */
    virtual uint32_t GetTargetTicks() const = 0;

    /**
     * @brief Gets the current ticks of the clock.
     * @return The current ticks of the clock in `sample_rate` ticks.
     */
    virtual uint32_t GetCurrentTicks() const = 0;

    /**
     * @brief Saves the current state of the clock to memory.
     */
    virtual void SaveToMemory() = 0;

    /**
     * @brief Loads the clock state from memory.
     * @note This is usually called on startup to restore the clock state.
     */
    virtual void LoadFromMemory() = 0;

    /**
     * @brief Returns true a tiny bit before the next cycle. Necessary for the sequencer to update CV slightly before the next cycle.
     * @return True is returned few ms before the nexct clock.
     */
    virtual bool IsReachingNextCycle() const = 0;

    /**
     * @brief Clock state for switching between different clock implementations.
     * @return The current state of the clock.
     */
    virtual State GetState() const = 0;

    /**
     * @brief Should be called when the tap button is pressed - to reset the ticks if necessary.
     */
    virtual void TapResetsTicks(bool force) = 0;

    /**
     * @brief Returns the total number of steps since clock started (or last reset was made). Used for sequencer realignment.
     * @return The total number of steps.
     */
    virtual uint32_t GetTotalSteps() = 0;
};

}
