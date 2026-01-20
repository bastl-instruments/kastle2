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

#include <cstdint>
#include "common/dsp/utility/Quantizer.hpp"
#include "common/dsp/utility/TriggerGenerator.hpp"

namespace kastle2
{

/**
 * @brief WaveBard file format is specified in WAVE_BARD_FORMAT.md
 * @see https://github.com/bastl-instruments/kastle2/blob/main/code/src/apps/WaveBard/WAVE_BARD_FORMAT.md
 */

typedef struct WaveBardSample
{
    uint32_t size;    // size in real bytes (not number of actual 16-bit samples)
    uint8_t channels; // 1 = mono, 2 = stereo
    // char name[8]; // Don't need to load the name, to save RAM
    uint8_t *data;
} WaveBardSample;

typedef struct WaveBardBank
{
    char name[8];
    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } color;
    WaveBardSample *samples;
} WaveBardBank;

typedef struct WaveBardFile
{
    char magic_string[4];
    uint32_t file_size;
    uint32_t sample_rate;
    uint8_t bit_depth;
    uint8_t num_banks;
    uint8_t num_samples;
    uint8_t num_scales;
    uint8_t num_rhythms;
    uint8_t sequencer_length;
    Quantizer::Scale *scales;
    TriggerGenerator::Rhythm *rhythms;
    WaveBardBank *banks;
    char end_marker[4];
} WaveBardFile;

} // namespace kastle2