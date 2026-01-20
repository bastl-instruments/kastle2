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

#include "AT24C.hpp"
#include <cstdlib>
#include <cstring>
#include <memory>
#include "hardware/i2c.h"

using namespace kastle2;

void AT24C::Init(i2c_inst_t *i2c_inst, uint8_t i2c_address)
{
    i2c_inst_ = i2c_inst;
    i2c_address_ = i2c_address;
}

bool AT24C::Read(uint16_t address, uint8_t *data, size_t len)
{
    uint8_t addr_buffer[kMaxAddressSize];
    WriteAddressTo(address, addr_buffer);

    size_t count = 0;
    count = i2c_write_blocking_until(i2c_inst_, i2c_address_, addr_buffer, address_size_, true, make_timeout_time_ms(kReadTimeout));
    if (count == 0)
    {
        return false;
    }
    count = i2c_read_blocking_until(i2c_inst_, i2c_address_, data, len, false, make_timeout_time_ms(kReadTimeout));
    if (count == 0)
    {
        return false;
    }
    return true;
}

bool AT24C::Write(uint16_t address, const uint8_t *data, size_t len)
{
    // Write data by pages
    while (len > 0)
    {
        size_t write_len = PageWrite(address, data, len);
        if (write_len == 0)
        {
            return false;
        }
        address += write_len;
        data += write_len;
        if (write_len > len) // something has gone wrong, return false
        {
            abort();
        }
        len -= write_len;
    }
    return true;
}

size_t AT24C::PageWrite(uint16_t address, const uint8_t *data, size_t len)
{
    // Check for page boundary and adjust write length if needed
    uint8_t page_offset = address % page_size_;       // Offset within the page
    size_t write_len = page_size_ - page_offset; // Number of bytes to write within this page
    if (len < write_len)
    {
        write_len = len;
    }

    // Dynamically allocate buffer for address + data
    size_t buffer_size = address_size_ + write_len;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[buffer_size]);
    WriteAddressTo(address, buffer.get());
    memcpy(buffer.get() + address_size_, data, write_len);
    i2c_write_blocking_until(i2c_inst_, i2c_address_, buffer.get(), buffer_size, false, make_timeout_time_ms(kWriteTimeout));

    // Sleep to allow EEPROM to write the data
    sleep_ms(kWriteDelay);

    // Returns number of written bytes
    return write_len;
}

bool AT24C::WriteByte(uint16_t address, uint8_t data)
{
    return Write(address, &data, 1);
}

bool AT24C::ReadByte(uint16_t address, uint8_t *data)
{
    return Read(address, data, 1);
}

bool AT24C::WriteAddressTo(uint16_t address, uint8_t *buffer)
{
    if (address_size_ == 2)
    {
        buffer[0] = address >> 8;   // MSB of address
        buffer[1] = address & 0xFF; // LSB of address
    }
    else
    {
        buffer[0] = address & 0xFF; // LSB of address
    }
    return true;
}