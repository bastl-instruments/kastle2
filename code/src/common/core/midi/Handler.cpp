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

#include "Handler.hpp"
#include "hardware/uart.h"
#include "common/core/Kastle2.hpp"
#include "common/core/Kastle2_cc.hpp"

using namespace kastle2;
using namespace kastle2::midi;

// MIDI uses 31,250 baud rate
#define MIDI_BAUD_RATE 31250
#define MIDI_UART uart0
#define MIDI_RX_PIN 1 // MIDI IN on GPIO1

void Handler::Init()
{
    // Initialize UART for MIDI
    uart_init(MIDI_UART, MIDI_BAUD_RATE);

    // Set the GPIO function to UART
    gpio_set_function(MIDI_RX_PIN, GPIO_FUNC_UART);

    // enable Tx and Rx fifos on UART
    uart_set_fifo_enabled(MIDI_UART, true);

    // Load MIDI channel from memory
    LoadFromMemory();

    // Set up the output CC cache and timing arrays
    for (size_t i = 0; i < output_cc_cache_.size(); ++i)
    {
        output_cc_cache_[i] = 255; // Initialize all CC values to 255 (=not set)
#if MIDI_CC_RATE_LIMIT
        output_cc_last_time_[i] = 0; // Initialize all last send times to 0
#endif
    }
}

void Handler::Process()
{
    // Read MIDI messages
    while (uart_is_readable(MIDI_UART)) // Handle UART MIDI (if there is any)
    {
        uint8_t midi_byte_ = uart_getc(MIDI_UART);

        // Check if the byte is a status byte
        if (midi_byte_ & 0x80)
        {
            // Check if it's a realtime message
            if ((midi_byte_ & Message::kRealTimeMask) == Message::kRealTimeMask)
            {
                std::array<uint8_t, 3> buff = {midi_byte_, 0, 0};
                Message msg = Message::ParseFromTrs(buff);
                Callbacks(&msg);
            }
            else // it's not a realtime message
            {
                msg_buffer_[0] = midi_byte_;
                byte_counter_ = 0; // Reset byte counter
            }
        }
        else
        {
            if (byte_counter_ == 0)
            {
                msg_buffer_[1] = midi_byte_;
                byte_counter_ = 1; // Move to the next byte
            }
            else if (byte_counter_ == 1)
            {
                msg_buffer_[2] = midi_byte_;
                byte_counter_ = 2; // No more will come
            }
        }

        if (byte_counter_ == Message::RetrieveNumBytes(msg_buffer_[0]))
        {
            // Reset the byte counter (this allows for Running Status to function properly)
            byte_counter_ = 0;

            // Parse the message from the buffer
            Message msg = Message::ParseFromTrs(msg_buffer_);

            // Call the base and app callback functions
            Callbacks(&msg);
        }
    }

    while (tud_midi_available()) // Handle USB MIDI (if there is any)
    {
        std::array<uint8_t, 4> buff;

        // Read the USB MIDI message
        tud_midi_packet_read(buff.data());
        Message msg = Message::ParseFromUsb(buff);

        // Call the base_and app callback functions
        Callbacks(&msg);
    }

    // Send MIDI messages
    while (!output_midi_buffer_.IsEmpty())
    {
        auto packet = output_midi_buffer_.Pop();
        if (packet.has_value())
        {
            tud_midi_packet_write(packet->data);
        }
    }
}

void Handler::SetBaseCallback(Callback callback)
{
    base_callback_ = callback;
}

void Handler::SetAppCallback(Callback callback)
{
    app_callback_ = callback;
}

void Handler::StartLearning()
{
    if (!learning_)
    {
        learning_channel_ = Message::kAllChannels; // Reset channel to all channels
        learning_ = true;
    }
}

bool Handler::StopLearning()
{
    // Not currently learning?
    if (!learning_)
    {
        return false;
    }

    learning_ = false;
    if (learning_channel_ != Message::kAllChannels)
    {
        channel_ = learning_channel_; // Set the channel to the learned one
        SaveToMemory();               // Save the channel to memory
        return true;                  // Learning was successful
    }
    return false; // Learning was not successful, no channel learned
}

void Handler::LoadFromMemory()
{
    uint8_t read_value;
    channel_ = Message::kAllChannels; // Default to channel 0
    if (Kastle2::memory.Read8(Memory::ADDR_MIDI_CHANNEL, &read_value))
    {
        if ((read_value <= 15) ||
            read_value == Message::kAllChannels)
        {
            channel_ = read_value;
        }
    }
}

