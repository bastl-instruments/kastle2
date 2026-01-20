/*
MIT License

Copyright (c) 2020 Electrosmith, Corp
Copyright (c) 2024 Marek Mach (Bastl Instruments)
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
#include <cstdint>
#include <memory>

#define SLEW_TYPE_SLOW 1
#define SLEW_TYPE_FAST 2
#define SLEW_TYPE SLEW_TYPE_FAST

namespace kastle2
{

/**
 * @class AdvancedDynamicDelayLine
 * @ingroup dsp_utility
 * @brief Dynamically allocated delay line using heap memory, with lot of additional features.
 * @author Marek Mach (Bastl Instruments), Vaclav Mach (Bastl Instruments)
 * @date 2024-05-30
 *
 * @note You need to create it using `new` keyword and delete it using `delete` keyword when you don't need it.
 *       Be careful with the RAM usage, it will crash if you try to allocate too much memory.
 *
 * There are two pointers (write pointer, read pointer), they independently increment with each write/read.
 * Be careful and store the read value (can't read twice the same sample).
 *
 * For more creative control, you can use:
 *  -  SetOversampling()  which allows you to increment the buffer index faster or slower thus enabling you to speed up or slow down recorded data
 *  -  SetLengthBoth()    (and other variations) which allows you to make shorter loops without speeding them up, just cutting off the ends
 *                        (those ends don't get deleted so you can make the short and then long again)
 *  -  SetDelay()         which sets the distance between the read and write heads
 *  -  SetDelaySnap()     which does the same thing as SetDelay() but without smoothing - you get a weird crackly sound that mimics granular time stretching
 *  -  WriteEmpty()       which works like Write() but doesn't rewrite the existing data, just moves the index
 *  -  SetReverse()       which allows you to change the direction of the index - if the buffer is not being recorded to and is jus playing, it will play backwards
 *                      recording to it however will fill it up backwards so it will sound normal again until you set the reverse to false
 *
 * Loosely based on `DelayLine` from DaisySP by shensley, Electrosmith listed under MIT License.
 */
template <typename T>
class AdvancedDynamicDelayLine
{

private:
    // this weird thing allows us to play with 64bit numbers without a speed penalty
    typedef union
    {
        uint64_t big;
        struct
        {
            uint32_t bottom;
            uint32_t top;
        } parts;
    } expanded_t;

    inline void IncrementPointer(expanded_t *ptr, expanded_t *inc, const size_t length, const bool reverse)
    {
        if (IsOversampling(inc))
        {
            // prevent overflow - unnecessary given the size of our ram, uncomment if the pointer can get close to the 32bit maximum
            /*if (ptr->parts.top < 0xFFFFFFFF)
            {
                ptr->big += inc->parts.bottom;
            }*/

            if (reverse)
            {
                ptr->big -= inc->parts.bottom; // worth 2%
            }
            else
            {
                ptr->big += inc->parts.bottom;
            }
        }
        if (reverse)
        {
            ptr->parts.top = ((length + ptr->parts.top) - inc->parts.top) % length;
        }
        else
        {
            ptr->parts.top = ((length + ptr->parts.top) + inc->parts.top) % length;
        }
    }

public:
    /**
     * @brief  0x00010000 is 1-1 sampling (default), below is undersampling, above is oversampling
     */
    static constexpr uint32_t kDefaultSampling = 0x00010000;

    /**
     * @brief Prepares the delay line and allocates memory
     * @param max_length The size of the delay line
     */
    AdvancedDynamicDelayLine(const size_t max_length)
    {
        max_length_ = max_length;
        line_ = std::make_unique<T[]>(max_length_);
        length_read_ = max_length_;
        length_write_ = max_length_;
        Reset();
    }

    /**
     * @brief Returns the max length of the delay buffer
     * @return Buffer max length in size_t
     */
    size_t inline GetMaxLength() const
    {
        return max_length_;
    }

    /**
     * @brief Sets the Length of the record and playback buffer to the entire actual buffer
     */
    void inline SetLengthBothToMax()
    {
        length_read_ = max_length_;
        length_write_ = max_length_;
    }

    /**
     * @brief Sets the Length of the playback buffer to the entire actual buffer, keeping the record buffer length unchanged
     * @note This WILL case the buffer to behave unpredictably if used incorrectly (use SetLengthBothToMax() instead)
     */
    void inline SetLengthReadToMax()
    {
        length_read_ = max_length_;
    }

