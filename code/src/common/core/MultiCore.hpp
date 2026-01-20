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

#include <cstddef>
#include "pico/stdlib.h"
#include "pico/multicore.h"

namespace kastle2
{

/**
 * @class MultiCore
 * @ingroup core
 * @brief Helper for comunication between cores.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-08-01
 */
class MultiCore
{
public:
    /**
     * @brief Message types for communication between cores.
     */
    enum class MessageType
    {
        BEGIN,          ///< Clears the second core state
        SAMPLE_REQUEST, ///< Request the second core to process a sample
        DONE,           ///< Indicates that the second core has finished processing samples
    };

    /**
     * @brief Function type for the second core function.
     */
    using Worker = void (*)(void);

    /**
     * @brief Message structure for communication between cores.
     * Contains the message type and data.
     */
    struct Message
    {
        MessageType type;
        int16_t data;
    };

    /**
     * @brief Resets and starts the second core with the given function.
     * @param second_core_worker Function to run on the second core (just once).
     */
    static void StartSecondCore(Worker second_core_worker)
    {
        multicore_reset_core1();
        sleep_ms(1);
        multicore_launch_core1(second_core_worker);
    }

    /**
     * @brief Creates a message object.
     * @param type Type of the message.
     * @param data Data of the message.
     * @return Message
     */
    static Message CreateMessage(MessageType type, int16_t data)
    {
        Message message;
        message.type = type;
        message.data = data;
        return message;
    }

    /**
     * @brief Sends a message to the OTHER core. Waits until the FIFO is ready.
     * @param message Message object.
     */
    static void SendMessage(Message message)
    {
        uint32_t m = (static_cast<uint32_t>(message.type) << 16) | (message.data + 32768);
        while (!multicore_fifo_wready())
        {
            tight_loop_contents();
        }
        multicore_fifo_push_blocking(m);
    }

    /**
     * @brief Sends a message to the OTHER core. Waits until the FIFO is ready.
     * @param type Type of the message.
     * @param data Signed 16-bit int.
     */
    static void SendMessage(MessageType type, int16_t data)
    {
        SendMessage(CreateMessage(type, data));
    }

    /**
     * @brief Sends a message to the OTHER core with data = 0.
     * @param type Type of the message.
     */
    static void SendMessage(MessageType type)
    {
        SendMessage(type, 0);
    }

    /**
     * @brief Checks if there is a message from the OTHER core.
     * @return There is a message in the other core.
     */
    static bool HasMessage()
    {
        return multicore_fifo_rvalid();
    }

    /**
     * @brief Gets the message from the OTHER core. If there is none, it waits until there is.
     * @return Message
     */
    static Message GetMessage()
    {
        uint32_t m = multicore_fifo_pop_blocking();
        Message message;
        message.type = static_cast<MessageType>(m >> 16);
        message.data = (m & 0xFFFF) - 32768;
        return message;
    }

    /**
     * @brief Waits until a message of the given type arrives.
     */
    static void WaitForMessage(MessageType message_type)
    {
        while (true)
        {
            if (HasMessage())
            {
                Message m = GetMessage();
                if (m.type == message_type)
                {
                    break;
                }
            }
        }
    }
};
}
