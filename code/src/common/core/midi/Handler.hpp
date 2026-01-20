/*
MIT License

Copyright (c) 2025 Marek Mach (Bastl Instruments)
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

#include <array>
#include <cstddef>
#include <cstdint>
#include "common/dsp/utility/RingBuffer.hpp"
#include "Message.hpp"
#include "tusb.h"

// Set this number to limit how often the same CC can be sent (in ms)
// Set to 0 to disable rate limiting
#define MIDI_CC_RATE_LIMIT 2

namespace kastle2
{

namespace midi
{
/**
 * @class Handler
 * @ingroup core_midi
 * @brief Manages all MIDI communications and parsing. Merges messages from UART and USB MIDI together.
 * @author Marek Mach (Bastl Instruments), Vaclav Mach (Bastl Instruments)
 * @date 2025-03-18
 * @note TinyUSB tud_init() and tud_task() are called in `Kastle2` class since they are used for multiple things.
 */
class Handler
{
public:
    /**
     * @brief Type definition for the callback function
     * @param midi_message Pointer to the received MIDI message
     */
    typedef void (*Callback)(Message *midi_message);

    /**
     * @brief MIDI learning states
     */
    enum class LearnState
    {
        IDLE,
        LEARNING,
        LEARNED
    };

    /**
     * @brief Initializer for the MIDI class, sets up UART and (if enabled) USB interfaces
     */
    void Init();

    /**
     * @brief Listens for and processes the incoming MIDI messages
     * @details This function is called in the main loop to process incoming MIDI messages.
     *          It handles both USB and UART MIDI messages.
     */
    void Process();

    /**
     * @brief Sets the address of the Base-wide MIDI callback function
     * @param callback Pointer to the callback function
     * @details The callback function is called when a MIDI message is received.
     */
    void SetBaseCallback(Callback callback);

    /**
     * @brief Sets the address of the application-wide MIDI callback function
     * @param callback Pointer to the callback function
     * @details The callback function is called when a MIDI message is received.
     */
    void SetAppCallback(Callback callback);

    /**
     * @brief Reports that the MIDI interface is disconnected using int
     */
    void ReportDisconnected();

    /**
     * @brief Starts listening for MIDI messages, stores the channel to listen to
     */
    void StartLearning();

    /**
     * @brief Returns the state of the MIDI learning process
     * @return Special enum value indicating the learning state
     */
    LearnState GetLearnState() const
    {
        if (learning_)
        {
            if (learning_channel_ != Message::kAllChannels)
            {
                return LearnState::LEARNED;
            }
            return LearnState::LEARNING;
        }
        return LearnState::IDLE;
    }

    /**
     * @brief Stops listening for MIDI messages
     * @return Learning was success
     */
    bool StopLearning();

    /**
     * @brief Loads the MIDI channel from the EEPROM
     */
    void LoadFromMemory();

    /**
     * @brief Saves the MIDI channel to the EEPROM
     */
    void SaveToMemory();

    /**
     * @brief Returns the currently set MIDI channel
     * @return The currently set MIDI channel
     */
    size_t GetChannel() const
    {
        return channel_;
    }

    /**
     * @brief Sets the MIDI channel to listen to and saves it to the EEPROM
     * @param channel The MIDI channel to set (0-15)
     */
    bool SetChannel(size_t channel);

    /**
     * @brief Writes a MIDI message to the output buffer
     * @param midi_message Pointer to the MIDI message to output
     * @note The message is not sent immediately, it is queued for sending in the Process() function.
     * @return True if the message was added to the send queue
     */
    bool Send(Message *midi_message);

    /**
     * @brief Sends a MIDI CC message
     * @param controller The controller number (0-127)
     * @param value The value of the controller (0-127)
     * @param force If true, the message will be sent even if the value is close to the last sent value
     * @return True if the message was successfully sent (or not since it was cached), false otherwise
     */
    bool SendCc(const uint8_t controller, const uint8_t value, const bool force = false);

    /**
     * @brief Sends a MIDI Note On message
     * @param note The note number (0-127)
     * @param velocity 0 - no press, 127 - full press
     * @return True if the message was added to the send queue
     */
    bool SendNoteOn(const uint8_t note, const uint8_t velocity = 127);

    /**
     * @brief Sends a MIDI Note Off message
     * @param note The note number (0-127)
     * @param velocity 0 - soft relase, 127 - hard relase (64 is the default)
     * @return True if the message was added to the send queue
     */
    bool SendNoteOff(const uint8_t note, const uint8_t velocity = 64);

    /**
     * @brief Sends a MIDI Global Pitch Bend message
     * @param value The pitch bend value (range: -8192 to 8191)
     * @param force If true, the message will be sent even if the value is close to the last sent value
     * @param min_diff Minimum difference between the last sent value and the new value to send the message (ignored if force is true)
     * @return True if the message was added to the send queue
     */
    bool SendPitchBend(const int32_t value, const bool force = false, const uint32_t min_diff = 1);

    /**
     * @brief Sends a MIDI Clock Pulse message
     * @return True if the message was added to the send queue
     */
    bool SendClockPulse();

    /**
     * @brief Sends a MIDI Clock Stop message
     * @return True if the message was added to the send queue
     */
    bool SendClockStop();

    /**
     * @brief Sends a MIDI Clock Start message
     * @return True if the message was added to the send queue
     */
    bool SendClockStart();

    /**
     * @brief Sends a MIDI Clock Stop message followed by a Clock Start message
     * @return True if both messages were successfully sent, false otherwise
     */
    bool SendClockReset();

private:
    /**
     * @brief Calls the registered callbacks with the received MIDI message
     * @param midi_message Pointer to the received MIDI message
     * @details This function calls both the base and application callbacks with the received MIDI message.
     */
    void Callbacks(Message *midi_message);

#if MIDI_CC_RATE_LIMIT > 0
    // Minimum time interval between sending the same CC (in ms)
    static constexpr uint32_t kCcRateLimit = MIDI_CC_RATE_LIMIT;

    // Timestamp of last CC send (in ms)
    std::array<uint32_t, 128> output_cc_last_time_{};
#endif

    // MIDI parsing stuff
    std::array<uint8_t, 3> msg_buffer_{0, 0, 0};
    size_t byte_counter_ = 0;

    // Callbacks when a MIDI message is received
    Callback base_callback_ = nullptr;
    Callback app_callback_ = nullptr;

    // Channel stuff
    size_t channel_ = Message::kAllChannels;
    size_t learning_channel_ = Message::kAllChannels;
    bool learning_ = false;

    // Buffer for output MIDI messages
    RingBuffer<Message::UsbPacket, 32> output_midi_buffer_;

    // Value of the last sent CC for each controller (0-127)
    std::array<uint8_t, 128> output_cc_cache_;

    // Buffer for the last sent pitch bend value,
    // so we can avoid sending the same (or similar) value multiple times
    int32_t output_pitch_bend_cache_ = INT32_MAX;
};
}
}
