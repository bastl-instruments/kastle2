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

#include <cstddef>
#include <cstdint>
#include <span>
#include "common/EnumTools.hpp"

namespace kastle2
{

/**
 * @class Quantizer
 * @ingroup dsp_utility
 * @brief Basic quantizer class, which quantizes frequencies to specific scales using Process() function.
 * @author Vaclav Mach (Bastl Instruments), Marek Mach (Bastl Instruments)
 * @date 2024-05-23
 *
 * The Quantizer class provides functionality for quantizing frequencies to specific scales.
 * It supports different quantization types such as chromatic, major, major pentatonic, major chord, blues, minor, minor pentatonic, and minor chord.
 * The class also allows enabling or disabling the quantization.
 *
 */
class Quantizer
{

public:
    /**
     * @brief Scale type - LSB First uint32_t bitmask, length 12
     * @note The 32-bit value is used because of the ARM memory alignment.
     */
    typedef uint32_t Scale;

    /**
     * @brief Quantization types.
     * @note When updating this enum, make sure to update default_scale_table_ in the Init() function.
     */
    enum class DefaultScale
    {
        MINOR_CHORD,      ///< Minor chord scale
        MINOR_PENTATONIC, ///< Minor pentatonic scale
        MINOR_DIATONIC,   ///< Minor scale
        CHROMATIC,        ///< Chromatic scale
        MAJOR_DIATONIC,   ///< Major scale
        MAJOR_PENTATONIC, ///< Major pentatonic scale
        MAJOR_CHORD,      ///< Major chord scale
        COUNT             ///< Number of quantization types
    };

    /**
     * @brief Root semitones of the quantization scale.
     *
     * The Root enum represents the root semitones of the quantization scale.
     * This note is used as the starting point for the quantization scale.
     */
    enum class ScaleRoot
    {
        C,
        C_SHARP,
        D,
        D_SHARP,
        E,
        F,
        F_SHARP,
        G,
        G_SHARP,
        A,
        A_SHARP,
        B,
        COUNT
    };

    /**
     * @brief Initializes the Quantizer.
     * @param semitone_threshold The minimal threshold between two quantizations in semitones (to prevent jumping).
     *
     * This function initializes the Quantizer with default values.
     */
    void Init(const float semitone_threshold = 0.0f);

    /**
     * @brief Processes the input frequency and returns the quantized frequency.
     *
     * If the quantizer is disabled, the input frequency is returned unchanged.
     *
     * @param frequency The input frequency to be quantized.
     * @return The quantized frequency.
     */
    float Process(const float frequency);

    /**
     * @brief Processes the input pitch multiplier (=relative frequency) and returns the quantized pitch multiplier.
     *
     * If the quantizer is disabled, the input multiplier is returned unchanged.
     *
     * @param multiplier The input multiplier to be quantized.
     * @return The quantized multiplier.
     */
    float ProcessMultiplier(float multiplier);

    /**
     * @brief Sets the quantization scale.
     *
     * @param quantization The quantization scale to be set.
     */
    void SetScale(const size_t quantization);

    /**
     * @brief Sets the quantization scale.
     *
     * @param quantization The quantization scale to be set.
     */
    void SetScale(const DefaultScale quantization);

    /**
     * @brief Gets the current quantization scale.
     *
     * @return The current quantization scale.
     */
    size_t GetScale() const;

    /**
     * @brief THIS DOESN'T TRANSPOSE THE OUTPUT FREQUENCY, but rather shifts the quantization scale.
     * @param root The root semitone of the scale.
     * @note Necessarily you don't want to use this, but apply the shift manually after the quantization,
     *       so the root setting actually transposes the final pitch.
     */
    void SetRoot(const ScaleRoot root);

    /**
     * @brief Gets the root semitone - how much is the quantization scale shifted.
     *
     * @return The root semitone of the scale.
     */
    ScaleRoot GetRoot() const;

