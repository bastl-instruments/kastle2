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

#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

#pragma once

/**
 * @class EnumArray
 * @ingroup utils
 * @brief EnumArray: A safe and efficient enum-indexed array.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-04-25
 * @note The enum must have a COUNT value to determine the size of the array.
 * @tparam Enum Enum type used as the array index.
 * @tparam T Type of the elements stored in the array.
 * @tparam Size Size of the array, defaulting to the number of enum values.
 */
template <typename Enum, typename T, size_t Size = std::to_underlying(Enum::COUNT)>
class EnumArray
{
public:
    using value_type = T;                   ///< Type of the elements in the array.
    using array_type = std::array<T, Size>; ///< Underlying array type.

    /**
     * @brief Default constructor. Initializes all elements with their default values.
     */
    EnumArray() = default;

    /**
     * @brief Constructor that initializes the array with an initializer list.
     * Delegates to the safer variadic template constructor.
     *
     * @param init Initializer list of values to fill the array with.
     */
    EnumArray(std::initializer_list<T> init)
    {
        // Runtime check that will assert if the sizes don't match
        if (init.size() != Size)
        {
            // Using static_assert-like message but with runtime check
            // This will only be caught at runtime
            assert(init.size() == Size && "Initializer list size does not match enum size");
        }

        auto it = init.begin();
        for (size_t i = 0; i < Size && it != init.end(); ++i, ++it)
        {
            data_[i] = *it;
        }
    }

    /**
     * @brief Accesses the element at the given enum index.
     *
     * @param e Enum value used as the index.
     * @return Reference to the element at the given index.
     */
    T &operator[](Enum e)
    {
        return data_[to_index(e)];
    }

    /**
     * @brief Accesses the element at the given enum index (const version).
     *
     * @param e Enum value used as the index.
     * @return Const reference to the element at the given index.
     */
    const T &operator[](Enum e) const
    {
        return data_[to_index(e)];
    }

    /**
     * @brief Returns an iterator to the beginning of the array.
     *
     * @return Iterator to the beginning.
     */
    auto begin() { return data_.begin(); }

    /**
     * @brief Returns an iterator to the end of the array.
     *
     * @return Iterator to the end.
     */
    auto end() { return data_.end(); }

    /**
     * @brief Returns a const iterator to the beginning of the array.
     *
     * @return Const iterator to the beginning.
     */
    auto begin() const { return data_.begin(); }

    /**
     * @brief Returns a const iterator to the end of the array.
     *
     * @return Const iterator to the end.
     */
    auto end() const { return data_.end(); }

    /**
     * @brief Fills the array with the specified value.
     *
     * @param value Value to fill the array with.
     */
    void fill(const T &value)
    {
        data_.fill(value);
    }

    /**
     * @brief Returns a pointer to the underlying data array.
     * @return Pointer to the underlying data array.
     */
    T *data()
    {
        return data_.data();
    }

    /**
     * @brief Accesses the element at the specified index.
     * @param index Index of the element to access.
     */
    T &at(size_t index)
    {
        return data_.at(index);
    }

    /**
     * @brief Accesses the element at the specified index.
     * @param index Index of the element to access.
     */
    const T &at(size_t index) const
    {
        return data_.at(index);
    }

    constexpr size_t size() const
    {
        return Size;
    }

private:
    /**
     * @brief Converts an enum value to its corresponding index.
     *
     * @param e Enum value to convert.
     * @return Index corresponding to the enum value.
     */
    static constexpr auto to_index(Enum e)
    {
        return std::to_underlying(e);
    }

    array_type data_{}; ///< Underlying array storage.
};

/**
 * @class EnumRange
 * @ingroup utils
 * @brief EnumRange: A range-based utility for iterating over enum values.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-04-25
 * @tparam Enum Enum type to iterate over.
 */
template <typename Enum>
class EnumRange
{
public:
    /**
     * @brief Iterator for EnumRange.
     */
    class Iterator
    {
    public:
        /**
         * @brief Constructs an iterator starting at the given index.
         *
         * @param index Starting index for the iterator.
         */
        explicit Iterator(size_t index) : index_(index) {}

        /**
         * @brief Dereferences the iterator to get the current enum value.
         *
         * @return Enum value at the current iterator position.
         */
        Enum operator*() const { return static_cast<Enum>(index_); }

        /**
         * @brief Advances the iterator to the next position.
         *
         * @return Reference to the updated iterator.
         */
        Iterator &operator++()
        {
            ++index_;
            return *this;
        }

        /**
         * @brief Compares two iterators for inequality.
         *
         * @param other Iterator to compare with.
         * @return True if the iterators are not equal, false otherwise.
         */
        bool operator!=(const Iterator &other) const { return index_ != other.index_; }

    private:
        size_t index_; ///< Current index of the iterator.
    };

    /**
     * @brief Returns an iterator to the beginning of the enum range.
     *
     * @return Iterator to the beginning.
     */
    Iterator begin() const { return Iterator(0); }

    /**
     * @brief Returns an iterator to the end of the enum range.
     *
     * @return Iterator to the end.
     */
    Iterator end() const { return Iterator(std::to_underlying(Enum::COUNT)); }
};

/**
 * @brief Increments an enum value and wraps around when it reaches COUNT
 * @tparam EnumType The enum type
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-04-25
 * @note The enum type must have a COUNT value to determine the size of the enum.
 * @param value Current enum value
 * @return The next enum value, or first value if at the end
 */
template <typename EnumType>
EnumType EnumIncrement(EnumType value)
{
    return static_cast<EnumType>((std::to_underlying(value) + 1) % std::to_underlying(EnumType::COUNT));
}
