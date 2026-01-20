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

#include <array>
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace kastle2
{

/**
 * @file math_utils.h
 * @ingroup
 * @brief Custom math functions and utilities.
 * @author Vaclav Mach (Bastl Instruments), Marek Mach (Bastl Instruments)
 * @date 2024-05-30
 */

/**
 * @brief Helper struct to determine the result type for mathematical operations
 * @tparam Ts Types to be considered for the result type.
 * @note The priority is: float > signed integer > unsigned integer.
 *       If any type is floating-point, the result will be float and so on.
 */
template <typename... Ts>
struct MathResultHelper
{
private:
    static constexpr bool any_float = (std::is_floating_point_v<Ts> || ...);
    static constexpr bool any_signed = ((std::is_integral_v<Ts> && std::is_signed_v<Ts>) || ...);

public:
    using type = std::conditional_t<
        any_float, float,
        std::conditional_t<any_signed, int, unsigned int>>;
};

/**
 * @brief Resolves to the appropriate type based on the rules defined in MathResultHelper.
 * @tparam Ts Types to be considered for the result type.
 *  */
template <typename... Ts>
using math_result_t = typename MathResultHelper<Ts...>::type;

/**
 * @brief Constrain a value to a range
 * @param x The value to constrain
 * @param in_min The minimum value of the range
 * @param in_max The maximum value of the range
 * @return The constrained value
 *
 * Uses the math_result_t helper to determine the appropriate type:
 * - If any input is floating-point, the result is floating-point
 * - If any input is signed (and none are floating-point), the result is signed
 * - Otherwise, use the common type (likely unsigned)
 */
template <typename T, typename U, typename V>
inline constexpr auto constrain(T x, U in_min, V in_max)
{
    // Use the math_result_t helper to determine the appropriate type
    using ResultType = math_result_t<T, U, V>;

    // Convert all values to the common type
    ResultType cx = static_cast<ResultType>(x);
    ResultType cmin = static_cast<ResultType>(in_min);
    ResultType cmax = static_cast<ResultType>(in_max);

    // Apply constraints
    if (cx > cmax)
    {
        return cmax;
    }
    if (cx < cmin)
    {
        return cmin;
    }
    return cx;
}

/**
 * @brief Check if a value is between two bounds (inclusive)
 * @param x The value to check
 * @param in_min The minimum bound
 * @param in_max The maximum bound
 * @return true if x is between in_min and in_max (inclusive), false otherwise
 */
template <typename T, typename U, typename V>
inline constexpr bool between(T x, U in_min, V in_max)
{
    // Use the math_result_t helper to determine the appropriate type
    using ResultType = math_result_t<T, U, V>;

    // Convert all values to the common type
    ResultType cx = static_cast<ResultType>(x);
    ResultType cmin = static_cast<ResultType>(in_min);
    ResultType cmax = static_cast<ResultType>(in_max);

    return (cx >= cmin) && (cx <= cmax);
}

/**
 * @brief Returns the absolute difference between two values.
 * @tparam A Type of the first value.
 * @tparam B Type of the second value.
 * @param a The first value.
 * @param b The second value.
 * @return The absolute difference between `a` and `b`.
 * @note This function works with both integral and floating-point types.
 */
template <typename A, typename B>
inline constexpr auto diff(A a, B b)
{
    using Common = std::common_type_t<A, B>;
    if constexpr (std::is_floating_point_v<Common>)
    {
        return a > b ? a - b : b - a;
    }
    else
    {
        using Signed = std::make_signed_t<Common>;
        return static_cast<std::make_unsigned_t<Signed>>(
            static_cast<Signed>(a) > static_cast<Signed>(b)
                ? static_cast<Signed>(a - b)
                : static_cast<Signed>(b - a));
    }
}

/**
 * @brief Ideal for mapping large values (eg Pot) to small values (eg MIDI CC).
 *        Maps an input value to an output value with hysteresis,
 *        keeping the last output stable unless the input changes significantly.
 * @param input The current input value to map.
 * @param in_min The minimum value of the input range.
 * @param in_max The maximum value of the input range.
 * @param out_min The minimum value of the output range.
 * @param out_max The maximum value of the output range.
 * @param last_output Reference to the last output value, which will be updated if the input changes significantly. Should be inited as INT32_MAX.
 * @param input_hysteresis The hysteresis threshold for input changes, default is 24.
 * @note Don't forget to initialize `last_output` to `INT32_MAX` before the first call!
 * @return The mapped output value, which may be the same as `last_output` if the input hasn't changed significantly.
 */
int32_t constexpr inline sticky_map(const int32_t input, const int32_t in_min, const int32_t in_max, const int32_t out_min, const int32_t out_max, int32_t &last_output, const int32_t input_hysteresis = 20)
{
    // Handle reversed input and output ranges
    bool input_reversed = in_min > in_max;
    bool output_reversed = out_min > out_max;

    // Use normalized ranges for internal calculations
    int32_t actual_in_min = input_reversed ? in_max : in_min;
    int32_t actual_in_max = input_reversed ? in_min : in_max;
    int32_t actual_out_min = output_reversed ? out_max : out_min;
    int32_t actual_out_max = output_reversed ? out_min : out_max;

    // Total number of output steps
    int32_t steps = actual_out_max - actual_out_min + 1;
    if (steps <= 1)
    {
        return out_min;
    }

    // Step size in input units
    int32_t step_size = (actual_in_max - actual_in_min + 1) / steps;

    // Prevent divide-by-zero or negative range
    if (step_size <= 0)
    {
        return out_min;
    }

    // Determine which range the input would fall into
    int32_t mapped_step = (input - actual_in_min) / step_size;

    // Clamp to valid output range
    mapped_step = constrain(mapped_step, 0, steps - 1);

    // Calculate candidate output in normalized range
    int32_t normalized_output = actual_out_min + mapped_step;

    // Convert to original range direction if needed
    int32_t candidate_output = output_reversed ? (actual_out_min + actual_out_max - normalized_output) : normalized_output;

    // If this is the first run, just set and return
    if (last_output == INT32_MAX)
    {
        last_output = candidate_output;
        return candidate_output;
    }

    // For hysteresis calculation, we need to map the last_output back to input space
    // First convert last_output to normalized range if output was reversed
    int32_t normalized_last_output = output_reversed ? (actual_out_min + actual_out_max - last_output) : last_output;

    // Hysteresis check: prevent switching unless outside of sticky zone
    int32_t last_input_center = actual_in_min + (normalized_last_output - actual_out_min) * step_size + step_size / 2;
    if (abs(input - last_input_center) < input_hysteresis)
    {
        return last_output;
    }

    // Otherwise accept new value
    last_output = candidate_output;
    return candidate_output;
}

/**
 * @brief Constructs a 12-bit potentiometer-style value (0–4095) from a floating-point literal.
 *
 * @tparam T Must be `float` or `double`.
 * @param x A floating-point value in the range [0.0, 1.0].
 * @return int32_t An integer in the range [0, 4095], rounded to the nearest.
 *
 * @note
 * - Can only be used at compile time.
 * - The input must be a `float` or `double` literal (e.g., `pot(1.0f)` or `pot(0.0)`).
 * - `1.0` is mapped to 4095.
 * - Values outside [0.0, 1.0] will be constrained to this range.
 */
template <typename T>
consteval int32_t pot(const T x)
{
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                  "pot() requires float or double literal (e.g., pot(0.f) or pot(0.0))");
    return static_cast<int32_t>(x * 4095.0 + 0.5);
}

