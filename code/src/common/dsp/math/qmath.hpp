/*
MIT License

Copyright (c) 2024 Vaclav Mach (Bastl Instruments)
Copyright (c) 2024 Marek Mach (Bastl Instruments)

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
#include "lookup_qmath_sine.hpp"
#include "math_utils.hpp"

namespace kastle2
{

/**
 * @file qmath.h
 * @ingroup dsp_math
 * @brief Fast fixed point math library.
 * @author Vaclav Mach (Bastl Instruments), Marek Mach (Bastl Instruments)
 * @date 2024-04-12
 *
 * This library provides fast fixed point arithmetic operations.
 * It includes functions and macros for addition, multiplication, and scaling of fixed point numbers.
 *
 */

/** @brief Fixed point 32-bit number (1-bit sign, 31-bit number) stored in 32 bits. */
using q31_t = int32_t;

static constexpr q31_t Q31_MAX = INT32_MAX;
static constexpr q31_t Q31_MIN = INT32_MIN;
static constexpr q31_t Q31_HALF = ((q31_t)0x3fffffffL);
static constexpr q31_t Q31_ZERO = ((q31_t)0u);

/**
 * @brief Constructs a Q1.31 fixed-point integer (`q31_t`) from a floating-point literal.
 *
 * @tparam T Must be `float` or `double`.
 * @param x A floating-point value in the range [-1.0, 1.0].
 * @return int32_t The fixed-point Q1.31 representation.
 *
 * @note
 * - Can only be used at compile time.
 * - The input must be a `float` or `double` literal (e.g., `q31(0.5f)` or `q31(0.5)`).
 * - `1.0` is mapped to INT32_MAX (2147483647).
 * - Values outside [-1.0, 1.0] will be constrained to this range.
 */
template <typename T>
consteval q31_t q31(const T x)
{
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                  "q31() requires float or double literal (e.g., q31(0.f) or q31(0.0))");
    float y = x > 1.0f ? 1.0f : (x < -1.0f ? -1.0f : x);
    if (y == 1.0)
    {
        return std::numeric_limits<int32_t>::max();
    }
    return static_cast<q31_t>(y * 2147483648.0);
}

/**
 * @brief Inverts a Q31 number.
 * @param x The input number.
 * @return The inverted number.
 */
inline constexpr q31_t q31_inv(const q31_t x)
{
    // Handle overflow
    if (x == Q31_MIN)
    {
        return Q31_MAX;
    }
    return Q31_MAX - x;
}

/**
 * @brief Saturates a fixed point number.
 */
inline constexpr q31_t q31_saturate(const int64_t x)
{
    return x > Q31_MAX ? Q31_MAX : ((x) < Q31_MIN ? Q31_MIN : (q31_t)x);
}

/** @brief Fixed point 16-bit number (1-bit sign, 15-bit number) stored in 16 bits to save RAM. */
using q15least_t = int16_t;

/** @brief Fixed point 16-bit number (1-bit sign, 15-bit number) stored in 32 bits to prevent padding overhead. */
using q15_t = int32_t;

static constexpr q15_t Q15_MAX = INT16_MAX;
static constexpr q15_t Q15_MIN = INT16_MIN;
static constexpr q15_t Q15_HALF = (q15_t)0x3fffL;
static constexpr q15_t Q15_ZERO = (q15_t)0u;

/**
 * @brief Constructs a Q1.15 fixed-point integer (`q15_t`) from a floating-point literal.
 *
 * @tparam T Must be `float` or `double`.
 * @param x A floating-point value in the range [-1.0, 1.0].
 * @return int16_t The fixed-point Q1.15 representation.
 *
 * @note
 * - Can only be used at compile time.
 * - The input must be a `float` or `double` literal (e.g., `q15(0.25f)` or `q15(0.25)`).
 * - `1.0` is mapped to INT16_MAX (32767).
 * - Values outside [-1.0, 1.0] will be constrained to this range.
 */
template <typename T>
consteval q15_t q15(const T x)
{
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                  "q15() requires float or double literal (e.g., q15(0.f) or q15(0.0))");
    float y = x > 1.0f ? 1.0f : (x < -1.0f ? -1.0f : x);
    if (y == 1.0)
    {
        return std::numeric_limits<int16_t>::max();
    }
    return static_cast<q15_t>(y * 32768.0);
}

/**
 * @brief Saturates a fixed point number.
 */
inline constexpr q15_t q15_saturate(const q15_t x)
{
    return x > Q15_MAX ? Q15_MAX : (x < Q15_MIN ? Q15_MIN : x);
}

/**
 * @brief Inverts a Q15 number.
 * @param x The input number.
 * @return The inverted number.
 */
