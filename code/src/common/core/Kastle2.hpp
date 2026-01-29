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

#include <memory>
#include <span>
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "common/config.hpp"
#include "common/core/App.hpp"
#include "common/core/Base.hpp"
#include "common/core/Codec.hpp"
#include "common/core/Hardware.hpp"
#include "common/core/Memory.hpp"
#include "common/core/MultiCore.hpp"
#include "common/core/midi/Handler.hpp"
#include "common/debug.hpp"
#include "common/debug/UsbSerial.hpp"
#include "common/testmode/TestMode.hpp"
#include "I2S.hpp"

namespace kastle2
{

/**
 * @class Kastle2
 * @ingroup core
 * @brief Main static class for Kastle2, you can access most of the stuff from here.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-06-03
 */
class Kastle2
{
public:
    /**
     * @brief Hardware (buttons, LEDs, etc.)
     */
    static inline Hardware hw;

    /**
     * @brief Common functions (LFO, sequencer, etc.)
     */
    static inline Base base;

    /**
     * @brief EEPROM memory for storing and loading data.
     */
    static inline Memory memory;

    /**
     * @brief MIDI communication and parsing.
     */
    static inline midi::Handler midi;

    /**
     * @brief Codec initialization and reference.
     */
    static inline Codec codec;

    /**
     * @brief Serial interface over USB for debugging etc. Disabled by default.
     * @note Use `Kastle2::debug.SetEnabled(true)` to enable it.
     */
    static inline UsbSerial debug;

    /**
     * @brief Pointer to the current app.
     */
    static inline App *app = nullptr;

    /**
     * @brief Initializes the Kastle 2 (HW, memory, base, etc.) together with test mode startup message (=version chain).
     * @details The version chain is passed here, because in test mode the RegisterApp isn't reached (TestMode hijacks the main thread).
     * @note Use the function Init() without any parameters if you don't need Kastle 2 to tell you the version.
     * @param version_chain What Test Mode says on the startup.
     */
    static void Init(std::span<const SamplePlayer16bit::Sample> version_chain);

    /**
     * @brief Initializes the Kastle2. Call this from your `main.cpp`.
     * @note This is a dummy function to allow Kastle2::Init() without version chain (list of samples).
     *       It will call Kastle2::Init with an empty version chain.
     * @see Kastle2::Init(const std::array<SamplePlayer16bit::Sample, N> &version_chain)
     */
    static void Init()
    {
        std::array<SamplePlayer16bit::Sample, 0> empty_version_chain;
        Init(empty_version_chain);
    }

    /**
     * @brief Reads the digital inputs (incl. buttons) and updates the LEDs. Call this from the `main.cpp`.
     */
    static void ReadInputs();

    /**
     * @brief Starts audio callback.
     * @param callback Matches I2S::AudioCallback type.
     * @note Make sure to call this after all initialization has been done.
     * @see I2S::AudioCallback
     */
    static void StartAudio(I2S::AudioCallback callback);

    /**
     * @brief Starts the second core with a function. Call before StartAudio.
     * @note Call before StartAudio.
     * @param second_core_worker Function to run on the second core.
     */
    static void StartSecondCore(MultiCore::Worker second_core_worker);

    /**
     * @brief Starts the midi interface with an app-localized callback function
     * @param callback Callback function which gets passed the data received
     */
    static void SetAppMidiCallback(midi::Handler::Callback callback);

    /**
     * @brief MIDI callback for base
     * @param msg Received midi event
     */
    static void BaseMidiCallback(midi::Message *msg);

    /**
     * @brief MIDI callback for test mode
     * @param msg Received midi event
     */
    static void TestModeMidiCallback(midi::Message *msg);

    /**
     * @brief Stores the custom App ID in the EEPROM. If there is a different app stored, call initialization.
     * @param app_to_register Main program class which inherits from App.
     * @return True if different to the stored version and new version stored successfully.
     */
    static bool RegisterApp(App *app_to_register);

    /**
     * @brief In case your UiLoop is too slow, you can call this function to keep the USB working.
     */
    static inline void UsbTask()
    {
        tud_task();
    }

    /**
     * @brief Apps with this ID won't clear or affect the stored value in EEPROM.
     */
    static constexpr uint8_t kDefaultAppId = 0xFF;

private:
    /**
     * @brief When should the LEDs and buttons be updated again. Prevents overloading LEDs and buttons bounce.
     */
    static inline absolute_time_t timeout_read_buttons_;

    /**
     * @brief Test mode object
     */
    static inline std::unique_ptr<TestMode> test_mode_;

    /**
     * @brief Test mode enabled flag.
     */
    static inline bool test_mode_enabled_ = false;

    /**
     * @brief Audio callback for apps.
     * @param input Input buffer.
     * @param output Output buffer.
     * @param size Buffer size.
     */
    static void AudioCallback(q15_t *input, q15_t *output, size_t size);

    /**
     * @brief Audio callback function.
     */
    static inline I2S::AudioCallback audio_callback_;
};
}
