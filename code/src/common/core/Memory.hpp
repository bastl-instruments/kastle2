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

#pragma once

#include <cstdint>
#include "hardware/i2c.h"
#include "common/core/Hardware.hpp"
#include "common/peripherals/AT24C.hpp"

namespace kastle2
{

/**
 * @class Memory
 * @ingroup core
 * @brief EEPROM memory abstraction.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-05-14
 */
class Memory
{

public:
    static constexpr size_t ADDR_CALIBRATIONS_SPACE = 0x00; // Calibrations space starts here (0 bytes)
    static constexpr size_t ADDR_BASE_SPACE = 0x30;         // Base space starts here (at 48 bytes)
    static constexpr size_t ADDR_APP_SPACE = 0x50;          // Application space starts here (at 80 bytes)

    // CALIBRATIONS SPACE
    static constexpr size_t ADDR_PITCH_CALIBRATIONS_VALID = ADDR_CALIBRATIONS_SPACE + 0x00; // 8-bit number
    static constexpr size_t ADDR_CVOUT_CALIBRATIONS_VALID = ADDR_CALIBRATIONS_SPACE + 0x00; // 8-bit number (not implemented yet)
    static constexpr size_t ADDR_CALIBRATIONS_START = ADDR_CALIBRATIONS_SPACE + 0x02;       // 10 x 2 bytes

    // Ensure calibrations space is before base space
    static_assert(ADDR_CALIBRATIONS_START + (static_cast<size_t>(Hardware::Calibration::COUNT) * 2) < ADDR_BASE_SPACE);

    // BASE SPACE
    static constexpr size_t ADDR_APP_ID = ADDR_BASE_SPACE + 0x00;             // 8-bit number
    static constexpr size_t ADDR_APP_VERSION = ADDR_BASE_SPACE + 0x01;        // 8-bit number
    static constexpr size_t ADDR_SYNC_SETTINGS = ADDR_BASE_SPACE + 0x02;      // 8-bit number
    static constexpr size_t ADDR_INPUT_GAIN = ADDR_BASE_SPACE + 0x03;         // 8-bit number
    static constexpr size_t ADDR_OUTPUT_GAIN = ADDR_BASE_SPACE + 0x04;        // 8-bit number
    static constexpr size_t ADDR_MONO_SETTINGS = ADDR_BASE_SPACE + 0x05;      // 8-bit number
    static constexpr size_t ADDR_CLOCK_TICKS = ADDR_BASE_SPACE + 0x06;        // 32-bit number
    static constexpr size_t ADDR_CLOCK_DIVIDER = ADDR_BASE_SPACE + 0x0A;      // 8-bit number
    static constexpr size_t ADDR_CLOCK_MULTIPLIER = ADDR_BASE_SPACE + 0x0B;   // 8-bit number
    static constexpr size_t ADDR_CLOCK_MIDI_DIVIDER = ADDR_BASE_SPACE + 0x0C; // 8-bit number
    static constexpr size_t ADDR_MIDI_CHANNEL = ADDR_BASE_SPACE + 0x0D;       // 8-bit number

    // APP SPACE
    // ... starts at ADDR_APP_SPACE (0x50) and defined by the application

    enum class MonoSetting
    {
        STEREO, ///< Normal stereo input
        LEFT,   ///< Only left channel is used (and copied to the right channel)
        RIGHT,  ///< Only right channel is used (and copied to the left channel)
        COUNT
    };

    enum class SyncSetting
    {
        NONE_DISABLED,     ///< Normal sync setting
        EXTERNAL_DISABLED, ///< External sync using jack or patchbay is disabled
        MIDI_DISABLED,     ///< MIDI sync (both TRS and USB) is disabled
        COUNT
    };

    // Last 8 bytes are reserved for the test string (248 - 255)
    static constexpr size_t ADDR_INIT_MESSAGE = 0xf8;

    // App space size
    static constexpr size_t APP_SPACE_SIZE = ADDR_INIT_MESSAGE - ADDR_APP_SPACE;

