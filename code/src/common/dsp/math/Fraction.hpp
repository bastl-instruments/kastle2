/*
MIT License

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

#include <cmath>
#include <cstdint>
#include <numeric>

namespace kastle2
{

/**
 * @class Fraction
 * @brief Fraction arithmetic class.
 * @ingroup dsp_math
 * @author Marek Mach (Bastl Instruments), Vaclav Mach (Bastl Instruments)
 * @note Functions and macros for addition, multiplication, and scaling of fractions will be added in the future.
 * @date 2024-06-17
 */
class Fraction
{
public:
    /**
     * @brief The top part of a fraction (numerator)
     */
    int32_t n;

    /**
     * @brief The bottom part of a fraction (denominator)
     */
    int32_t d;

    /**
     * @brief Default constructor
     */
    constexpr Fraction() : n(0), d(1) {}

    /**
     * @brief Constructor with numerator and denominator
     * @param numerator The top part of the fraction (n)
     * @param denominator The bottom part of the fraction (d)
     */
    constexpr Fraction(int32_t numerator, int32_t denominator) : n(numerator), d(denominator) {}
    
    /**
     * @brief Copy constructor
     * @param other The fraction to copy from
     */
    constexpr Fraction(const Fraction& other) : n(other.n), d(other.d) {}

    /**
     * @brief Equality operator to compare two fractions
     * @param other The other fraction to compare with
     * @return true if both fractions have identical numerator and denominator
     */
    constexpr bool operator==(const Fraction &other) const
    {
        return n == other.n && d == other.d;
    }

    /**
     * @brief Assignment operator to copy one fraction to another
     * @param other The fraction to copy from
     * @return Reference to this fraction after assignment
     */
    constexpr Fraction &operator=(const Fraction &other)
    {
        n = other.n;
        d = other.d;
        return *this;
    }

    /**
     * @brief Compare two fractions by their mathematical value (1/2 == 2/4)
     * @param other The other fraction to compare with
     * @return True if both fractions represent the same mathematical value
     */
    constexpr bool Equal(const Fraction &other) const
    {
        // Simple fast check for direct equality
        if (n == other.n && d == other.d)
        {
            return true;
        }
        // Cross multiplication to avoid division: a/b = c/d if and only if a*d = b*c
        return (n * other.d) == (d * other.n);
    }

    /**
     * @brief Reduce a fraction to its lowest terms (smallest equivalent fraction)
     * @return A new Fraction with the same value but in its simplest form
     */
    constexpr Fraction Simplify() const
    {
        // Handle special cases
        if (n == 0)
        {
            return Fraction(0, 1);
        }
        if (d == 0)
        {
            return Fraction(n > 0 ? 1 : -1, 0); // Preserve sign for infinity
        }
        // Find greatest common divisor using std::gcd
        int32_t divisor = std::gcd(std::abs(n), std::abs(d));
        // Handle negative denominator by moving the sign to numerator
        if (d < 0)
        {
            return Fraction(-n / divisor, -d / divisor);
        }
        else
        {
            return Fraction(n / divisor, d / divisor);
        }
    }
};

}