/**
 * @brief Defines a mapping table for input and output values using `curve_map` or `step_map`.
 * @tparam T The type of the input and output values (must be arithmetic).
 * @tparam N The size of the mapping table.
 */
template <typename T, size_t N>
    requires std::is_arithmetic_v<T>
struct MapDef
{
private:
    static constexpr size_t size_ = N;

public:
    std::array<T, N> input{};
    std::array<T, N> output{};

    /**
     * @brief Default constructor.
     */
    constexpr MapDef() = default;

    /**
     * @brief Creates a mapping definition with the given input and output arrays.
     * @param in The input values.
     * @param out The output values.
     * @note The input and output arrays must be of the same size.
     */
    constexpr MapDef(const std::array<T, N> &in,
                     const std::array<T, N> &out)
        : input(in), output(out) {}

    /**
     * @brief Returns the size of the mapping table in C++ container-style semantics.
     */
    constexpr size_t size() const
    {
        return size_;
    }
};

/**
 * @brief Defines a mapping table for input and output values using `enum_map`.
 * @tparam Ti The type of the input values (must be arithmetic).
 * @tparam To The type of the output values.
 * @tparam N The size of the mapping table.
 */
template <typename Ti, typename To, size_t N>
    requires std::is_arithmetic_v<Ti>
