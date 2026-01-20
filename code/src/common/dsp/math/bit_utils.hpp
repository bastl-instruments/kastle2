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

namespace kastle2
{

/**
 * @file bit_utils.h
 * @ingroup dsp_math
 * @brief Bit manipulation utilities.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-05-30
 * 
 * Bit manipulation utilities.
 */

/**
 * @brief Reads a bit from a number.
 */
#define bit_read(value, bit) (((value) >> (bit)) & 0x01)

/**
 * @brief Sets a bit in a number to 1.
 */
#define bit_set(value, bit) ((value) |= (1UL << (bit)))

/**
 * @brief Clears a bit in a number to 0.
 */
#define bit_clear(value, bit) ((value) &= ~(1UL << (bit)))

/**
 * @brief Sets a bit in a number to a 0 or 1 based on provided bitvalue.
 */
#define bit_write(value, bit, bitvalue) (bitvalue ? bit_set(value, bit) : bit_clear(value, bit))
}
