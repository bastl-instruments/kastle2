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

#include "tusb.h"

namespace kastle2
{

/**
 * @class UsbSerial
 * @ingroup debug
 * @brief Simple USB serial interface for debuging. It's disabled by default to save performance.
 * @author Marek Mach (Bastl Instruments), Vaclav Mach (Bastl Instruments)
 * @note Use through Kastle2::debug. Call SetEnabled(true) to enable the communication.
 * @date 2025-03-4
 */
class UsbSerial
{
public:
    /**
     * @brief Initializes the USB serial interface
     */
    void Init();

    /**
     * @brief Sets the enabled state of the USB serial interface to send data
     * @param enabled If true, the USB serial interface is enabled to send/receive data
     */
    void SetEnabled(bool enabled)
    {
        enabled_ = enabled;
    }

    /**
     * @brief Task to be called in the main loop repeatedly, handles USB event and serial receiving / filtering
     */
    void Process();

    /**
     * @brief Sends a single character over USB serial
     *
     * @param data The character to send
     */
    void Print(const char data);

    /**
     * @brief Sends a string over USB serial
     *
     * @param data The string to send
     * @param size The size of the string (0 means null-terminated string)
     */
    void Print(const char *data, const size_t size = 0);

    /**
     * @brief Sends a string over USB serial with a new line and carrige return at the end
     *
     * @param data The string to send
     * @param size The size of the string (0 means null-terminated string)
     */
    void PrintLine(const char *data, const size_t size = 0);

    /**
     * @brief Flushes the USB serial output buffer
     * @note This is useful if you want to ensure the data is sent ASAP
     */
    void Flush();

    /**
     * @brief Checks if a character was received and returns true if it was
     * @note The character gets updated only when calling Process(), if it is detected, it is removed from the queue
     *
     * @param character The character to check for
     * @return true if the character was received
     */
    bool ReceivedChar(const char character);

    /**
     * @brief Flushes the received character buffer
     */
    void ReceivedFlush();

private:
    // Disabled by default to save performance
    bool enabled_ = false;

    // RX stuff
    char usb_got_char_ = 0;

    // TX stuff
    static constexpr size_t TX_BUFFER_SIZE = 256;
    char tx_buffer_[TX_BUFFER_SIZE];
    size_t tx_buffer_pos_ = 0;

    // Helper method to flush the internal buffer
    void FlushInternalBuffer();
};
} // namespace kastle2