inline constexpr q15_t q15_inv(const q15_t x)
{
    q15_t y = Q15_MAX - x;
    return q15_saturate(y);
}

/**
 * @brief Converts a q15_t to a q31_t.
 */
inline constexpr q31_t q15_to_q31(const q15_t x)
{
    return ((q31_t)(x)) << 16;
}

/**
 * @brief Converts a q31_t to a q15_t.
 */
inline constexpr q15_t q31_to_q15(const q31_t x)
{
    return (x) >> 16;
}

/**
 * @brief Calculates the absolute value of a q15_t number. Handles problem with -32,768.
 * @note q15_t is stored as 32-bit integer so we don't need to cast it.
 *
 * @param x The input number.
 * @return The absolute value of the input number.
 */
inline constexpr q15_t q15_abs(const q15_t x)
{
    return q15_saturate(x > 0 ? x : -x);
}

/**
 * @brief Calculates the absolute value of a q31_t number. Handles problem with -2,147,483,648.
 * @param x The input number.
 * @return The absolute value of the input number.
 */
inline constexpr q31_t q31_abs(const q31_t x)
{
    if (x == Q31_MIN)
    {
        return Q31_MAX;
    }
    return x > 0 ? x : -x;
}

/**
 * @brief Adds two fixed point numbers saturating the result.
 *
 * This function adds two fixed point numbers and returns the result.
 * If the result exceeds the maximum or minimum value of the fixed point representation,
 * it is clamped to the maximum or minimum value respectively.
 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @return The sum of the two fixed point numbers.
 */
inline constexpr q31_t q31_add(const q31_t a, const q31_t b)
{
    int64_t x = (int64_t)a + (int64_t)b;
    return q31_saturate(x);
}

/**
 * @brief Adds multiple fixed point numbers saturating the result.
 *
 * This function adds multiple fixed point numbers by recursively calling the binary q31_add.
 * The result is saturated at each step to prevent overflow.
 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @param args Additional fixed point numbers to add.
 * @return The sum of all the fixed point numbers.
 */
template <typename... Args>
inline constexpr q31_t q31_add(const q31_t a, const q31_t b, const Args... args)
{
    static_assert(sizeof...(args) > 0, "q31_add variadic version requires at least 3 parameters");
    return q31_add(q31_add(a, b), args...);
}

/**
 * @brief Substract two fixed point numbers saturating the result.
 *
 * This function substract two fixed point numbers and returns the result.
 * If the result exceeds the maximum or minimum value of the fixed point representation,
 * it is clamped to the maximum or minimum value respectively.
 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @return The sum of the two fixed point numbers.
 */
inline constexpr q31_t q31_sub(const q31_t a, const q31_t b)
{
    int64_t x = (int64_t)a - (int64_t)b;
    return q31_saturate(x);
}

/**
 * @brief Multiplies two fixed point numbers saturating the result.
 *
 * This function multiplies two fixed point numbers and returns the result.
 * The result is right-shifted by 31 bits to maintain the fixed point representation.
 * If the result exceeds the maximum or minimum value of the fixed point representation,
 * it is clamped to the maximum or minimum value respectively.
 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @return The product of the two fixed point numbers.
 */
inline constexpr q31_t q31_mult(const q31_t a, const q31_t b)
{
    int64_t x = ((int64_t)a * (int64_t)b);
    x = x >> 31;
    return q31_saturate(x);
}

/**
 * @brief Multiplies multiple fixed point numbers saturating the result.
 *
 * This function multiplies multiple fixed point numbers by recursively calling the binary q31_mult.
 * The result is saturated at each step to prevent overflow.
 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @param args Additional fixed point numbers to multiply.
 * @return The product of all the fixed point numbers.
 */
template <typename... Args>
inline constexpr q31_t q31_mult(const q31_t a, const q31_t b, const Args... args)
{
    static_assert(sizeof...(args) > 0, "q31_mult variadic version requires at least 3 parameters");
    return q31_mult(q31_mult(a, b), args...);
}

/**
 * @brief Divides two fixed point numbers saturating the result.
 *
 * This function divides two fixed point numbers and returns the result.
 *
 * @param a The nominator.
 * @param b The denominator.
 * @return The result of the division.
 */
inline constexpr q31_t q31_div(const q31_t a, const q31_t b)
{
    if (b == 0)
    {
        return 0;
    }

    // Convert to int64_t to prevent overflow during the shift
    // Shift left to maintain precision after division
    int64_t a64 = a << 31;

    // Perform the division
    int64_t result = a64 / b;
    return q31_saturate(result);
}

/**
 * @brief Converts a float to a fixed point number.
 * @param a The float value to be converted.
 * @return The fixed point representation of the float value.
 */
