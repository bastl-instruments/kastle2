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

#include <cstddef>
#include <cstdint>
#include "common/core/Base.hpp"
#include "common/core/Hardware.hpp"
#include "common/core/midi/Handler.hpp"
#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

/**
 * @class App
 * @ingroup core
 * @brief Interface for the Kastle 2 apps. Implement this interface to create a new app.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2023-11-28
 */
class App
{
public:
    /**
     * @brief Initializes all the parameters, memory, etc.
     */
    virtual void Init() = 0;

    /**
     * @brief Frees all the memory, interfaces etc.
     */
    virtual void DeInit() = 0;

    /**
     * @brief Called each interupt loop. Implement your audio processing here.
     * @param input Input buffer.
     * @param output Output buffer.
     * @param size Number of sample pairs in the buffer (real size of the buffer is 2*size).
     */
    virtual void AudioLoop(q15_t *input, q15_t *output, size_t size) = 0;

    /**
     * @brief Called whenever CPU is not busy with audio processing. Implement ADC readings (too slow for audio processing) and all user inputs here.
     */
    virtual void UiLoop() = 0;

    /**
     * @brief Each app (FX Wizard etc.) has a unique ID. 0xBx, 0xCx, 0xDx are reserved for Bastl use. When the value is 0xFF, the functionality is disabled.
     * @note When making an app public, add it to APP_LIST.md
     * @return Unique ID of the app.
     */
    virtual uint8_t GetId() = 0;

    /**
     * @brief Called when the app is first loaded (eg. firmware change).
     */
    virtual void MemoryInitialization() = 0;

    /**
     * @brief Called once when the second core is started. Make while(true) loop there to keep it running.
     */
    virtual void SecondCoreWorker() {};

    /**
     * @brief Called when a MIDI message is received.
     */
    virtual void MidiCallback([[maybe_unused]] midi::Message *msg) {};
};
}