void Handler::SaveToMemory()
{
    Kastle2::memory.QueueUpdate8(Memory::ADDR_MIDI_CHANNEL, channel_);
}

bool Handler::SetChannel(size_t channel)
{
    // Valid MIDI channel or Message::kAllChannels
    if (channel < 16 || channel == Message::kAllChannels)
    {
        channel_ = channel;
        SaveToMemory();
        return true;
    }
    return false;
}

void Handler::Callbacks(Message *midi_message)
{
    uint8_t ch = midi_message->GetChannel();

    // Learning? Store the channel
    if (learning_ && ch != Message::kAllChannels)
    {
        learning_channel_ = ch;
    }

    // Ignore messages not on the current channel
    if (ch != Message::kAllChannels &&
        ch != channel_ &&
        channel_ != Message::kAllChannels &&
        (!learning_ || ch != learning_channel_))
    {
        return;
    }

    // check for nullptr
    if (app_callback_ != nullptr)
    {
        app_callback_(midi_message);
    }
    if (base_callback_ != nullptr)
    {
        base_callback_(midi_message);
    }
}

void Handler::ReportDisconnected()
{
    Message msg = Message(
        Message::Type::CONTROL_CHANGE,
        channel_,
        cc::RESET_CONTROLLERS);
    Callbacks(&msg);
}

bool Handler::Send(Message *midi_message)
{
    if (midi_message == nullptr)
    {
        return false; // Cannot send null message
    }
    // Push the message to the output buffer
    // We aren't sending it immediately because it can come from different threads/callbacks etc.
    // The data is send in the Process() function
    return output_midi_buffer_.Push(midi_message->GetUsbPacket(0));
}

bool Handler::SendCc(const uint8_t controller, const uint8_t value, const bool force)
{
    if (controller > 127 || value > 127)
    {
        return false;
    }

    // No need to send if the value is the same as the last sent value
    if (!force && output_cc_cache_[controller] == value)
    {
        return true;
    }

#if MIDI_CC_RATE_LIMIT
    // Check if enough time has passed since last send of this CC
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (!force && (current_time - output_cc_last_time_[controller] < kCcRateLimit))
    {
        // Skip sending, too soon after last send
        return false;
    }
#endif

    // Load the value into the cache and update the timestamp
    output_cc_cache_[controller] = value;

#if MIDI_CC_RATE_LIMIT
    output_cc_last_time_[controller] = current_time;
#endif

    // Create the MIDI message
    Message msg = Message(
        Message::Type::CONTROL_CHANGE,
        channel_,
        controller,
        value);
    return Send(&msg);
}

bool Handler::SendNoteOn(const uint8_t note, const uint8_t velocity)
{
    if (note > 127 || velocity > 127)
    {
        return false;
    }

    Message msg = Message(
        Message::Type::NOTE_ON,
        channel_,
        note,
        velocity);
    return Send(&msg);
}

bool Handler::SendNoteOff(const uint8_t note, const uint8_t velocity)
{
    if (note > 127 || velocity > 127)
    {
        return false;
    }

    Message msg = Message(
        Message::Type::NOTE_OFF,
        channel_,
        note,
        velocity);
    return Send(&msg);
}

bool Handler::SendPitchBend(const int32_t value, const bool force, const uint32_t min_diff)
{

    // Invalid pitch bend value
    if (value < Message::kPitchBendMin || value > Message::kPitchBendMax)
    {
        return false;
    }

    // No need to send if the value is the same as the last sent value
    if (!force && diff(output_pitch_bend_cache_, value) < min_diff)
    {
        return true;
    }
    output_pitch_bend_cache_ = value;

    // Shift to 0-16383 range
    uint16_t midi_value = static_cast<uint32_t>(value - Message::kPitchBendMin);

    Message msg = Message(
        Message::Type::PITCH_BEND,
        channel_,
        midi_value & 0x7F,         // LSB
        (midi_value >> 7) & 0x7F); // MSB
    return Send(&msg);
}

bool Handler::SendClockPulse()
{
    Message msg = Message(Message::Type::CLOCK, channel_);
    return Send(&msg);
}

bool Handler::SendClockStop()
{
    Message msg = Message(Message::Type::STOP, channel_);
    return Send(&msg);
}
bool Handler::SendClockStart()
{
    Message msg = Message(Message::Type::START, channel_);
    return Send(&msg);
}

bool Handler::SendClockReset()
{
    if (!SendClockStop())
    {
        return false;
    }
    return SendClockStart();
}