inline constexpr q31_t float_to_q31(const float a)
{
    // Implement constrain logic inline to ensure it's always constexpr compatible
    float value = a;
    if (value > 1.0f)
    {
        value = 1.0f;
    }
    if (value < 0.0f)
    {
        value = 0.0f;
    }
    return (q31_t)(value * 2147483648.0f);
}

/**
 * @brief Converts a fixed point number to a float.
 * @param a The fixed point value to be converted.
 * @return The float representation of the fixed point value.
 */
inline constexpr float q31_to_float(const q31_t a)
{
    return (float)a / 2147483648.0f;
}

/**
 * @brief Converts a float to a fixed point number.
 * @param a The float value to be converted.
 * @return The fixed point representation of the float value.
 */
inline constexpr q15_t float_to_q15(const float a)
{
    // Implement constrain logic inline to ensure it's always constexpr compatible
    float value = a;
    if (value > 1.0f)
    {
        value = 1.0f;
    }
    if (value < 0.0f)
    {
        value = 0.0f;
    }
    return (q31_t)(value * 32767.0f);
}

/**
 * @brief Converts a fixed point number to a float.
 * @param a The fixed point value to be converted.
 * @return The float representation of the fixed point value.
 */
inline constexpr float q15_to_float(const q15_t a)
{
    return (float)a / 32767.0f;
}

/**
 * @brief Scales an array of fixed point numbers.
 *
 * This function scales an array of fixed point numbers by a given scale factor and shift value.
 * The scale factor is multiplied with each element of the array, and the result is left-shifted
 * by the specified shift value. If the result exceeds the maximum or minimum value of the fixed
 * point representation, it is clamped to the maximum or minimum value respectively.
 *
 * @param buff The array of fixed point numbers to be scaled.
 * @param scale The scale factor.
 * @param shift The shift left value.
 * @param length The length of the array.
 */
inline constexpr void q31_scale_buffer(q31_t *buffer, const q31_t scale, const int8_t shift, const uint32_t length)
{
    int32_t shift_ = 31 - shift;
    for (uint32_t i = 0; i < length; i++)
    {
        int64_t x = ((int64_t)buffer[i] * (int64_t)scale);
        x = x >> shift_;
        buffer[i] = q31_saturate(x);
    }
}
/**
 * @brief Scales an array of fixed point numbers.
 *
 * This function scales an array of fixed point numbers by a given scale factor and shift value.
 * The scale factor is multiplied with each element of the array, and the result is left-shifted
 * by the specified shift value. If the result exceeds the maximum or minimum value of the fixed
 * point representation, it is clamped to the maximum or minimum value respectively.
 *
 * @param buff The array of fixed point numbers to be scaled.
 * @param scale The scale factor.
 * @param shift The shift left value.
 * @param length The length of the array.
 */
inline constexpr void q15_scale_buffer(q15_t *buffer, const q15_t scale, const int8_t shift, const uint32_t length)
{
    int32_t shift_ = 15 - shift;
    for (uint32_t i = 0; i < length; i++)
    {
        int32_t x = (buffer[i] * scale);
        x = x >> shift_;
        buffer[i] = q15_saturate(x);
    }
}

/**
 * @brief Adds two fixed point numbers saturating the result.
 *
 * This function adds two fixed point numbers and returns the result.
 * If the result exceeds the maximum or minimum value of the fixed point representation,
 * it is clamped to the maximum or minimum value respectively.
 *
 * @note q15_t is stored as 32-bit integer so we don't need to cast it.
 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @return The sum of the two fixed point numbers.
 */
inline constexpr q15_t q15_add(const q15_t a, const q15_t b)
{
    int32_t x = a + b;
    return q15_saturate(x);
}

/**
 * @brief Adds multiple fixed point numbers saturating the result.
 *
 * This function adds multiple fixed point numbers by recursively calling the binary q15_add.
 * The result is saturated at each step to prevent overflow.
 *
 * @note q15_t is stored as 32-bit integer so we don't need to cast it.
 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @param args Additional fixed point numbers to add.
 * @return The sum of all the fixed point numbers.
 */
template <typename... Args>
inline constexpr q15_t q15_add(const q15_t a, const q15_t b, const Args... args)
{
    static_assert(sizeof...(args) > 0, "q15_add variadic version requires at least 3 parameters");
    return q15_add(q15_add(a, b), args...);
}

/**
 * @brief Substract two fixed point numbers saturating the result.
 *
 * This function substracts two fixed point numbers and returns the result.
 * If the result exceeds the maximum or minimum value of the fixed point representation,
 * it is clamped to the maximum or minimum value respectively.
 *
 * @note q15_t is stored as 32-bit integer so we don't need to cast it

 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @return The sum of the two fixed point numbers.
 */