struct EnumMapDef
{
private:
    static constexpr size_t size_ = N;

public:
    std::array<Ti, N> input{};
    std::array<To, N> output{};

    /**
     * @brief Default constructor.
     */
    constexpr EnumMapDef() = default;

    /**
     * @brief Creates a mapping definition with the given input and output arrays.
     * @param in The input values.
     * @param out The output values.
     * @note The input and output arrays must be of the same size.
     */
    constexpr EnumMapDef(const std::array<Ti, N> &in,
                         const std::array<To, N> &out)
        : input(in), output(out) {}

    /**
     * @brief Returns the size of the mapping table in C++ container-style semantics.
     */
    constexpr size_t size() const
    {
        return size_;
    }
};

/**
 * @brief Outside of range behavior for mapping functions.
 * @note We use enum class to ensure maximum safety.
 */
enum class MapClamp
{
    FALSE, ///< Allow extrapolation.
    TRUE   ///< Clamp the output to the output range.
};

/**
 * @brief Whether to use 64-bit math to avoid overflows for big numbers.
 *        This is only relevant for integer types, as floating-point types are always "safe".
 * @note We use enum class to ensure maximum safety.
 */
enum class MapSafe
{
    FALSE, ///< Use 32 bit math to increase speed at risk of overflows for big numbers
    TRUE   ///< Use 64 bit math to avoid overflows for big numbers, but slower.
};

/**
 * @brief Maps a value from one range to another with optional safety checks.
 * @tparam T Type of the input value
 * @tparam U Type of the input range minimum
 * @tparam V Type of the input range maximum
 * @tparam W Type of the output range minimum
 * @tparam X Type of the output range maximum
 * @param value The value to map.
 * @param inMin The minimum value of the input range.
 * @param inMax The maximum value of the input range.
 * @param outMin The minimum value of the output range.
 * @param outMax The maximum value of the output range.
 * @param clamp If true, clamps the output to the output range.
 * @param safe If true, uses a safe mapping that avoids overflow for integer types by promoting to int64_t.
 *             Ignored for floating-point types, as they are always safe.
 *
 * @return The mapped value.
 */
template <typename T, typename U, typename V, typename W, typename X>
constexpr auto map(T value, U inMin, V inMax, W outMin, X outMax, MapClamp clamp = MapClamp::FALSE, MapSafe safe = MapSafe::FALSE)
{
    // Use the math_result_t helper to determine the appropriate type
    using ResultType = math_result_t<T, U, V, W, X>;

    ResultType vValue = static_cast<ResultType>(value);
    ResultType vInMin = static_cast<ResultType>(inMin);
    ResultType vInMax = static_cast<ResultType>(inMax);
    ResultType vOutMin = static_cast<ResultType>(outMin);
    ResultType vOutMax = static_cast<ResultType>(outMax);

    if (vInMax == vInMin)
    {
        return vOutMin; // avoid divide-by-zero
    }

    ResultType result;

    // Check if we need safe integer calculation
    if constexpr (!std::is_floating_point_v<ResultType>)
    {
        if (safe == MapSafe::TRUE)
        {
            // For integer types, promote to a larger type to avoid overflow
            using SafeType = std::conditional_t<sizeof(ResultType) >= 8,
                                                ResultType,
                                                std::conditional_t<std::is_signed_v<ResultType>,
                                                                   int64_t,
                                                                   uint64_t>>;

            result = static_cast<ResultType>(
                (static_cast<SafeType>(vValue - vInMin) * static_cast<SafeType>(vOutMax - vOutMin)) /
                    static_cast<SafeType>(vInMax - vInMin) +
                static_cast<SafeType>(vOutMin));
        }
        else
        {
            // Standard integer calculation
            result = (vValue - vInMin) * (vOutMax - vOutMin) / (vInMax - vInMin) + vOutMin;
        }
    }
    else
    {
        // For floating point, just do the standard calculation
        result = (vValue - vInMin) * (vOutMax - vOutMin) / (vInMax - vInMin) + vOutMin;
    }

    // Apply clamping if requested
    if (clamp == MapClamp::TRUE)
    {
        ResultType minOut = vOutMin < vOutMax ? vOutMin : vOutMax;
        ResultType maxOut = vOutMin < vOutMax ? vOutMax : vOutMin;
        if (result < minOut)
        {
            result = minOut;
        }
        else if (result > maxOut)
        {
            result = maxOut;
        }
    }

    return result;
}

