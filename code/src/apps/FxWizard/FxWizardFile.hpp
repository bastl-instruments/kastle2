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

#include <cstdint>
#include "common/dsp/utility/TriggerGenerator.hpp"

namespace kastle2
{

/**
 * @brief FxWizard file format is specified in FX_WIZARD_FILE_FORMAT.md
 * @see https://github.com/bastl-instruments/kastle2/blob/main/code/src/apps/FxWizard/FX_WIZARD_FILE_FORMAT.md
 */

typedef struct FxWizardFile
{
    char magic_string[4];
    uint32_t file_size;
    uint8_t num_rhythms;
    uint8_t sequencer_length;
    TriggerGenerator::Rhythm *rhythms;
    char end_marker[4];
} FxWizardFile;

} // namespace kastle2