    static constexpr size_t kTestStringLength = 8;
    static constexpr uint8_t kTestString[kTestStringLength] = {'2', '0', '2', '5', '0', '7', '0', '3'}; // Date in ASCII

    enum class State
    {
        OK,
        INIT_OK,
        INIT_FAIL,
        MISSING,
        COUNT
    };

    /**
     * Starts the EEPROM.
     * @param i2c_inst I2C instance
     * @return OK, INIT_OK, INIT_FAIL, MISSING...
     */
    State Init(i2c_inst_t *i2c_inst);

    /**
     * Initialize the EEPROM with default settings.
     * @return true if successful
     */
    bool FreshInitialize();

    /**
     * @brief Writes the default tempo, audio and stuff like this.
     * @return true if successful
     */
    bool WriteDefaultSettings();

    /**
     * @brief Clears the application space.
     * @return true if successful
     */
    bool ClearAppSpace();

    /**
     * @brief Reads a block of data into the memory.
     *        If the memory not available, does nothing.
     *
     * @param address Memory address
     * @param buffer Buffer to store the data
     * @param length Length of the data
     * @return true if successful
     */
    bool Read(uint16_t address, uint8_t *buffer, uint16_t length);

    /**
     * @brief Reads a byte back to the value.
     *        If the memory not available, does nothing.
     * @param address Memory address
     * @param value Value to store the byte
     * @return true if successful
     */
    bool Read8(uint16_t address, uint8_t *value);

    /**
     * @brief Reads a 16-bit number back to the 16-bit value.
     *        If the memory not available, does nothing.
     * @param address Memory address
     * @param value Value to store the 16-bit number
     * @return true if successful
     */
    bool Read16(uint16_t address, uint16_t *value);

    /**
     * @brief Reads a 32-bit number back to the 32-bit value.
     *        If the memory not available, does nothing.
     * @param address Memory address
     * @param value Value to store the 32-bit number
     * @return true if successful
     */
    bool Read32(uint16_t address, uint32_t *value);

    /**
     * @brief Reads a 16-bit number stored in the memory to the 32-bit value.
     *        If the memory not available, does nothing.
     *
     * @param address Memory address
     * @param value Value to store the 32-bit number
     * @return true if successful
     */
    bool Read16Into32(uint16_t address, uint32_t *value);

    /**
     * @brief Reads a byte stored in the memory to the 32-bit value.
     *        If the memory not available, does nothing.
     *
     * @param address Memory address
     * @param value Value to store the 32-bit number
     * @return true if successful
     */
    bool Read8Into32(uint16_t address, uint32_t *value);

    /**
     * @brief Returns a byte from the memory.
     *        If the memory not available, returns 0.
     *
     * @param address Memory address
     * @return Byte value or 0 if not available
     */
    uint8_t Get8(uint16_t address);

    /**
     * @brief Returns a 16-bit number from the memory.
     *        If the memory not available, returns 0.
     *
     * @param address Memory address
     * @return 16-bit number or 0 if not available
     */
    uint16_t Get16(uint16_t address);

    /**
     * @brief Returns a 32-bit number from the memory.
     *        If the memory not available, returns 0.
     *
     * @param address Memory address
     * @return 32-bit number or 0 if not available
     */
    uint32_t Get32(uint16_t address);

    /**
     * @brief Writes a block of data into the memory. By default it verifies the write.
     *
     * @param address Memory address
     * @param data Data to write
     * @param length Length of the data
     * @param verify Enable verification of write
     * @param retries Number of retries if verification fails
     * @return true if successful
     */
    bool Write(uint16_t address, const uint8_t *data, uint16_t length, bool verify = WRITE_VERIFY, uint32_t retries = WRITE_RETRIES);

    /**
     * @brief Writes a byte into the memory. By default it verifies the write.
     *
     * @param address Memory address
     * @param value Value to write
     * @param verify Enable verification of write
     * @param retries Number of retries if verification fails
     * @return true if successful
     */
    bool Write8(uint16_t address, uint8_t value, bool verify = WRITE_VERIFY, uint32_t retries = WRITE_RETRIES);

