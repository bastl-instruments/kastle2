/*
MIT License

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

#include <cstddef>
#include <optional>

namespace kastle2
{

/**
 * @class RingBuffer
 * @ingroup common_dsp_utility
 * @brief A simple ring buffer implementation for storing items of type T with a fixed size N.
 * @tparam T The type of items to store in the buffer.
 * @tparam N The size of the buffer (must be greater than 1).
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-06-12
 */
template <typename T, size_t N>
class RingBuffer
{
    static_assert(N > 1, "RingBuffer size must be greater than 1");

public:
    /**
     * @brief Default constructor for the RingBuffer.
     */
    RingBuffer() = default;

    // Disable copy/move to avoid accidental overhead
    RingBuffer(const RingBuffer &) = delete;
    RingBuffer &operator=(const RingBuffer &) = delete;

    /**
     * @brief Push an item into the ring buffer
     * @param item The item to push into the buffer
     * @return true if the item was successfully pushed, false if the buffer is full
     */
    bool Push(const T &item)
    {
        if (IsFull())
        {
            return false;
        }
        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % N;
        return true;
    }

    /**
     * @brief Push an item into the ring buffer (move semantics)
     * @param item The item to push into the buffer
     * @return true if the item was successfully pushed, false if the buffer is full
     */
    bool Push(T &&item)
    {
        if (IsFull())
        {
            return false;
        }
        buffer_[tail_] = std::move(item);
        tail_ = (tail_ + 1) % N;
        return true;
    }

    /**
     * @brief Pop an item from the ring buffer (removes it from the buffer and returns it)
     * @return An optional containing the popped item if successful, or std::nullopt if the buffer is empty
     */
    std::optional<T> Pop()
    {
        if (IsEmpty())
        {
            return std::nullopt;
        }
        T item = std::move(buffer_[head_]);
        head_ = (head_ + 1) % N;
        return item;
    }

    /**
     * @brief Return the front item of the ring buffer without removing it
     */
    std::optional<const T &> Front() const
    {
        if (IsEmpty())
        {
            return std::nullopt;
        }
        return buffer_[head_];
    }

    /**
     * @brief Check if the ring buffer is empty
     * @return empty or not
     */
    bool IsEmpty() const
    {
        return head_ == tail_;
    }

    /**
     * @brief Check if the ring buffer is full
     * @return full or not
     */
    bool IsFull() const
    {
        return (tail_ + 1) % N == head_;
    }

    /**
     * @brief Get the capacity of the ring buffer (usable slots)
     * @return The number of usable slots in the buffer
     */
    size_t Capacity() const
    {
        return N - 1; // one slot is unused
    }

    /**
     * @brief Get the current size of the ring buffer (number of items stored)
     * @return The number of items currently in the buffer
     */
    size_t Size() const
    {
        if (tail_ >= head_)
        {
            return tail_ - head_;
        }
        return N - head_ + tail_;
    }

    /**
     * @brief Clear the ring buffer (remove all items)
     */
    void Clear()
    {
        head_ = tail_ = 0;
    }

private:
    T buffer_[N]; // actual size is N, usable capacity is N - 1
    size_t head_ = 0;
    size_t tail_ = 0;
};
}