    /**
     * @brief Sets the Length of the record buffer to the entire actual buffer, keeping the playback buffer length unchanged
     * @note This WILL case the buffer to behave unpredictably if used incorrectly (use SetLengthBothToMax() instead)
     */
    void inline SetLengthWriteToMax()
    {
        length_write_ = max_length_;
    }

    /**
     * @brief Sets the Length of the record and playback buffer (can be changed on the fly, unlike max_length)
     * @param length Buffer length in samples
     */
    void inline SetLengthBoth(const size_t length)
    {
        length_read_ = length <= max_length_ ? length : max_length_;
        length_write_ = length <= max_length_ ? length : max_length_;
    }

    /**
     * @brief Sets the Length of the playback buffer while leaving the record buffer as it was.
     * @note This WILL case the buffer to behave unpredictably if used incorrectly (use SetLengthBoth() instead)
     * @param length Buffer length in samples
     */
    void inline SetLengthRead(const size_t length)
    {
        length_read_ = length <= max_length_ ? length : max_length_;
    }

    /**
     * @brief Sets the Length of the record buffer while leaving the playback buffer as it was.
     * @note This WILL case the buffer to behave unpredictably if used incorrectly (use SetLengthBoth() instead)
     * @param length Buffer length in samples
     */
    void inline SetLengthWrite(const size_t length)
    {
        length_write_ = length <= max_length_ ? length : max_length_;
    }

    /**
     * @brief Returns the length of the playback buffer, which can differ from record buffer length
     * @return Playback loop length in samples
     */
    size_t inline GetLengthRead() const
    {
        return length_read_;
    }

    /**
     * @brief Returns the length of the record buffer, which can differ from playback buffer length
     * @return Record loop length in samples
     */
    size_t inline GetLengthWrite() const
    {
        return length_write_;
    }

    /**
     * @brief Calling this function resets a counter that counts how many samples have been recorded.
     *        Use along with getRecordingProgress() to get how many samples have been recorded since calling this function.
     * @note This is about actual samples of the buffer (oversampling will make the count go up faster, undersampling slower).
     */
    void StartRecordProgress()
    {
        recorded_samples_.big = 0;
    }

    /**
     * @brief Returns the number of actual samples recorded into the buffer since StartRecordProgress() was called.
     * @note This is about actual samples of the buffer (oversampling will make the count go up faster, undersampling slower).
     *       This function can return a maximum of 4294967295 (0xFFFFFFFF) samples counted so be ready to deal with overflows.
     * @return Number of actual samples recorded to the buffer since calling StartRecordProgress().
     */
    size_t GetRecordProgress() const
    {
        return recorded_samples_.parts.top;
    }

    /**
     * @brief Clears buffer, sets write pointer to 0, and delay to 1 sample
     */
    void Reset()
    {
        for (size_t i = 0; i < max_length_; i++)
        {
            line_[i] = T(0);
        }
        write_ptr_.big = 0;
        write_ptr_prev_ = 0;
        delay_ = 1;
        write_ptr_inc_.parts.top = 1;
        write_ptr_inc_.parts.bottom = 0;
        reverse_ = false;
        length_read_ = max_length_;
        length_write_ = max_length_;
        recorded_samples_.big = 0;
        SyncReadPointer();
    }

    /**
     * @brief Sets the delay time in samples
     * @param delay The delay time in samples
     */
    inline void SetDelay(const size_t delay)
    {
        delay_ = delay < max_length_ ? delay : max_length_ - 1;
        // if it is reversed, invert the delay amount to keep the behavior identical
        if (reverse_)
        {
            delay_ = max_length_ - delay_;
        }
    }

    /**
     * @brief Sets the delay time in samples directly, without slew limiting
     * @param delay The delay time in samples
     */
    inline void SetDelaySnap(const size_t delay)
    {
        delay_smooth_.parts.top = delay < max_length_ ? delay : max_length_ - 1;
        delay_ = delay;
        // if it is reversed, invert the delay amount to keep the behavior identical
        if (reverse_)
        {
            delay_smooth_.parts.top = max_length_ - delay_smooth_.parts.top;
            delay_ = delay_smooth_.parts.top;
        }
    }

    /**
     * @brief Gets the exact delay value, after smoothing
     * @return The delay time in samples
     */
    inline size_t GetDelay() const
    {
        return delay_smooth_.parts.top;
    }

    /**
     * @brief Sets the amount of over / under sampling (oversampling records some samples multiple times, undersampling doesn't store all samples)
     * @param speed Sets the write pointer increment (0x00010000 is 1-1 sampling, below is undersampling, above is oversampling)
     * @note be modest with the oversampling, each skipped sample still has to get written
     */
    inline void SetOversampling(const uint32_t speed)
    {
        write_ptr_inc_.big = (uint64_t)speed << 16;
        SyncReadPointer();
    }

