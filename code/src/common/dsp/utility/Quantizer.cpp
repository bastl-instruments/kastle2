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

#include "Quantizer.hpp"
#include <cmath>
#include "common/dsp/math/math_utils.hpp"

using namespace kastle2;

void Quantizer::Init(const float threshold_semitones)
{
    enabled_ = false;
    prev_frequency_ = 1.0f;
    threshold_enabled_ = false;

    if (threshold_semitones > 0.0f)
    {
        threshold_enabled_ = true;
        // precalculate the frequency ratio here to avoid doing log2 each Process()
        threshold_ratio_ = std::pow(2.0, threshold_semitones / 12.0);
    }

    // Set up the default scale table
    default_scale_table_[DefaultScale::CHROMATIC] = 0b111111111111;
    default_scale_table_[DefaultScale::MAJOR_DIATONIC] = 0b101010110101;
    default_scale_table_[DefaultScale::MAJOR_PENTATONIC] = 0b001010010101;
    default_scale_table_[DefaultScale::MAJOR_CHORD] = 0b000010010001;
    default_scale_table_[DefaultScale::MINOR_DIATONIC] = 0b010110101101;
    default_scale_table_[DefaultScale::MINOR_PENTATONIC] = 0b101010110101;
    default_scale_table_[DefaultScale::MINOR_CHORD] = 0b000010001001;

    // Load default scale table
    SetScaleTable({default_scale_table_.data(), default_scale_table_.size()});
    SetScale(DefaultScale::CHROMATIC);
}

void Quantizer::SetScaleTable(std::span<const Scale> scale_table)
{
    scale_table_ = scale_table;
    scale_index_ = 0;
}

float Quantizer::Process(const float frequency)
{
    if (!enabled_)
    {
        return frequency;
    }

    // Compare it to the previous frequency for hysteresis
    if (threshold_enabled_ && (std::max(frequency, prev_frequency_) / std::min(frequency, prev_frequency_)) < threshold_ratio_)
    {
        return prev_frequency_;
    }

    float closest_frequency = kFrequencyTable[0];
    size_t closest_index = 0;
    float min_difference = std::abs(frequency - closest_frequency);

    // Find the closest frequency in the kFrequencyTable
    for (size_t i = 1; i < kFrequencyTable.size(); ++i)
    {
        float difference = std::abs(frequency - kFrequencyTable[i]);
        if (difference < min_difference)
        {
            closest_frequency = kFrequencyTable[i];
            closest_index = i;
            min_difference = difference;
        }
    }

    // Check if the closest frequency's modulo % 12 matches enabled indexes inside scale_table_
    uint32_t mask = 1 << (closest_index % 12);
    if ((scale_table_[scale_index_] & mask) != 0)
    {
        prev_frequency_ = closest_frequency;
        return prev_frequency_;
    }

    // If not, find the next closest frequency that matches the condition
    // Search both upwards and downwards from the closest frequency
    for (size_t i = 1; i < kFrequencyTable.size(); ++i)
    {
        if (closest_index + i < kFrequencyTable.size())
        {
            mask = 1 << ((closest_index + i) % 12);
            if ((scale_table_[scale_index_] & mask) != 0)
            {
                prev_frequency_ = kFrequencyTable[closest_index + i];
                return prev_frequency_;
            }
        }

        if (closest_index >= i)
        {
            mask = 1 << ((closest_index - i) % 12);
            if ((scale_table_[scale_index_] & mask) != 0)
            {
                prev_frequency_ = kFrequencyTable[closest_index - i];
                return prev_frequency_;
            }
        }
    }

    // If no matching frequency is found, return the original frequency
    prev_frequency_ = frequency;
    return prev_frequency_;
}

