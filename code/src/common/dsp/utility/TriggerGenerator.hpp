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

#include <array>
#include <bitset>
#include <span>
#include "common/dsp/math/bit_utils.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

/**
 * @class TriggerGenerator
 * @ingroup dsp_utility
 * @brief Generates triggers based on the Q15 input value and selected type.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-05-29
 *
 * There are several types of trigger generators:
 * - TABLE: Patterns from predefined table
 * - DIVIDER: Sets the triggers on 1/1, 1/2, 1/4 steps etc.
 * - EUCLID: Euclidian rhythm generator
 * - RANDOM: Random trigger generator (higher input, more triggers)
 *
 * The code is based on various generators contained in other Bastl Instruments projects (Little Nerd, Kompas, Neo Trinity)
 *
 */
class TriggerGenerator
{

public:
    /**
     * @brief Rhythm type - MSB First uint32_t bitmask, length 16
     * @note The 32-bit value is used because of the ARM memory alignment.
     */
    typedef uint32_t Rhythm;

    /**
     * @brief Types of the trigger generator (excluding table based since that you need table for).
     */
    enum class Type
    {
        DIVIDER, ///< Sets the triggers on 1/1, 1/2, 1/4 steps etc.
        EUCLID,  ///< Euclidian rhythm generator
        RANDOM,  ///< Random trigger generator (higher input, more triggers)
        COUNT    ///< Number of types available
    };

    /**
     * @brief Generates triggers based on the Q15 input value and selected type.
     * @param type Type of the trigger generator
     * @param input Q15 input value (0-32767)
     * @param triggers Bitset reference to store the triggers
     * @param length Length of the active portion of the bitset
     * @note Use GenerateTriggersUsingTable for table based generation.
     */
    template <size_t N>
    static void GenerateTriggers(Type type, q15_t input, std::bitset<N> &triggers, size_t length)
    {
        switch (type)
        {
        case Type::DIVIDER:
            DividerTriggerGenerator(input, triggers, length);
            break;
        case Type::EUCLID:
            EuclidianTriggerGenerator(input, triggers, length);
            break;
        case Type::RANDOM:
            RandomTriggerGenerator(input, triggers, length);
            break;
        }
    }

    /**
     * @brief Clears the triggers bitset.
     * @param triggers Bitset reference to store the triggers
     * @param length Length of the active portion of the bitset
     */
    template <size_t N>
    static void ClearTriggers(std::bitset<N> &triggers, size_t length)
    {
        for (size_t i = 0; i < length && i < N; i++)
        {
            triggers[i] = false;
        }
    }

    /**
     * @brief Fills the triggers bitset.
     * @param triggers Bitset reference to store the triggers
     * @param length Length of the active portion of the bitset
     */
    template <size_t N>
    static void FillTriggers(std::bitset<N> &triggers, size_t length)
    {
        for (size_t i = 0; i < length && i < N; i++)
        {
            triggers[i] = true;
        }
    }

    /**
     * @brief Generates triggers based on the Q15 input value provided table.
     * @param input Q15 input value (0-32767)
     * @param triggers Bitset reference to store the triggers
     * @param length Length of the active portion of the bitset
     * @param table Table of patterns
     */
    template <size_t N>
    static void PatternTriggerGenerator(q15_t input, std::bitset<N> &triggers, size_t length, std::span<const uint32_t> table)
    {
        ClearTriggers(triggers, length);
        size_t index = constrain(map(input, 0, Q15_MAX, 0, table.size()), 0, table.size() - 1);
        for (size_t i = 0; i < length && i < N; i++)
        {
            triggers[i] = bit_read(table[index], kGeneratorPatternLength - 1 - i % kGeneratorPatternLength);
        }
    }

    /**
     * @brief Generates divider of 2 triggers based on the Q15 input value.
     * @param input Q15 input value (0-32767)
     * @param triggers Bitset reference to store the triggers
     * @param length Length of the active portion of the bitset
     */
    template <size_t N>
    static void DividerTriggerGenerator(q15_t input, std::bitset<N> &triggers, size_t length)
    {
        ClearTriggers(triggers, length);

        // find max exp
        uint32_t max_exp = 1;
        for (uint32_t i = 0; i < length; i++)
        {
            if ((uint32_t)(1 << i) <= length)
            {
                max_exp = i;
            }
            else
            {
                break;
            }
        }
        uint32_t found_exp = map(input, 0, Q15_MAX, 0, max_exp + 1);
        for (size_t i = 0; i < length && i < N; i++)
        {
            // steps - i because triggers are MSB first
            triggers[i] = ((length - i) % (1 << found_exp)) == 0;
        }
    }

    /**
     * @brief Generates Euclidian rhythm based on the Q15 input value.
     * @param input Q15 input value (0-32767)
     * @param triggers Bitset reference to store the triggers
     * @param length Length of the active portion of the bitset
     */
    template <size_t N>
    static void EuclidianTriggerGenerator(q15_t input, std::bitset<N> &triggers, size_t length)
    {
        ClearTriggers(triggers, length);
        size_t fills = map(input, 0, Q15_MAX, 1, length);
        if (fills >= length - 1)
        {
            // fill all
            FillTriggers(triggers, length);
            return;
        }

        // fill first to none
        triggers[0] = false;

        float coordinate = (float)length / (float)fills;
        float whereFloat = 0;
        while (whereFloat < length)
        {
            size_t where = (size_t)whereFloat;
            if ((whereFloat - where) >= 0.5)
            {
                where++;
            }
            if (where > length - 1)
            {
                where = length - 1;
            }
            if (where < N)
            {
                triggers[where] = true;
            }
            whereFloat += coordinate;
        }
    }

    /**
     * @brief Generates random triggers based on the Q15 input value.
     * @param input Q15 input value (0-32767)
     * @param triggers Bitset reference to store the triggers
     * @param length Length of the active portion of the bitset
     */
    template <size_t N>
    static void RandomTriggerGenerator(q15_t input, std::bitset<N> &triggers, size_t length)
    {
        ClearTriggers(triggers, length);
        bool any_trig = false;
        for (size_t i = 0; i < length && i < N; i++)
        {
            bool trig = (rand() % Q15_MAX) <= input;
            if (trig)
            {
                any_trig = true;
            }
            triggers[i] = trig;
        }
        if (!any_trig && length > 0)
        {
            size_t random_pos = rand() % length;
            if (random_pos < N)
            {
                triggers[random_pos] = true;
            }
        }
    }

    /**
     * @brief Length of the generator pattern that you supply the table for.
     * @see kExampleTable
     */
    static constexpr size_t kGeneratorPatternLength = 16;

    /**
     * @brief Example pattern table.
     * Just 16 steps which are repeated for 32, 64 steps if necessary
     * MSB format (first step is left)
     */
    static constexpr std::array<uint32_t, 2> kExampleTable = {
        0b1000100010001000,
        0b1010101010101010};

private:
};

}
