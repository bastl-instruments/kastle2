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

#include <map>
#include <string_view>
#include "common/dsp/sampling/SamplePlayer.hpp"
#include "common/testmode/version_samples.hpp"

namespace kastle2
{

/**
 * @class VersionChainGenerator
 * @ingroup testmode
 * @brief Generates a version chain of audio samples at compile time.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-06-23
 * @note Translates "1.2" into a chain of samples like "1", "point", "2".
 */
class VersionChainGenerator
{
private:
    /**
     * @brief Returns a SamplePlayer16bit::Sample based on the string input.
     * @param str The string to match against known sample names.
     * @return The corresponding SamplePlayer16bit::Sample if found, otherwise an empty sample
     * @note This function is evaluated at compile time.
     */
    static consteval const SamplePlayer16bit::Sample GetSample(std::string_view str)
    {
        // Return pointers to the global samples
        if (str == "rc")
            return version::rc;
        if (str == "release_candidate")
            return version::release_candidate;
        if (str == "beta_version")
            return version::beta_version;
        if (str == "test_mode")
            return version::test_mode;
        if (str == "test_success")
            return version::test_success;
        if (str == "citadel")
            return version::citadel;
        if (str == "kastle2")
            return version::kastle2;
        if (str == "version")
            return version::version;
        if (str == "0")
            return version::zero;
        if (str == "1")
            return version::one;
        if (str == "2")
            return version::two;
        if (str == "3")
            return version::three;
        if (str == "4")
            return version::four;
        if (str == "5")
            return version::five;
        if (str == "6")
            return version::six;
        if (str == "7")
            return version::seven;
        if (str == "8")
            return version::eight;
        if (str == "9")
            return version::nine;
        if (str == "10")
            return version::ten;
        if (str == ".")
            return version::point;

        return version::empty;
    }

    /**
     * @brief Finds the longest matching sample in the string starting from a given position.
     * @param str The string to search in.
     * @param start_position The position in the string to start searching from.
     * @return A pair containing the matching SamplePlayer16bit::Sample and the word length.
     * @note This function is evaluated at compile time.
     */
    static consteval std::pair<const SamplePlayer16bit::Sample, size_t> GetLongestMatchingSample(std::string_view str, size_t start_position)
    {
        for (size_t word_len = str.length() - start_position; word_len > 0; --word_len)
        {
            std::string_view substr = str.substr(start_position, word_len);
            const SamplePlayer16bit::Sample sample = GetSample(substr);
            if (sample.length > 0)
            {
                return {sample, word_len};
            }
        }
        return {version::empty, 0};
    }

public:
    /**
     * @brief Generates the version chain at compile time using template metaprogramming
     * @param app_name_sample The sample representing the application name (e.g., "WaveBard").
     * @param version_string The version string to be converted into a chain of samples, eg. "1.5"
     * @return A std::array of SamplePlayer16bit::Sample containing the version chain
     * @details N+1 because we need to subtract 1 for the null terminator in the string, add 1 for the app name sample, and 1 for the "version" word sample.
     * @note This function is evaluated at compile time.
     * @warning Can produce zero samples at the end of chain when there are samples spanning multiplie characters. Are filtered out in the TestMode class.
     */
    template <size_t N>
    static consteval std::array<SamplePlayer16bit::Sample, N + 1> Generate(const SamplePlayer16bit::Sample app_name_sample,
                                                                           const char (&version_string)[N])
    {
        std::array<SamplePlayer16bit::Sample, N + 1> result = {};

        // Start with app name
        result[0] = app_name_sample;

        // Add "version" word
        result[1] = version::version;

        size_t result_idx = 2;
        size_t i = 0;

        // Convert char array to string_view for easier substring operations
        std::string_view version_string_view(version_string, N - 1); // N-1 to skip null terminator

        while (i < N - 1)
        {
            auto [sample, word_len] = GetLongestMatchingSample(version_string_view, i);
            if (sample.length > 0)
            {
                result[result_idx++] = sample;
                i += word_len;
            }
            else
            {
                // Skip unrecognized character
                ++i;
            }
        }

        return result;
    }
};

}