float Quantizer::ProcessMultiplier(float multiplier)
{
    if (!enabled_)
    {
        return multiplier;
    }

    // Compare it to the previous frequency for hysteresis
    if (threshold_enabled_ && (std::max(multiplier, prev_multiplier_) / std::min(multiplier, prev_multiplier_)) < threshold_ratio_)
    {
        return prev_multiplier_;
    }

    // Scale the multiplier into the LUT range
    float scaled_amount = 1.f;
    // using a forloop to limit the number of division and prevent an endless loop
    for (uint32_t i = 0; i < 20; ++i)
    {
        if (multiplier < kMultiplierMultiplyThreshold)
        {
            multiplier *= 2.f;
            scaled_amount /= 2.f;
        }
        else
        {
            break;
        }
    }
    for (uint32_t i = 0; i < 20; ++i)
    {
        if (multiplier > kMultiplierDivideThreshold)
        {
            multiplier /= 2.f;
            scaled_amount *= 2.f;
        }
        else
        {
            break;
        }
    }

    float closest_multiplier = kMultiplierTable[0];
    size_t closest_index = 0;
    float min_difference = std::abs(multiplier - closest_multiplier);

    // Find the closest frequency in the kFrequencyTable
    for (size_t i = 1; i < kMultiplierTable.size(); ++i)
    {
        float difference = std::abs(multiplier - kMultiplierTable[i]);
        if (difference < min_difference)
        {
            closest_multiplier = kMultiplierTable[i];
            closest_index = i;
            min_difference = difference;
        }
    }

    // Check if the closest multiplier matches enabled indexes inside scale_table_
    uint32_t mask = 1 << ((closest_index + scale_root_offset_) % 12);
    if ((scale_table_[scale_index_] & mask) != 0)
    {
        prev_multiplier_ = closest_multiplier * scaled_amount;
        return prev_multiplier_;
    }

    // If not, find the next closest multiplier that matches the condition
    // Search both upwards and downwards from the closest one
    for (size_t i = 0; i < 12; ++i)
    {
        // search upwards
        mask = 1 << ((closest_index + i + scale_root_offset_) % 12);
        if ((scale_table_[scale_index_] & mask) != 0)
        {
            if (closest_index + i < kMultiplierTable.size()) // still within this octave
            {
                prev_multiplier_ = kMultiplierTable[closest_index + i] * scaled_amount;
                return prev_multiplier_;
            }
            else // in the next octave
            {
                scaled_amount *= 2.f;
                prev_multiplier_ = kMultiplierTable[(closest_index + i) - kMultiplierTable.size()] * scaled_amount;
                return prev_multiplier_;
            }
        }

        // search downwards
        mask = 1 << ((closest_index + 12 - i + scale_root_offset_) % 12);
        if ((scale_table_[scale_index_] & mask) != 0)
        {
            if ((int)closest_index - (int)i >= 0) // still within this octave
            {
                prev_multiplier_ = kMultiplierTable[closest_index - i] * scaled_amount;
                return prev_multiplier_;
            }
            else // in the previous octave
            {
                scaled_amount /= 2.f;
                prev_multiplier_ = kMultiplierTable[(kMultiplierTable.size() + closest_index) - i] * scaled_amount;
                return prev_multiplier_;
            }
        }
    }

    // If no matching multiplier is found, return the original
    prev_multiplier_ = multiplier;
    return prev_multiplier_;
}

void Quantizer::SetScale(const size_t scale_index)
{
    if (scale_index >= GetScaleTableSize())
    {
        return;
    }
    scale_index_ = scale_index;
}

void Quantizer::SetScale(const DefaultScale scale)
{
    SetScale(static_cast<size_t>(scale));
}

size_t Quantizer::GetScale() const
{
    return scale_index_;
}

void Quantizer::SetRoot(const ScaleRoot root)
{
    scale_root_offset_ = static_cast<uint32_t>(root);
}

Quantizer::ScaleRoot Quantizer::GetRoot() const
{
    return static_cast<ScaleRoot>(scale_root_offset_);
}

void Quantizer::SetEnabled(const bool enabled)
{
    enabled_ = enabled;
}

bool Quantizer::IsEnabled() const
{
    return enabled_;
}