/**
 * @brief Maps a value using a predefined mapping table with linear interpolation.
 * @tparam T The type of the value (must be arithmetic).
 * @tparam U The type of the input and output values in the mapping table (must be arithmetic).
 * @tparam N The size of the mapping table.
 * @param value The value to map.
 * @param mapDef The mapping definition containing input and output arrays.
 * @param clamp If true, clamps the output to the output range.
 *              If false, extrapolates the output value based on the nearest input range.
 * @param safe If true, uses a safe mapping that avoids overflow by promoting to int64_t. It's ignored for floating-point types.
 * @return The mapped value, interpolated or extrapolated as needed.
 */
template <typename T, typename U, size_t N>
    requires(N >= 2)
constexpr auto curve_map(T value, const MapDef<U, N> &mapDef, MapClamp clamp = MapClamp::FALSE, MapSafe safe = MapSafe::FALSE)
{
    // Use the math_result_t helper to determine the appropriate type
    using ResultType = math_result_t<T, U>;

    // Convert input value to the compute type
    ResultType typed_value = static_cast<ResultType>(value);

    // Detect if the input array is ascending or descending
    bool is_ascending = static_cast<ResultType>(mapDef.input[0]) <= static_cast<ResultType>(mapDef.input[N - 1]);

    // Helper lambda to compare values based on array direction
    auto in_range = [is_ascending](ResultType val, ResultType low, ResultType high) -> bool
    {
        if (is_ascending)
        {
            return val >= low && val <= high;
        }
        else
        {
            return val <= low && val >= high;
        }
    };

    // Search for the range containing the value
    for (size_t i = 0; i < N - 1; ++i)
    {
        ResultType inLow = static_cast<ResultType>(mapDef.input[i]);
        ResultType inHigh = static_cast<ResultType>(mapDef.input[i + 1]);

        if (in_range(typed_value, inLow, inHigh))
        {
            return map(typed_value,
                       inLow, inHigh,
                       static_cast<ResultType>(mapDef.output[i]),
                       static_cast<ResultType>(mapDef.output[i + 1]),
                       MapClamp::FALSE,
                       safe);
        }
    }

    // Determine effective first and last values based on array direction
    ResultType first_input = static_cast<ResultType>(mapDef.input[0]);
    ResultType last_input = static_cast<ResultType>(mapDef.input[N - 1]);
    ResultType first_output = static_cast<ResultType>(mapDef.output[0]);
    ResultType last_output = static_cast<ResultType>(mapDef.output[N - 1]);

    // Handle out-of-range values, accounting for array direction
    if ((is_ascending && typed_value < first_input) ||
        (!is_ascending && typed_value > first_input))
    {
        if (clamp == MapClamp::TRUE)
        {
            return first_output;
        }
        else
        {
            // Extrapolate using first two points
            return map(typed_value,
                       first_input,
                       static_cast<ResultType>(mapDef.input[1]),
                       first_output,
                       static_cast<ResultType>(mapDef.output[1]),
                       MapClamp::FALSE,
                       safe);
        }
    }
    else if ((is_ascending && typed_value > last_input) ||
             (!is_ascending && typed_value < last_input))
    {
        if (clamp == MapClamp::TRUE)
        {
            return last_output;
        }
        else
        {
            // Extrapolate using last two points
            return map(typed_value,
                       static_cast<ResultType>(mapDef.input[N - 2]),
                       last_input,
                       static_cast<ResultType>(mapDef.output[N - 2]),
                       last_output,
                       MapClamp::FALSE,
                       safe);
        }
    }
    return typed_value;
}

/**
 * @brief Maps a value using a predefined mapping table with no interpolation (like a detent switch).
 * @tparam T The type of the value (must be arithmetic).
 * @tparam U The type of the input and output values in the mapping table (must be arithmetic).
 * @tparam N The size of the mapping table.
 * @param value The value to map.
 * @param mapDef The mapping definition containing input and output arrays with their types.
 * @return The mapped value based on which input value's range the value falls into.
 *
 * @details
 * This function maps a value to an output without interpolation, similar to a detent switch.
 * It determines which discrete "zone" the input value falls into by using thresholds
 * at the midpoints between input values, then returns the output value for that zone.
 * - If value is lower than the midpoint between input[0] and input[1], output[0] is returned
 * - If value is between the midpoints of input[i-1]/input[i] and input[i]/input[i+1], output[i] is returned
 * - If value is higher than the midpoint between input[N-2] and input[N-1], output[N-1] is returned
 */
