/*
MIT License

Copyright (c) 2026 Vaclav Mach (Bastl Instruments)

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
#include "common/EnumTools.hpp"
#include "common/controls/FancyMode.hpp"
#include "common/controls/FancyPot.hpp"
#include "common/core/App.hpp"
#include "common/core/Hardware.hpp"
#include "common/dsp/control/AdsrEnv.hpp"
#include "common/dsp/effects/StereoDelay.hpp"
#include "common/dsp/filters/Svf.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/synthesis/Fm2.hpp"
#include "common/dsp/utility/Quantizer.hpp"

namespace kastle2
{

/**
 * @class AppExampleSynth
 * @ingroup apps
 * @brief Simple synthesizer example for Kastle 2 platform
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2026-01-19
 */
class AppExampleSynth : public virtual App
{
public:
    /**
     * @brief Synthesizer operating modes
     */
    enum class Mode
    {
        SUBTRACTIVE, ///< Classic synthesis mode (oscillator + filter)
        FM,          ///< "FM" synthesis mode
        COUNT        ///< Total number of modes
    };

    /**
     * @brief Sets up oscillators, filter, envelope, quantizer, delay effects and all controls.
     */
    void Init();

    /**
     * @brief Cleans up resources (if necessary) and marks the app as not initialized.
     */
    void DeInit();

    /**
     * @brief Main audio processing loop
     *
     * @param input Input audio buffer (currently unused)
     * @param output Output stereo audio buffer
     * @param size Number of audio samples to process
     */
    void AudioLoop(q15_t *input, q15_t *output, size_t size);

    /**
     * @brief Main UI control loop
     *
     * Handles trigger processing, mode switching, potentiometer reading,
     * parameter mapping, and LED control. Runs at lower rate than audio loop.
     */
    void UiLoop();

    /**
     * @brief Memory initialization
     */
    void MemoryInitialization();

    /**
     * @brief Get the unique application ID according to the APP_LIST.md
     * @return uint8_t The unique identifier for this app (0xD2)
     */
    uint8_t GetId()
    {
        return 0xD2;
    }

private:
    /** @brief Initialization state flag */
    bool inited_ = false;

    /** @brief LED colors for each synthesis mode */
    EnumArray<Mode, uint32_t> mode_colors_ = {
        WS2812::ORANGE, ///< Color for SUBTRACTIVE mode
        WS2812::GREEN}; ///< Color for FM mode

    /**
     * @brief Enumeration of all potentiometer controls
     * Maps to different hardware potentiometers across different layers:
     * - NORMAL layer: Basic synthesis parameters
     * - SHIFT layer: Modulation and effects parameters
     * - MODE layer: Scale, tuning, and advanced parameters
     */
    enum class Pot
    {
        PITCH,       ///< Main pitch control (POT_5, Normal layer)
        PITCH_MOD,   ///< Pitch attenuversion amount (POT_1, Normal layer)
        TIMBRE,      ///< Timbre control - filter cutoff or FM index (POT_6, Normal layer)
        TIMBRE_MOD,  ///< Timbre attenuversion amount (POT_2, Normal layer)
        ENV,         ///< Envelope control (POT_4, Normal layer)
        ENV_MOD,     ///< Envelope attenuversion amount (POT_4, Shift layer)
        RESONANCE,   ///< Filter resonance or FM ratio (POT_6, Shift layer)
        PITCH_SCALE, ///< Quantizer scale selection (POT_1, Mode layer)
        PITCH_ROOT,  ///< Root note selection (POT_2, Mode layer)
        PITCH_FINE,  ///< Fine pitch tuning (POT_3, Mode layer)
        FX,          ///< Delay effect wet/dry mix (POT_2, Shift layer)
        MODE_MOD,    ///< Mode attenuation control (POT_4, Mode layer)
        COUNT        ///< Total number of potentiometer controls
    };