    /**
     * @brief Selects if buffer index is incrementing or decrementing (used for playing recorded audio backwards)
     * @param reverse Set to false means index is incrementing (slightly better optimized), true means it decrements
     */
    inline void SetReverse(const bool reverse)
    {
        reverse_ = reverse;
    }

    /**
     * @brief Returns if buffer index in incrementing or decrementing
     * @return False means index is incrementing, true means it decrements
     */
    inline bool GetReverse() const
    {
        return reverse_;
    }

    /**
     * @brief Writes the sample of type T to the delay line, and advances the write pointer
     * @param sample The sample to write
     */
    void Write(const T sample)
    {
        if (!IsOversampling(&write_ptr_inc_))
        {
            // Simple implement without oversampling
            IncrementPointer(&write_ptr_, &write_ptr_inc_, length_write_, reverse_);
            recorded_samples_.big += write_ptr_inc_.big; // simple incrementing is required
            line_[write_ptr_.parts.top] = sample;
        }
        else
        {
            // Special incrementing only oversampling is used
            write_ptr_prev_ = write_ptr_.parts.top;

            IncrementPointer(&write_ptr_, &write_ptr_inc_, length_write_, reverse_);
            recorded_samples_.big += write_ptr_inc_.big; // simple incrementing is required

            if (reverse_)
            {
                while (write_ptr_prev_ >= write_ptr_.parts.top) // worth 6% - maybe could be optimized
                {
                    line_[write_ptr_prev_] = sample;
                    // prevent underflow (might cause artifacts but I don't hear any)
                    if (write_ptr_prev_ == 0)
                        break;
                    write_ptr_prev_--;
                }
            }
            else
            {
                while (write_ptr_prev_ <= write_ptr_.parts.top)
                {
                    line_[write_ptr_prev_++] = sample;
                }
            }
        }
    }

    static inline bool IsOversampling(expanded_t *inc)
    {
        return !(inc->parts.top == 1 && !inc->parts.bottom);
    }

    /**
     * @brief Does not write anything, just advances the write pointer (useful for looping)
     */
    void WriteEmpty()
    {
        IncrementPointer(&write_ptr_, &write_ptr_inc_, length_write_, reverse_);
        recorded_samples_.big += write_ptr_inc_.big; // simple incrementing is required
        SyncReadPointer();
    }

    /**
     * @brief Get a sample, it's distance (delay) from the newest is set by SetDelay()
     * @return A single sample from the delay line
     */
    T Read()
    {
        // Increment read pointer
        IncrementPointer(&read_ptr_, &write_ptr_inc_, length_read_, reverse_);

        // Calculate the delay position pointer (slew limited)
#if SLEW_TYPE == SLEW_TYPE_SLOW
        delay_smooth_.big = (static_cast<uint64_t>(delay_) << 20) + ((delay_smooth_.big * 4095) >> 12);
#elif SLEW_TYPE == SLEW_TYPE_FAST
        delay_smooth_.big = (static_cast<uint64_t>(delay_) << 22) + ((delay_smooth_.big * 1023) >> 10);
#endif

        // For short delays, make sure they are always correct
        if (delay_ <= kShortDelay && delay_smooth_.parts.top > delay_ && delay_smooth_.parts.top - delay_ < 10 && delay_smooth_.parts.top - delay_ > 0)
        {
            delay_smooth_.parts.top++;
        }
        T a = line_[(length_read_ + read_ptr_.parts.top - (delay_smooth_.parts.top)) % length_read_];
        return a;
    }

    /**
     * @brief Syncs the read pointer to the write pointer position
     */
    inline void SyncReadPointer()
    {
        read_ptr_ = write_ptr_;
    }

    /**
     * @brief Syncs the write pointer to the read pointer position
     */
    inline void SyncWritePointer()
    {
        write_ptr_ = read_ptr_;
    }

private:
    expanded_t write_ptr_ = {0};
    size_t write_ptr_prev_ = 0;
    expanded_t write_ptr_inc_ = {0};
    expanded_t read_ptr_ = {0};
    size_t delay_ = 0;
    expanded_t delay_smooth_ = {0};
    size_t max_length_ = 0;
    size_t length_read_ = 0;
    size_t length_write_ = 0;
    expanded_t recorded_samples_ = {0};
    bool reverse_ = false;
    std::unique_ptr<T[]> line_;

    static constexpr size_t kShortDelay = 48; // do corrections only for delays shorter than this
};
}
