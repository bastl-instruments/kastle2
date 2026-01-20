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

#include "hardware/i2c.h"

namespace kastle2
{

/**
 * @class AT24C
 * @ingroup peripherals
 * @brief Driver for EEPROM memories (24XX02, 24XX32, ...)
 * @note Don't access directly, use Kastle2::memory instead.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-04-22
 */
class AT24C
{
public:
    /**
     * Raw constructor, you can use this or predefined static functions.
     *
     * @param size Size of the memory in bytes
     * @param page_size Size of the memory page in bytes
     */
    AT24C(size_t size, size_t address_size, size_t page_size)
        : size_(size), address_size_(address_size), page_size_(page_size)
    {
    }

    /**
     * AT24C32 (2 kBits => 256 Bytes)
     * @return AT24C object
     */
    static AT24C AT24C02()
    {
        return AT24C(256, 1, 8);
    }

    /**
     * AT24C32 (32 kBits => 4 kBytes)
     * @return AT24C object
     */
    static AT24C AT24C32()
    {
        return AT24C(4096, 2, 32);
    }

    /**
     * Sets up the I2C memory
     * doesn't connect to it yet
     */
    void Init(i2c_inst_t *i2c_inst, uint8_t i2c_address_ = kDefaultI2cAddress);

    /**
     * Writes an array of bytes
     * If the writing fails (eg. chip not available), returns false
     */
    bool Write(uint16_t address, const uint8_t *data, size_t len);

    /**
     * Writes a single byte
     * If the writing fails (eg. chip not available), returns false
     */
    bool WriteByte(uint16_t address, uint8_t data);

    /**
     * Reads an array of bytes
     * If the reading fails (eg. chip not available), returns false
     */
    bool Read(uint16_t address, uint8_t *data, size_t len);

    /**
     * Reads a single byte
     * If the reading fails (eg. chip not available), returns false
     */
    bool ReadByte(uint16_t address, uint8_t *data);

    /**
     * Default memory address
     */
    static constexpr uint8_t kDefaultI2cAddress = 0x50;

    /**
     * Maxiumum memory address size
     */
    static constexpr size_t kMaxAddressSize = 2;

    /**
     * Simple timeout for writing,
     * because EEPROM takes a while when it's writing the values
     */
    static constexpr uint32_t kWriteTimeout = 1000;

    /**
     * Simple timeout for reading.
     */
    static constexpr uint32_t kReadTimeout = 100;

private:
    uint8_t i2c_address_;
    i2c_inst_t *i2c_inst_;

    size_t size_;
    size_t address_size_;
    size_t page_size_;

    // Delay between page writes in ms (usually 5ms but keeping 6ms to be safe)
    static constexpr uint32_t kWriteDelay = 6;

    size_t PageWrite(uint16_t address, const uint8_t *data, size_t len);
    bool WriteAddressTo(uint16_t address, uint8_t *buffer);
};
}