inline constexpr q15_t q15_sub(const q15_t a, const q15_t b)
{
    int32_t x = a - b;
    return q15_saturate(x);
}

/**
 * @brief Multiplies two fixed point numbers saturating the result.
 *
 * This function multiplies two fixed point numbers and returns the result.
 * The result is right-shifted by 15 bits to maintain the fixed point representation.
 * If the result exceeds the maximum or minimum value of the fixed point representation,
 * it is clamped to the maximum or minimum value respectively.
 *
 * @note q15_t is stored as 32-bit integer so we don't need to cast it

 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @return The product of the two fixed point numbers.
 */
inline constexpr q15_t q15_mult(const q15_t a, const q15_t b)
{
    int32_t x = (a * b);
    x = x >> 15;
    return q15_saturate(x);
}

/**
 * @brief Multiplies multiple fixed point numbers saturating the result.
 *
 * This function multiplies multiple fixed point numbers by recursively calling the binary q15_mult.
 * The result is saturated at each step to prevent overflow.
 *
 * @note q15_t is stored as 32-bit integer so we don't need to cast it.
 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @param args Additional fixed point numbers to multiply.
 * @return The product of all the fixed point numbers.
 */
template <typename... Args>
inline constexpr q15_t q15_mult(const q15_t a, const q15_t b, const Args... args)
{
    static_assert(sizeof...(args) > 0, "q15_mult variadic version requires at least 3 parameters");
    return q15_mult(q15_mult(a, b), args...);
}

/**
 * @brief Multiplies two fixed point numbers WITHOUT SATURATION CHECK!
 *
 * This function multiplies two fixed point numbers and returns the result.
 * The result is right-shifted by 15 bits to maintain the fixed point representation.
 *
 * @note q15_t is stored as 32-bit integer so we don't need to cast it.
 *
 * @param a The first fixed point number.
 * @param b The second fixed point number.
 * @return The product of the two fixed point numbers.
 */
inline constexpr q15_t q15_mult_fast(const q15_t a, const q15_t b)
{
    return (a * b) >> 15;
}

/**
 * @brief Divides two fixed point numbers saturating the result.
 *
 * This function divides two fixed point numbers and returns the result.

 * @note q15_t is stored as 32-bit integer so we don't need to cast it.

 *
 * @param a The nominator.
 * @param b The denominator.
 * @return The result of the division.
 */
inline constexpr q15_t q15_div(const q15_t a, const q15_t b)
{
    // Handle division by zero
    if (b == 0)
    {
        return 0;
    }
    // Convert to int32_t to prevent overflow during the shift
    // Shift left to maintain precision after division
    int32_t a32 = a << 15;
    int32_t result = a32 / b;
    return q15_saturate(result);
}

/**
 * @brief Converts a fraction of two numbers into Q15 fixed point format.
 *
 * @param a The nominator, must be smaller than The denominator.
 * @param b The denominator.
 * @return The number between 0-1 in Q15 format.
 */
inline constexpr q15_t fraction_to_q15(const int32_t numerator, const int32_t denominator)
{
    int32_t result = (numerator * 32767 + (denominator / 2)) / denominator;
    return q15_saturate(result);
}

/**
 * @brief Fast sine implementation using a lookup table.
 * @param x The angle in radians in q31_t format.
 * @return The sine of the angle in q31_t format.
 */
inline constexpr q31_t q31_sine(const q31_t x)
{
    return qmath_sine_table[q31_abs(x) >> QMATH_SINE_TABLE_SHIFT_Q31];
}

/**
 * @brief Fast sine implementation using a lookup table.
 * @param x The angle in radians in q15_t format.
 * @return The sine of the angle in q15_t format.
 */
inline constexpr q15_t q15_sine(const q15_t x)
{
    return q31_to_q15(qmath_sine_table[q15_abs(x) >> QMATH_SINE_TABLE_SHIFT_Q15]);
}

/**
 * @brief Converts a frequency in Hz to a relative frequency to sample_rate.
 * @param frequency The frequency in Hz.
 * @param sample_rate The sample rate in Hz.
 * @return The relative frequency in q31_t format.
 */
inline constexpr q31_t freq_to_q31(const float frequency, const float sample_rate)
{
    return (frequency * (Q31_MAX / sample_rate));
}

/**
 * @brief Converts a frequency in Hz to a relative frequency to sample_rate.
 * @param frequency The frequency in Hz.
 * @param sample_rate The sample rate in Hz.
 * @return The relative frequency in q15_t format.
 */
inline constexpr q15_t freq_to_q15(const float frequency, const float sample_rate)
{
    return (frequency * (Q15_MAX / sample_rate));
}

}