template <typename T, typename U, size_t N>
    requires(N >= 2)
constexpr auto step_map(T value, const MapDef<U, N> &mapDef)
{
    // Use the math_result_t helper to determine the appropriate type
    using ResultType = math_result_t<T, U>;

    // Convert input value to the compute type
    ResultType typed_value = static_cast<ResultType>(value);

    // Calculate threshold between first and second values
    ResultType upper_threshold = (static_cast<ResultType>(mapDef.input[0]) +
                                  static_cast<ResultType>(mapDef.input[1])) /
                                 2;

    // If below first threshold, return first output
    if (typed_value <= upper_threshold)
        return static_cast<ResultType>(mapDef.output[0]);

    // Check each middle range
    for (size_t i = 1; i < N - 1; ++i)
    {
        // Thresholds are midpoints between the current and adjacent values
        ResultType lower_threshold = (static_cast<ResultType>(mapDef.input[i]) +
                                      static_cast<ResultType>(mapDef.input[i - 1])) /
                                     2;
        upper_threshold = (static_cast<ResultType>(mapDef.input[i]) +
                           static_cast<ResultType>(mapDef.input[i + 1])) /
                          2;

        if (typed_value >= lower_threshold && typed_value <= upper_threshold)
            return static_cast<ResultType>(mapDef.output[i]);
    }

    // If we get here, return last output value
    return static_cast<ResultType>(mapDef.output[N - 1]);
}

/**
 * @brief Maps an enum using a predefined mapping table with no interpolation (like a detent switch).
 * @tparam T The type of the value (must be arithmetic).
 * @tparam Tm The type of the input values in the mapping table (must be arithmetic).
 * @tparam U The type of the output values in the mapping table.
 * @tparam N The size of the mapping table.
 * @param value The value to map.
 * @param mapDef The mapping definition containing input and output arrays.
 * @return The mapped value based on which input value's range the value falls into.
 *
 * @details
 * This function maps a value to an output without interpolation, similar to a detent switch.
 * It determines which discrete "zone" the input value falls into by using thresholds
 * at the midpoints between input values, then returns the output value for that zone.
 * - If value is lower than the midpoint between input[0] and input[1], output[0] is returned
 * - If value is between the midpoints of input[i-1]/input[i] and input[i]/input[i+1], output[i] is returned
 * - If value is higher than the midpoint between input[N-2] and input[N-1], output[N-1] is returned
 */
template <typename T, typename Tm, typename U, size_t N>
    requires(N >= 2)
constexpr auto enum_map(T value, const EnumMapDef<Tm, U, N> &mapDef)
{
    // Convert input value to the compute type
    Tm typed_value = static_cast<Tm>(value);

    // Calculate threshold between first and second values
    Tm upper_threshold = (static_cast<Tm>(mapDef.input[0]) +
                          static_cast<Tm>(mapDef.input[1])) /
                         2;

    // If below first threshold, return first output
    if (typed_value <= upper_threshold)
    {
        return static_cast<U>(mapDef.output[0]);
    }

    // Check each middle range
    for (size_t i = 1; i < N - 1; ++i)
    {
        // Thresholds are midpoints between the current and adjacent values
        Tm lower_threshold = (static_cast<Tm>(mapDef.input[i]) +
                              static_cast<Tm>(mapDef.input[i - 1])) /
                             2;
        upper_threshold = (static_cast<Tm>(mapDef.input[i]) +
                           static_cast<Tm>(mapDef.input[i + 1])) /
                          2;

        if (typed_value >= lower_threshold && typed_value <= upper_threshold)
        {
            return static_cast<U>(mapDef.output[i]);
        }
    }

    // If we get here, return last output value
    return static_cast<U>(mapDef.output[N - 1]);
}

/**
 * @brief Saturates an integer value to a uint8_t range
 * @param x The value to saturate
 * @return The saturated value
 */
inline constexpr uint8_t u8_saturate(const int32_t x)
{
    return x > 255 ? 255 : (x < 0 ? 0 : x);
}

/**
 * @brief Generates a semitone offset from a semitone number
 * @param semitone The semitone number
 * @return The generated offset
 */
constexpr float semitone_mod(int32_t semitones) noexcept
{
    return std::pow(2.0f, static_cast<float>(semitones) / 12.0f);
}

} // namespace kastle2
