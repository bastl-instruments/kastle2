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

#include "Message.hpp"
#include <cmath>

using namespace kastle2;
using namespace kastle2::midi;

Message::Type Message::RetrieveType(const uint8_t message_first_byte)
{
    Type type = Type::INVALID;
    // System common messages
    if ((message_first_byte & kSystemMessageMask) == kSystemMessageMask)
    {
        for (const uint8_t &message : kReservedMessages)
        {
            if (message_first_byte == message)
            {
                break;
            }
            type = static_cast<Type>(message_first_byte);
        }
    }
    else
    {
        type = static_cast<Type>(message_first_byte & kSystemMessageMask);
    }
    return type;
}

uint32_t Message::RetrieveNumBytes(const uint8_t message_first_byte)
{
    Type type = RetrieveType(message_first_byte);
    switch (type)
    {
    // types with 2 data bytes
    case Type::NOTE_ON:
    case Type::NOTE_OFF:
    case Type::AFTER_TOUCH_POLY:
    case Type::CONTROL_CHANGE:
    case Type::PITCH_BEND:
    case Type::SYSTEM_EXCLUSIVE:
    case Type::SONG_POSITION:
        return 2;
    // types with 1 data byte
    case Type::PROGRAM_CHANGE:
    case Type::AFTER_TOUCH_GLOBAL:
    case Type::TIME_CODE_QUARTER_FRAME:
    case Type::SONG_SELECT:
        return 1;
    // types with no data bytes
    case Type::TUNE_REQUEST:
    case Type::CLOCK:
    case Type::START:
    case Type::CONTINUE:
    case Type::STOP:
    case Type::ACTIVE_SENSING:
    case Type::SYSTEM_RESET:
    case Type::INVALID:
        return 0;
    };

    return 0;
}

Message Message::ParseFromTrs(const std::array<uint8_t, 3> &data)
{
    Message msg;
    msg.data_[0] = data[0];
    msg.data_[1] = data[1];
    msg.data_[2] = data[2];
    msg.source_ = Source::TRS;
    msg.ParseChannelAndType();
    return msg;
}

Message Message::ParseFromUsb(const std::array<uint8_t, 4> &data)
{
    Message msg;
    // The first byte of the USB MIDI is the CIN byte, so we don't use it as data
    msg.data_[0] = data[1];
    msg.data_[1] = data[2];
    msg.data_[2] = data[3];
    msg.source_ = Source::USB;
    msg.ParseChannelAndType();
    return msg;
}

void Message::StoreChannelAndType()
{
    // For system messages (0xF0-0xFF), don't add channel information
    if (static_cast<uint8_t>(type_) >= 0xF0)
    {
        data_[0] = static_cast<uint8_t>(type_);
        return;
    }

    // For channel messages, combine type with channel
    uint8_t ch = channel_;
    // Use channel 0 if kAllChannels or invalid
    if (ch == kAllChannels || ch > 15)
    {
        ch = 0;
    }
    data_[0] = static_cast<uint8_t>(type_) | (ch & 0x0F);
}

void Message::ParseChannelAndType()
{
    uint8_t channel = Message::kAllChannels;
    if ((data_[0] & 0xF0) != 0xF0)
    {
        channel = data_[0] & 0x0F; // Extract channel from the first byte
    }
    type_ = RetrieveType(data_[0]);
    channel_ = channel; // Set the channel
}

Message::UsbPacket Message::GetUsbPacket(const uint8_t cable_number)
{
    StoreChannelAndType();
    UsbPacket packet;
    packet.data[0] = GenerateCinByte(type_, cable_number);
    packet.data[1] = data_[0];
    packet.data[2] = data_[1];
    packet.data[3] = data_[2];
    return packet;
}

uint8_t Message::GenerateCinByte(const Type type, const uint8_t cable_number)
{
    uint8_t code_index = 0;
    // System messages
    switch (type)
    {
    case Type::SYSTEM_EXCLUSIVE:
        code_index = 0x4;
        break;
    case Type::TIME_CODE_QUARTER_FRAME:
        code_index = 0x2;
        break;
    case Type::SONG_POSITION:
        code_index = 0x3;
        break;
    case Type::SONG_SELECT:
        code_index = 0x2;
        break;
    case Type::TUNE_REQUEST:
        code_index = 0x5;
        break;
    case Type::SYSTEM_EXCLUSIVE_END:
        code_index = 0x5;
        break;
    case Type::CLOCK:
    case Type::START:
    case Type::CONTINUE:
    case Type::STOP:
    case Type::ACTIVE_SENSING:
    case Type::SYSTEM_RESET:
        code_index = 0xF;
        break;
    case Type::NOTE_OFF:
        code_index = 0x8;
        break;
    case Type::NOTE_ON:
        code_index = 0x9;
        break;
    case Type::AFTER_TOUCH_POLY:
        code_index = 0xA;
        break;
    case Type::CONTROL_CHANGE:
        code_index = 0xB;
        break;
    case Type::PROGRAM_CHANGE:
        code_index = 0xC;
        break;
    case Type::AFTER_TOUCH_GLOBAL:
        code_index = 0xD;
        break;
    case Type::PITCH_BEND:
        code_index = 0xE;
        break;
    default:
        code_index = 0x0;
        break;
    }
    return (cable_number << 4) | code_index;
}

float Message::GetPitchBendAsFloat() const
{
    // Convert 0-16383 range to -8192 to 8191 range
    int16_t value = (static_cast<int16_t>(data_[2]) << 7) | data_[1];
    // Convert to float range -1.0 to 1.0
    return static_cast<float>(value + kPitchBendMin) / (-kPitchBendMin);
}

float Message::GetPitchBendAsSemitones(float semitones_range) const
{
    float pitch_bend = GetPitchBendAsFloat();
    // Map pitch_bend from [-1, 1] to [-7, 7] semitones range
    float semitones = pitch_bend * semitones_range;
    // Convert semitones to frequency multiplier
    return std::pow(2.0f, semitones / 12.0f);
}