    /**
     * @brief Shorthand constants for CV input assignments
     */
    static constexpr Hardware::AnalogInput CV_TIMBRE = Hardware::AnalogInput::PARAM_1;     ///< CV input for timbre modulation
    static constexpr Hardware::AnalogInput CV_ENV = Hardware::AnalogInput::PARAM_3;        ///< CV input for envelope modulation
    static constexpr Hardware::AnalogInput CV_MODE = Hardware::AnalogInput::MODE;          ///< CV input for mode selection
    static constexpr Hardware::AnalogInput CV_PITCH_FREE = Hardware::AnalogInput::PITCH_1; ///< CV input for free pitch modulation
    static constexpr Hardware::AnalogInput CV_PITCH_NOTE = Hardware::AnalogInput::PITCH_2; ///< CV input for quantized pitch (V/Oct)

    /** @brief Array of smart pointers to FancyPot objects for all potentiometer controls */
    EnumArray<Pot, std::unique_ptr<FancyPot>> pots_;

    /**
     * @brief Memory addresses for persistent storage of values
     */
    static constexpr size_t kMemMode = Memory::ADDR_APP_SPACE + 0x0;       ///< Memory address for mode selection
    static constexpr size_t kMemFx = Memory::ADDR_APP_SPACE + 0x1;         ///< Memory address for FX (delay) setting
    static constexpr size_t kMemEnvMod = Memory::ADDR_APP_SPACE + 0x2;     ///< Memory address for envelope modulation setting
    static constexpr size_t kMemPitchScale = Memory::ADDR_APP_SPACE + 0x3; ///< Memory address for pitch scale setting
    static constexpr size_t kMemPitchRoot = Memory::ADDR_APP_SPACE + 0x4;  ///< Memory address for pitch root note setting
    static constexpr size_t kMemPitchFine = Memory::ADDR_APP_SPACE + 0x5;  ///< Memory address for fine pitch setting
    static constexpr size_t kMemModeMod = Memory::ADDR_APP_SPACE + 0x6;    ///< Memory address for mode modulation setting

    /** @brief Mode selector for switching between synthesis modes with trigger-based input reading */
    FancyMode mode_selector_ = FancyMode(FancyMode::Config{
        .memory_addr = kMemMode,
        .modes_count = static_cast<uint32_t>(Mode::COUNT),
        .input_reading = FancyMode::InputReading::TRIGGERED});

    /** @brief Edge detector for trigger input processing (rising edge) */
    EdgeDetector trigger_ = EdgeDetector(EdgeDetector::Type::RISING);

    /** @brief Currently active synthesis mode */
    Mode current_mode_ = Mode::SUBTRACTIVE;

    /** @brief FM oscillator for frequency modulation synthesis mode */
    Fm2 fm_osc_;

    /** @brief Basic oscillator for subtractive synthesis mode */
    Oscillator subtractive_osc_;

    /** @brief State variable filter for subtractive synthesis mode */
    Svf filter_;

    /** @brief Pitch quantizer for musical note quantization */
    Quantizer quantizer_;

    /** @brief Stored CV value for quantized pitch input (V/Oct) */
    int32_t pitch_note_cv_ = 0;

    /** @brief ADSR envelope generator */
    AdsrEnv env_;

    /** @brief Current envelope value in q15 format */
    q15_t env_value_ = 0;

    /** @brief Flag indicating whether envelope is active (controlled by ENV pot position) */
    bool env_enabled_ = false;

    /**
     * @brief Handle trigger events
     *
     * Processes trigger input by storing current pitch CV values,
     * updating mode selector, and triggering the envelope generator.
     */
    void Trigger();

    /** @brief Flag indicating a trigger event should be processed in the UI loop */
    bool do_trigger_ = false;

    /**
     * @brief Update delay time based on current clock tempo
     *
     * Calculates delay time based on the average clock ticks and applies
     * filtering to reduce noise in the delay time calculation.
     */
    void UpdateDelayTime();

    /** @brief Stereo delay effect processor */
    StereoDelay stereo_delay_;

    /** @brief Previous delay length value for noise filtering */
    uint32_t prev_stereo_delay_length_ = 0;
};
}
