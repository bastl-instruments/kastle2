/*
MIT License

Copyright (c) 2024 Vaclav Mach (Bastl Instruments)

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

#include "Memory.hpp"
#include <memory>
#include "common/config.hpp"
#include "common/core/Hardware.hpp"
#include "common/core/midi/Message.hpp"

using namespace kastle2;

Memory::State Memory::Init(i2c_inst_t *i2c_inst)
{
    available_ = false;
    ClearQueue();

    // Initialize the EEPROM
    eeprom_.Init(i2c_inst);

    // Try read from EEPROM
    uint8_t readString[kTestStringLength];
    bool initialized = true;

    // Available?
    if (!eeprom_.Read(ADDR_INIT_MESSAGE, readString, kTestStringLength))
    {
        return State::MISSING;
    }

    // Test string
    for (size_t i = 0; i < kTestStringLength; i++)
    {
        if (kTestString[i] != readString[i])
        {
            initialized = false;
            break;
        }
    }

    // Memory not initialized, initialize!
    if (!initialized)
    {
        if (FreshInitialize())
        {
            available_ = true;
            return State::INIT_OK;
        }
        available_ = false;
        return State::INIT_FAIL;
    }

    available_ = true;
    return State::OK;
}

bool Memory::FreshInitialize()
{
    bool result = true;

    // Make memory available, so the `Write16` functions work
    available_ = true;

    // Write calibrations
    result &= Write8(ADDR_PITCH_CALIBRATIONS_VALID, false);
    result &= Write8(ADDR_CVOUT_CALIBRATIONS_VALID, false);

    // Write defaults
    WriteDefaultSettings();

    // Write test string
    result &= eeprom_.Write(ADDR_INIT_MESSAGE, kTestString, kTestStringLength);
    sleep_ms(50);

    return result;
}

bool Memory::WriteDefaultSettings()
{
    bool result = true;
    result &= Write8(ADDR_INPUT_GAIN, 160u);
    result &= Write8(ADDR_OUTPUT_GAIN, 160u);
    result &= Write8(ADDR_SYNC_SETTINGS, 0u);
    result &= Write32(ADDR_CLOCK_TICKS, 140u);
    result &= Write8(ADDR_CLOCK_DIVIDER, 1u);
    result &= Write8(ADDR_CLOCK_MULTIPLIER, 1u);
    result &= Write8(ADDR_CLOCK_MIDI_DIVIDER, kMidiTempoDividerDefault);
    result &= Write8(ADDR_MIDI_CHANNEL, midi::Message::kAllChannels);
    result &= Write8(ADDR_MONO_SETTINGS, 0u);
    result &= ClearAppSpace();
    return result;
}

bool Memory::ClearAppSpace()
{
    uint8_t buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int32_t to_write = APP_SPACE_SIZE;
    while (to_write > 0)
    {
        size_t write_size = to_write > 8 ? 8 : to_write;
        if (!Write(ADDR_APP_SPACE + APP_SPACE_SIZE - to_write, buffer, write_size))
        {
            return false;
        }
        to_write -= write_size;
    }
    return true;
}

bool Memory::Read(uint16_t address, uint8_t *buffer, uint16_t length)
{
    if (!available_)
    {
        return false;
    }
    return eeprom_.Read(address, buffer, length);
}

uint8_t Memory::Get8(uint16_t address)
{
    if (!available_)
    {
        return 0;
    }
    uint8_t result = 0;
    eeprom_.ReadByte(address, &result);
    return result;
}

uint16_t Memory::Get16(uint16_t address)
{
    if (!available_)
    {
        return 0;
    }
    uint8_t buffer[2] = {0, 0};
    eeprom_.Read(address, buffer, 2);
    return (buffer[1] << 8) | buffer[0];
}

uint32_t Memory::Get32(uint16_t address)
{
    if (!available_)
    {
        return 0;
    }
    uint8_t buffer[4] = {0, 0, 0, 0};
    eeprom_.Read(address, buffer, 3);
    uint32_t read_value = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
    return read_value;
}

bool Memory::Read8(uint16_t address, uint8_t *value)
{
    if (!available_)
    {
        return false;
    }
    return eeprom_.ReadByte(address, value);
}

bool Memory::Read16(uint16_t address, uint16_t *value)
{
    if (!available_)
    {
        return false;
    }
    uint8_t buffer[2] = {0, 0};
    eeprom_.Read(address, buffer, 2);
    *value = (buffer[1] << 8) | buffer[0];
    return true;
}

bool Memory::Read32(uint16_t address, uint32_t *value)
{
    if (!available_)
    {
        return false;
    }
    uint8_t buffer[4] = {0, 0};
    eeprom_.Read(address, buffer, 4);
    uint32_t read_value = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
    *value = read_value;
    return true;
}

bool Memory::Read8Into32(uint16_t address, uint32_t *value)
{
    if (!available_)
    {
        return false;
    }
    uint8_t buffer[1] = {0};
    eeprom_.ReadByte(address, buffer);
    *value = buffer[0];
    return true;
}

bool Memory::Read16Into32(uint16_t address, uint32_t *value)
{
    if (!available_)
    {
        return false;
    }
    uint8_t buffer[2] = {0, 0};
    eeprom_.Read(address, buffer, 2);
    uint16_t read_value = (buffer[1] << 8) | buffer[0];
    *value = read_value;
    return true;
}

bool Memory::Write(uint16_t address, const uint8_t *data, uint16_t length, bool verify, uint32_t retries)
{
    if (!available_)
    {
        return false;
    }

    bool result;

    do
    {
        // Perform the write operation
        result = eeprom_.Write(address, data, length);

        // If write failed or no verification is required, exit the loop
        if (!result || !verify)
        {
            break;
        }

        if (length > MEMORY_SIZE)
        {
            // Maybe OK?
            return true;
        }

        // Buffer for verification
        std::unique_ptr<uint8_t[]> read_buffer = std::make_unique<uint8_t[]>(length);
        Read(address, read_buffer.get(), length);

        // Check if data matches what was written
        bool data_match = true;
        for (uint16_t i = 0; i < length; ++i)
        {
            if (data[i] != read_buffer[i])
            {
                data_match = false;
                break;
            }
        }

        // If data matches, success!
        if (data_match)
        {
            return true;
            break;
        }

        // If out of retries, failure
        if (retries == 0)
        {
            result = false;
            break;
        }

        // Otherwise, decrement retries and continue
        retries--;
        sleep_ms(1); // Small delay before retrying

    } while (true);

    return result;
}

bool Memory::Write8(uint16_t address, uint8_t value, bool verify, uint32_t retries)
{
    uint8_t data[1] = {value};
    return Write(address, data, 1, verify, retries);
}

bool Memory::Write16(uint16_t address, uint16_t value, bool verify, uint32_t retries)
{
    uint8_t buffer[2] = {static_cast<uint8_t>(value & 0xFF),
                         static_cast<uint8_t>((value >> 8) & 0xFF)};
    return Write(address, buffer, 2, verify, retries);
}

bool Memory::Write32(uint16_t address, uint32_t value, bool verify, uint32_t retries)
{
    uint8_t buffer[4] = {static_cast<uint8_t>(value & 0xFF),
                         static_cast<uint8_t>((value >> 8) & 0xFF),
                         static_cast<uint8_t>((value >> 16) & 0xFF),
                         static_cast<uint8_t>((value >> 24) & 0xFF)};
    return Write(address, buffer, 4, verify, retries);
}

bool Memory::Update8(uint16_t address, uint8_t value)
{
    if (!available_)
    {
        return false;
    }
    if (Get8(address) != value)
    {
        return Write8(address, value);
    }
    return true;
}

bool Memory::Update16(uint16_t address, uint16_t value)
{
    if (!available_)
    {
        return false;
    }
    if (Get16(address) != value)
    {
        return Write16(address, value);
    }
    return true;
}

bool Memory::Update32(uint16_t address, uint32_t value)
{
    if (!available_)
    {
        return false;
    }
    uint32_t read_value = Get32(address);
    if (read_value != value)
    {
        return Write32(address, value);
    }
    return true;
}

bool Memory::WriteCalibrations(const Hardware::CalibrationsType &calibrations)
{
    bool result = true;
    size_t i = 0;
    for (auto calibration : EnumRange<Hardware::Calibration>())
    {
        int16_t calibration_16b = calibrations[calibration];
        result &= Write16(ADDR_CALIBRATIONS_START + i, calibration_16b);
        i += 2; // Each calibration is 2 bytes
    }
    result &= Write8(ADDR_PITCH_CALIBRATIONS_VALID, true);
    return result;
}

bool Memory::ClearCalibrations()
{
    bool result = true;
    size_t i = 0;
    result &= Write8(ADDR_PITCH_CALIBRATIONS_VALID, false);
    for ([[maybe_unused]] auto calibration : EnumRange<Hardware::Calibration>())
    {
        int16_t calibration_16b = 0;
        result &= Write16(ADDR_CALIBRATIONS_START + i, calibration_16b);
        i += 2; // Each calibration is 2 bytes
    }
    return result;
}

bool Memory::AreCalibrationsValid()
{
    if (!available_)
    {
        return false;
    }
    return Get8(ADDR_PITCH_CALIBRATIONS_VALID) != 0;
}

bool Memory::ReadCalibrations(Hardware::CalibrationsType &calibrations)
{
    if (!AreCalibrationsValid())
    {
        return false;
    }
    size_t i = 0;
    for (auto calibration : EnumRange<Hardware::Calibration>())
    {
        calibrations[calibration] = Get16(ADDR_CALIBRATIONS_START + i);
        i += 2; // Each calibration is 2 bytes
    }
    return true;
}

void Memory::QueueUpdate8(uint16_t address, uint8_t value)
{
    QueueItem item;
    item.address = address;
    item.value = value;
    item.size = 1;
    QueueAdd(item);
}

void Memory::QueueUpdate16(uint16_t address, uint16_t value)
{
    QueueItem item;
    item.address = address;
    item.value = value;
    item.size = 2;
    QueueAdd(item);
}

void Memory::QueueUpdate32(uint16_t address, uint32_t value)
{
    QueueItem item;
    item.address = address;
    item.value = value;
    item.size = 4;
    QueueAdd(item);
}

void Memory::QueueAdd(QueueItem item)
{
    while (queue_size_ >= UPDATE_QUEUE_MAX_SIZE)
    {
        ProcessQueue(); // Process the queue if it's full
    }

    // Add to the queue
    uint32_t insert_position = (queue_index_ + queue_size_) % UPDATE_QUEUE_MAX_SIZE;
    queue_[insert_position] = item;
    queue_size_++;
}

void Memory::ProcessQueue()
{
    if (queue_size_ == 0)
    {
        return; // Nothing to process
    }

    // Get the item at the front of the queue
    QueueItem item = queue_[queue_index_];

    // Process based on size
    switch (item.size)
    {
    case 1:
        Update8(item.address, static_cast<uint8_t>(item.value));
        break;
    case 2:
        Update16(item.address, static_cast<uint16_t>(item.value));
        break;
    case 4:
        Update32(item.address, item.value);
        break;
    }

    // Move the index to next item (with wrap-around)
    queue_index_ = (queue_index_ + 1) % UPDATE_QUEUE_MAX_SIZE;
    queue_size_--;
}

void Memory::ClearQueue()
{
    queue_index_ = 0;
    queue_size_ = 0;
}