    /**
     * @brief Writes a 16-bit number into the memory. By default it verifies the write.
     * @param address Memory address
     * @param value Value to write
     * @param verify Enable verification of write
     * @param retries Number of retries if verification fails
     * @return true if successful
     */
    bool Write16(uint16_t address, uint16_t value, bool verify = WRITE_VERIFY, uint32_t retries = WRITE_RETRIES);

    /**
     * @brief Writes a 32-bit number into the memory. By default it verifies the write.
     * @param address Memory address
     * @param value Value to write
     * @param verify Enable verification of write
     * @param retries Number of retries if verification fails
     * @return true if successful
     */
    bool Write32(uint16_t address, uint32_t value, bool verify = WRITE_VERIFY, uint32_t retries = WRITE_RETRIES);

    /**
     * @brief Reads a byte from the memory, compares with the given value.
     *        If it differs, writes the new value into the memory.
     *        Verifies the write.
     * @param address Memory address
     * @param value Value to update
     * @return true if successful
     */
    bool Update8(uint16_t address, uint8_t value);

    /**
     * @brief Reads a 16-bit number from the memory, compares with the given value.
     *        If it differs, writes the new value into the memory.
     *        Verifies the write.
     * @param address Memory address
     * @param value Value to update
     * @return true if successful
     */
    bool Update16(uint16_t address, uint16_t value);

    /**
     * @brief Reads a 32-bit number from the memory, compares with the given value.
     *        If it differs, writes the new value into the memory.
     *        Verifies the write.
     * @param address Memory address
     * @param value Value to update
     * @return true if successful
     */
    bool Update32(uint16_t address, uint32_t value);

    /**
     * @brief Queues a byte update to be written to the memory. Verifies the write.
     * @param address Memory address
     * @param value Value to update
     * @warning The Factory Reset will not wait for the queue to empty, use Write8 instead.
     */
    void QueueUpdate8(uint16_t address, uint8_t value);

    /**
     * @brief Queues a 16-bit number update to be written to the memory. Verifies the write.
     * @param address Memory address
     * @param value Value to update
     * @warning The Factory Reset will not wait for the queue to empty, use Write16 instead.
     */
    void QueueUpdate16(uint16_t address, uint16_t value);

    /**
     * @brief Queues a 32-bit number update to be written to the memory. Verifies the write.
     * @param address Memory address
     * @param value Value to update
     * @warning The Factory Reset will not wait for the queue to empty, use Write32 instead.
     */
    void QueueUpdate32(uint16_t address, uint32_t value);

    /**
     * @brief Takes the first item from the queue and writes it to the memory.
     *        If the queue is empty, does nothing.
     */
    void ProcessQueue();

    /**
     * @brief Clears the queue.
     */
    void ClearQueue();

    /**
     * @brief Writes calibrations array into the memory
     * @param calibrations 4 items array
     * @return true if successful
     */
    bool WriteCalibrations(const Hardware::CalibrationsType &calibrations);

    /**
     * @brief Clears the calibrations array in the memory.
     * @return true if successful
     */
    bool ClearCalibrations();

    /**
     * @brief Reads calibrations array from the memory
     * @param calibrations 4 items array for writing
     * @return true if successful
     */
    bool ReadCalibrations(Hardware::CalibrationsType &calibrations);

    /**
     * @brief Checks if the calibrations are valid.
     * @return true if valid, false otherwise
     */
    bool AreCalibrationsValid();

private:
    static constexpr uint16_t MEMORY_SIZE = 256;
    static constexpr uint32_t UPDATE_QUEUE_MAX_SIZE = 32;
    static constexpr uint32_t WRITE_RETRIES = 10;
    static constexpr bool WRITE_VERIFY = true;

    struct QueueItem
    {
        uint16_t address;
        uint32_t value;
        uint8_t size; // 1, 2, or 4 bytes
    };

    void QueueAdd(QueueItem item);
    uint32_t queue_index_ = 0;
    uint32_t queue_size_ = 0;
    QueueItem queue_[UPDATE_QUEUE_MAX_SIZE];

    AT24C eeprom_ = AT24C::AT24C02();
    bool available_ = false;
};
}