    /**
     * @brief Sets whether the quantizer is enabled or disabled.
     *
     * @param enabled True to enable the quantizer, false to disable it.
     */
    void SetEnabled(const bool enabled);

    /**
     * @brief Checks if the quantizer is enabled.
     *
     * @return True if the quantizer is enabled, false otherwise.
     */
    bool IsEnabled() const;

    /**
     * @brief Sets the custom scale table, resetting the selected scale to 0.
     * @param scale_table The custom scale table.
     */
    void SetScaleTable(std::span<const Scale> scale_table);

    /**
     * @brief Gets the size of the current scale table.
     * @return The size of the current scale table.
     */
    size_t GetScaleTableSize() const
    {
        return scale_table_.size();
    }

    /**
     * @brief MIDI note frequencies.
     */
    static constexpr std::array<float, 128> kFrequencyTable = {
        8.18f, 8.66f, 9.18f, 9.72f, 10.30f, 10.91f, 11.56f, 12.25f, 12.98f, 13.75f, 14.57f, 15.43f,
        16.35f, 17.32f, 18.35f, 19.45f, 20.60f, 21.83f, 23.12f, 24.50f, 25.96f, 27.50f, 29.14f, 30.87f,
        32.70f, 34.65f, 36.71f, 38.89f, 41.20f, 43.65f, 46.25f, 49.00f, 51.91f, 55.00f, 58.27f, 61.74f,
        65.41f, 69.30f, 73.42f, 77.78f, 82.41f, 87.31f, 92.50f, 98.00f, 103.83f, 110.00f, 116.54f, 123.47f,
        130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 196.00f, 207.65f, 220.00f, 233.08f, 246.94f,
        261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f, 440.00f, 466.16f, 493.88f,
        523.25f, 554.37f, 587.33f, 622.25f, 659.26f, 698.46f, 739.99f, 783.99f, 830.61f, 880.00f, 932.33f, 987.77f,
        1046.50f, 1108.73f, 1174.66f, 1244.51f, 1318.51f, 1396.91f, 1479.98f, 1567.98f, 1661.22f, 1760.00f, 1864.66f, 1975.53f,
        2093.00f, 2217.46f, 2349.32f, 2489.02f, 2637.02f, 2793.83f, 2959.96f, 3135.96f, 3322.44f, 3520.00f, 3729.31f, 3951.07f,
        4186.01f, 4434.92f, 4698.64f, 4978.03f, 5274.04f, 5587.65f, 5919.91f, 6271.93f, 6644.88f, 7040.00f, 7458.62f, 7902.13f,
        8372.02f, 8869.84f, 9397.27f, 9956.06f, 10548.08f, 11175.30f, 11839.82f, 12543.85f};

    /**
     * @brief Frequency multipliers in each octave (C=1.0, C#=1.05946, D=1.12246, etc.).
     */
    static constexpr std::array<float, 12> kMultiplierTable = {
        1.0f, 1.05946f, 1.12246f, 1.18920f, 1.2599f, 1.33483f, 1.41421f, 1.49830f, 1.58740f, 1.68179f, 1.78179f, 1.88774f};

private:
    size_t scale_index_ = 0;    ///< The current quantization scale.
    uint32_t scale_root_offset_ = 0; ///< The root semitone of the scale.
    bool enabled_ = false;           ///< Flag indicating whether the quantizer is enabled or disabled.

    bool threshold_enabled_ = false; ///< Flag indicating whether the threshold is enabled or disabled.
    float threshold_ratio_ = 0.0f;   ///< The minimal threshold between two quantization levels.
    float prev_frequency_ = 0.0f;    ///< The previous frequency.
    float prev_multiplier_ = 0.0f;   ///< The previous multiplier.

    std::span<const Scale> scale_table_;                               ///< The current scale table.
    static inline EnumArray<DefaultScale, Scale> default_scale_table_; ///< Filled in Init()

    static constexpr float kMultiplierMultiplyThreshold = 0.97193f;
    static constexpr float kMultiplierDivideThreshold = 1.94387f;
};
}
