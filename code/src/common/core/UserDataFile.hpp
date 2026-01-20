/*
MIT License

Copyright (c) 2026 Vaclav Mach (Bastl Instruments)

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
#include <cstring>
#include "common/config.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/utility/Quantizer.hpp"
#include "common/dsp/utility/TriggerGenerator.hpp"

namespace kastle2
{

/**
 * @class UserDataFile
 * @ingroup core
 * @brief Parsing user data section from flash memory into usable structures.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2026-01-15
 */
class UserDataFile
{
private:
    /** Current memory offset within the user data section */
    size_t memory_offset_ = 0;

    /** Flag indicating if the file is valid */
    bool valid_ = false;

public:
    /**
     * @brief Maximum number of scales supported
     */
    static constexpr size_t kMaxScales = 32;

    /**
     * @brief Maximum number of rhythms supported
     */
    static constexpr size_t kMaxRhythms = 64;

    /**
     * @brief Initialize and validate the file header with magic string and end marker
     * @param expected_magic 4-character magic string (e.g., "k2ac", "k2wb")
     * @param file_size_offset Offset in header where file_size is stored
     * @return true if file is valid and ready for reading
     * @warning This functions resets the memory offset to the beginning of the user data section.
     */
    bool Validate(const char *expected_magic,
                  size_t file_size_offset = 4)
    {
        valid_ = false;
        memory_offset_ = 0;

        // Read magic string from the beginning
        char magic[4];
        memcpy(magic, reinterpret_cast<const void *>(USER_DATA_SECTION_BEGIN), 4);
        if (memcmp(magic, expected_magic, 4) != 0)
        {
            return false;
        }

        // Read file size
        uint32_t file_size;
        memcpy(&file_size, reinterpret_cast<const void *>(USER_DATA_SECTION_BEGIN + file_size_offset), 4);

        // Read and validate end marker
        char end_marker[4];
        memcpy(end_marker, reinterpret_cast<const void *>(USER_DATA_SECTION_BEGIN + file_size - 4), 4);
        if (memcmp(end_marker, "ahoj", 4) != 0)
        {
            return false;
        }

        valid_ = true;
        memory_offset_ = USER_DATA_SECTION_BEGIN;
        return true;
    }

    /**
     * @brief Check if the file is valid and ready for reading
     * @return true if file was successfully validated
     */
    bool IsValid() const
    {
        return valid_;
    }

    /**
     * @brief Get current memory offset position
     * @return Current memory offset
     */
    size_t GetOffset() const
    {
        return memory_offset_;
    }

    /**
     * @brief Get pointer to current memory position
     * @return Pointer to current memory position
     */
    uint8_t *GetCurrentMemoryPointer() const
    {
        return reinterpret_cast<uint8_t *>(memory_offset_);
    }

    /**
     * @brief Jump to a specific memory offset in the user data section
     * @param offset Absolute offset from USER_DATA_SECTION_BEGIN
     */
    void JumpTo(size_t offset)
    {
        memory_offset_ = USER_DATA_SECTION_BEGIN + offset;
    }

    /**
     * @brief Advance the memory offset by specified bytes
     * @param bytes Number of bytes to advance
     */
    void Advance(size_t bytes)
    {
        memory_offset_ += bytes;
    }

    /**
     * @brief Loads scales from current memory position
     * @param num_scales Number of scales to load
     * @param max_scales Maximum allowed scales for validation
     * @return Pointer to allocated scales array, or nullptr on failure
     * @warning This function automatically advances the memory offset!
     */
    Quantizer::Scale *LoadScales(size_t num_scales, size_t max_scales = kMaxScales)
    {
        if (!valid_ || !between(num_scales, 1, max_scales))
        {
            return nullptr;
        }

        // Allocate and load scales
        Quantizer::Scale *scales = new Quantizer::Scale[num_scales];
        size_t total_bytes = sizeof(Quantizer::Scale) * num_scales;
        memcpy(scales, reinterpret_cast<const void *>(memory_offset_), total_bytes);
        memory_offset_ += total_bytes;

        return scales;
    }

    /**
     * @brief Loads rhythms from current memory position
     * @param num_rhythms Number of rhythms to load
     * @param max_rhythms Maximum allowed rhythms for validation
     * @return Pointer to allocated rhythms array, or nullptr on failure
     * @warning This function automatically advances the memory offset!
     */
    TriggerGenerator::Rhythm *LoadRhythms(size_t num_rhythms, size_t max_rhythms = kMaxRhythms)
    {
        if (!valid_ || !between(num_rhythms, 1, max_rhythms))
        {
            return nullptr;
        }

        // Allocate and load rhythms
        TriggerGenerator::Rhythm *rhythms = new TriggerGenerator::Rhythm[num_rhythms];
        size_t total_bytes = sizeof(TriggerGenerator::Rhythm) * num_rhythms;
        memcpy(rhythms, reinterpret_cast<const void *>(memory_offset_), total_bytes);
        memory_offset_ += total_bytes;

        return rhythms;
    }

    /**
     * @brief Reads multiple values from current memory position
     * @tparam T Type to read
     * @param dest Destination buffer
     * @param byte_count Number of bytes to read
     * @warning This function automatically advances the memory offset!
     */
    template <typename T>
    void Read(T *dest, size_t byte_count)
    {
        if (!valid_)
        {
            return;
        }
        memcpy(dest, reinterpret_cast<const void *>(memory_offset_), byte_count);
        memory_offset_ += byte_count;
    }
};

} // namespace kastle2