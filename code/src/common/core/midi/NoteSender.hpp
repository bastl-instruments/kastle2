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

#include "common/core/Kastle2.hpp"

namespace kastle2::midi
{

/**
 * @class NoteSender
 * @ingroup core_midi
 * @brief Sends MIDI note on/off messages based on a duration - `SetDuration`
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-06-12
 */
class NoteSender
{

public:
    /**
     * @brief Initializes the NoteSender with a sample rate.
     * @param sample_rate The sample rate in Hz.
     */
    void Init(float sample_rate)
    {
        sample_rate_ = sample_rate;
        SetDuration(0.5f);
    }

    /**
     * @brief Triggers a MIDI note on message.
     * @param note The MIDI note number to send.
     * @param velocity The velocity of the note (default is 127).
     * @note This will clear any previously sent note.
     */
    void Trigger(uint8_t note, uint8_t velocity = 127)
    {
        Clear();
        note_active_ = true;
        note_sent_ = note;
        counter_ = 0;
        Kastle2::midi.SendNoteOn(note_sent_, velocity);
    }

    /**
     * @brief Clears the currently active note.
     * @note Sends a MIDI note off message if a note is currently active.
     */
    void Clear()
    {
        if (note_active_)
        {
            Kastle2::midi.SendNoteOff(note_sent_, 127);
            note_active_ = false;
        }
    }

    /**
     * @brief Sets the duration of the note in seconds.
     * @param seconds The duration in seconds.
     * @note The duration is converted to samples based on the sample rate.
     */
    void SetDuration(const float seconds)
    {
        SetDuration(static_cast<size_t>(sample_rate_ * seconds));
    }

    /**
     * @brief Sets the duration of the note in samples.
     * @param samples The duration in samples.
     */
    void SetDuration(const size_t samples)
    {
        duration_samples_ = samples;
        if (duration_samples_ == 0)
        {
            duration_samples_ = 1; // Ensure at least one sample
        }
    }

    /**
     * @brief Processes the note sender.
     * @note This should be called in the audio processing loop.
     */
    void Process()
    {
        if (!note_active_)
        {
            return;
        }
        if (counter_ < duration_samples_)
        {
            counter_++;
        }
        else
        {
            Clear();
        }
    }

    /**
     * @brief Checks if a note is currently active.
     * @return True if a note is active, false otherwise.
     */
    bool IsNoteActive() const
    {
        return note_active_;
    }

private:
    bool note_active_ = false;
    uint8_t note_sent_ = 0;
    float sample_rate_ = 0;
    size_t duration_samples_ = 0;
    size_t counter_ = 0;
};

}
