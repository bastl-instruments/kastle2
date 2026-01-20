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

#include "UsbSerial.hpp"

using namespace kastle2;

void UsbSerial::Init()
{
    enabled_ = false;
    usb_got_char_ = 0;
    tx_buffer_pos_ = 0;
}

void UsbSerial::Flush()
{
    if (!enabled_)
    {
        return;
    }
    FlushInternalBuffer();
    tud_cdc_write_flush();
}

void UsbSerial::Process()
{
    if (!enabled_)
    {
        return;
    }

    // Run TinyUSB task to process any pending USB events
    tud_task();

    // Only process if USB CDC is connected
    if (!tud_cdc_connected())
    {
        return;
    }

    // Flush the data
    Flush();

    // Process any available incoming data
    if (tud_cdc_available())
    {
        usb_got_char_ = tud_cdc_read_char();
    }
}

void UsbSerial::Print(const char data)
{
    if (!enabled_)
    {
        return;
    }
    // If the buffer is full or we're asked to flush, flush it
    if (tx_buffer_pos_ >= TX_BUFFER_SIZE - 1)
    {
        FlushInternalBuffer();
    }

    // Add the character to the buffer
    tx_buffer_[tx_buffer_pos_++] = data;
}

void UsbSerial::Print(const char *data, const size_t size)
{
    if (!enabled_)
    {
        return;
    }
    if (!data)
    {
        return;
    }

    size_t len = size;

    // If size is 0, assume null-terminated string
    if (len == 0)
    {
        len = strlen(data);
    }

    // Fast path: if the data fits entirely in the buffer
    if (tx_buffer_pos_ + len <= TX_BUFFER_SIZE)
    {
        memcpy(&tx_buffer_[tx_buffer_pos_], data, len);
        tx_buffer_pos_ += len;
        return;
    }

    // Slow path: data doesn't fit entirely, so we need to chunk it
    size_t remaining = len;
    size_t offset = 0;

    while (remaining > 0)
    {
        // Calculate how much we can copy
        size_t space_available = TX_BUFFER_SIZE - tx_buffer_pos_;
        size_t to_copy = (remaining < space_available) ? remaining : space_available;

        // Copy data to the buffer
        if (to_copy > 0)
        {
            memcpy(&tx_buffer_[tx_buffer_pos_], data + offset, to_copy);
            tx_buffer_pos_ += to_copy;
            offset += to_copy;
            remaining -= to_copy;
        }

        // If the buffer is full, flush it
        if (tx_buffer_pos_ >= TX_BUFFER_SIZE - 1)
        {
            FlushInternalBuffer();
        }
    }
}

void UsbSerial::PrintLine(const char *data, const size_t size)
{
    if (!enabled_)
    {
        return;
    }
    Print(data, size);

    // Check if we have room for the newline and carriage return
    if (tx_buffer_pos_ >= TX_BUFFER_SIZE - 2)
    {
        FlushInternalBuffer();
    }

    // Add newline and carriage return to the buffer
    tx_buffer_[tx_buffer_pos_++] = '\n';
    tx_buffer_[tx_buffer_pos_++] = '\r';
}

void UsbSerial::FlushInternalBuffer()
{
    if (tx_buffer_pos_ > 0)
    {
        // If not connected, just discard the data
        if (!tud_cdc_connected())
        {
            tx_buffer_pos_ = 0;
            return;
        }

        uint32_t written = 0;
        while (written < tx_buffer_pos_)
        {
            // Write as much as possible
            uint32_t available = tud_cdc_write_available();
            if (available == 0)
            {
                // No space available, need to run tud_task() and try again
                tud_task();
                tud_cdc_write_flush();

                // If we lost connection while writing, discard remaining data
                if (!tud_cdc_connected())
                {
                    tx_buffer_pos_ = 0;
                    return;
                }
                continue;
            }

            uint32_t to_write = tx_buffer_pos_ - written;
            if (to_write > available)
            {
                to_write = available;
            }

            uint32_t count = tud_cdc_write(&tx_buffer_[written], to_write);
            written += count;

            if (count < to_write)
            {
                // Couldn't write all at once, run tud_task() and try again
                tud_task();
                tud_cdc_write_flush();
            }
        }

        // Reset buffer position
        tx_buffer_pos_ = 0;
    }
}

bool UsbSerial::ReceivedChar(const char character)
{
    if (!enabled_)
    {
        return false;
    }
    if (usb_got_char_ == character)
    {
        usb_got_char_ = 0;
        return true;
    }
    return false;
}

void UsbSerial::ReceivedFlush()
{
    if (!enabled_)
    {
        return;
    }
    tud_cdc_n_read_flush(0);
}
