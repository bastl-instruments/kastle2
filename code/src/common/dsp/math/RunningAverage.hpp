/*
MIT License

Copyright (c) 2024 Vaclav Mach (Bastl Instruments)
Copyright (c) 2025 Marek Mach (Bastl Instruments)

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

#include <algorithm>
#include <memory>

namespace kastle2
{

/**
 * @class RunningAverage
 * @ingroup dsp_math
 * @brief Calculating the running average and median of a set of values in a circular buffer.
 * @details Pass size of the buffer in the constructor. Add values with Add() method.
 *          Optionally set the resetting threshold with SetResetThreshold() method for instantaneous response to larger changes.
 * @tparam T Usually float or int32_t etc.
 * @author Vaclav Mach (Bastl Instruments), Marek Mach (Bastl Instruments)
 * @date 2024-07-31
 */
template <typename T>
class RunningAverage
{
public:
    /**
     * @brief Default constructor for containers that require default constructibility
     */
    RunningAverage() : size_(0), count_(0), index_(0), reset_threshold_(T{}), reset_threshold_set_(false), values_(nullptr) {}

    /**
     * @brief Construct a new Running Average object
     * @param size The size of the buffer.
     * @param reset_threshold The threshold for resetting the buffer. If the difference between the current average and the new value exceeds this threshold, the buffer is reset.
     * @note If reset_threshold is set to 0, the resetting is disabled.
     */
    RunningAverage(size_t size, T reset_threshold = T{}) : size_(size), count_(0), index_(0)
    {
        values_ = std::make_unique<T[]>(size_);
        SetResetThreshold(reset_threshold);
    }

    /**
     * @brief Copy constructor
     * @param other The object to copy from
     */
    RunningAverage(const RunningAverage &other) : size_(other.size_),
                                                  count_(other.count_),
                                                  index_(other.index_),
                                                  reset_threshold_(other.reset_threshold_),
                                                  reset_threshold_set_(other.reset_threshold_set_)
    {
        if (size_ > 0)
        {
            values_ = std::make_unique<T[]>(size_);
            for (size_t i = 0; i < size_; i++)
            {
                values_[i] = other.values_[i];
            }
        }
    }

    /**
     * @brief Move constructor
     * @param other The object to move from
     */
    RunningAverage(RunningAverage &&other) noexcept : size_(other.size_),
                                                      count_(other.count_),
                                                      index_(other.index_),
                                                      reset_threshold_(other.reset_threshold_),
                                                      reset_threshold_set_(other.reset_threshold_set_),
                                                      values_(std::move(other.values_))
    {
        other.size_ = 0;
        other.count_ = 0;
        other.index_ = 0;
        other.reset_threshold_ = T{};
        other.reset_threshold_set_ = false;
    }

    /**
     * @brief Copy assignment operator
     * @param other The object to copy from
     * @return Reference to this object
     */
    RunningAverage &operator=(const RunningAverage &other)
    {
        if (this != &other)
        {
            size_ = other.size_;
            count_ = other.count_;
            index_ = other.index_;
            reset_threshold_ = other.reset_threshold_;
            reset_threshold_set_ = other.reset_threshold_set_;

            if (size_ > 0)
            {
                values_ = std::make_unique<T[]>(size_);
                for (size_t i = 0; i < size_; i++)
                {
                    values_[i] = other.values_[i];
                }
            }
            else
            {
                values_.reset();
            }
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     * @param other The object to move from
     * @return Reference to this object
     */
    RunningAverage &operator=(RunningAverage &&other) noexcept
    {
        if (this != &other)
        {
            size_ = other.size_;
            count_ = other.count_;
            index_ = other.index_;
            reset_threshold_ = other.reset_threshold_;
            reset_threshold_set_ = other.reset_threshold_set_;
            values_ = std::move(other.values_);

            other.size_ = 0;
            other.count_ = 0;
            other.index_ = 0;
            other.reset_threshold_ = T{};
            other.reset_threshold_set_ = false;
        }
        return *this;
    }

    /**
     * @brief Sets the resetting threshold.
     * @param threshold The resetting threshold value, setting it to 0 disables resetting.
     */
    void SetResetThreshold(T threshold)
    {
        reset_threshold_ = threshold;
        reset_threshold_set_ = true;
        if constexpr (std::is_floating_point_v<T>)
        {
            if (threshold <= 0.0f)
            {
                reset_threshold_set_ = false; // disable resetting if threshold is zero or negative
            }
        }
        else if constexpr (std::is_signed_v<T>)
        {
            if (threshold <= 0)
            {
                reset_threshold_set_ = false; // disable resetting if threshold is zero or negative
            }
        }
        else
        {
            if (threshold <= 0)
            {
                reset_threshold_set_ = false; // disable resetting if threshold is zero
            }
        }
    }

    /**
     * @brief Resets the buffer.
     */
    void Reset()
    {
        count_ = 0;
        index_ = 0;
    }

    /**
     * @brief Checks if the buffer is empty.
     * @return True if the buffer is empty, false otherwise.
     */
    bool IsEmpty() const
    {
        return count_ == 0;
    }

    /**
     * @brief Gets the count of the items (maxes out at size).
     * @return The count of the items.
     */
    size_t GetCount() const
    {
        return count_;
    }

    /**
     * @brief Adds a value to the circular buffer.
     * @param value Usually float or int32_t etc.
     */
    void Add(T value)
    {
        if (reset_threshold_set_)
        {
            T avg = GetAverage();
            T diff = (value > avg) ? (value - avg) : (avg - value);
            if (diff > reset_threshold_)
            {
                Reset();
            }
        }
        values_[index_] = value;
        index_ = (index_ + 1) % size_;
        if (count_ < size_)
        {
            count_++;
        }
    }

    /**
     * @brief Gets the average of the values in the circular buffer.
     * @return The average of the values.
     */
    T GetAverage() const
    {
        T sum = 0;
        for (size_t i = 0; i < count_; i++)
        {
            sum += values_[i];
        }
        return count_ > 0 ? sum / count_ : 0;
    }

    /**
     * @brief Gets the median of the values in the circular buffer.
     * @return The median of the values.
     */
    T GetMedian() const
    {
        if (count_ == 0)
        {
            return 0;
        }

        // Create a temporary array to hold the values
        std::unique_ptr<T[]> temp_values = std::make_unique<T[]>(count_);
        std::copy(values_.get(), values_.get() + count_, temp_values.get());

        // Sort the temporary array
        std::sort(temp_values.get(), temp_values.get() + count_);

        // Calculate the median
        if (count_ % 2 == 0)
        {
            return (temp_values[count_ / 2 - 1] + temp_values[count_ / 2]) / 2;
        }
        else
        {
            return temp_values[count_ / 2];
        }
    }

private:
    size_t size_ = 0;
    size_t count_ = 0;
    size_t index_ = 0;
    T reset_threshold_ = T{};
    bool reset_threshold_set_ = false;
    std::unique_ptr<T[]> values_;
};
}
