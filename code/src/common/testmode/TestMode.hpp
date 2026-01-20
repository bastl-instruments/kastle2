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
#include <span>
#include <cstddef>
#include "common/core/Hardware.hpp"
#include "common/dsp/control/AdsrEnv.hpp"
#include "common/dsp/control/EnvelopeFollower.hpp"
#include "common/dsp/math/Fft.hpp"
#include "common/dsp/sampling/SamplePlayer.hpp"
#include "common/dsp/synthesis/Oscillator.hpp"
#include "common/peripherals/WS2812.hpp"
#include "common/testmode/TestEntry.hpp"

namespace kastle2
{

/**
 * @class TestMode
 * @ingroup testmode
 * @brief Kastle 2 Test Mode for testing most of the hardware features.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-07-23
 * @details Hold MODE button on startup to enter it.
 *
 * On startup the Kastle 2 will introduce itself and the LEDs will cycle RGB.
 *
 * Buttons:
 * - User needs to press each button
 *
 * Pots:
 * - User needs to turn each pot fully clockwise and counter-clockwise
 *
 * Jacks:
 * - Sync OUT jack -> Sync IN jack
 * - Audio OUT jack -> Audio IN jack
 *
 * Patchbay wiring:
 * - Pulse OUT -> Trig IN
 * - Pulse OUT -> Reset IN
 * - Sync OUT L -> Sync OUT R
 * - Sync IN L -> ADC FEED 1
 * - Sync IN R -> ADC FEED 3
 * - ENV OUT -> ADC PARAM 1
 * - ENV OUT -> ADC PARAM 3
 * - CV OUT -> ADC PARAM 2
 * - CV OUT -> ADC PITCH 1
 * - CV OUT -> ADC PITCH 2
 * - Tri OUT -> ADC MODE
 * - Gate OUT -> ADC FEED 2
 *
 * Each 1 second the test results are printed via USB Serial.
 * When all tests pass, the LEDs will turn green and a success sound will play.
 */

class TestMode
{
public:
    /**
     * @brief Where we are in the test mode.
     */
    enum class Stage
    {
        CALIBRATED,
        INTRO,
        VERSION,
        TESTING,
        SUCCESS,
        CITADEL_PLAYGROUND
    };

    /**
     * @brief Initializes the TestMode, similarly to the App.
     * @param sample_rate The sample rate of the system
     */
    void Init(float sample_rate);

    /**
     * @brief MIDI callback for the TestMode, called by the Kastle2.
     * @param msg Received midi event
     */
    void MidiCallback(midi::Message *msg)
    {
        for (auto &test : tests_)
        {
            test.MidiCallback(msg);
        }
    }

    /**
     * @brief Audio loop of the TestMode, called by the interrupt. Time precise.
     * @param input The input buffer
     * @param output The output buffer
     * @param size The size of the buffer in left and right pairs
     */
    void AudioLoop(q15_t *input, q15_t *output, size_t size);

    /**
     * @brief UI loop of the TestMode, called by the main loop. Not time precise.
     */
    void UiLoop();

    /**
     * @brief What test mode says on the startup.
     * @param version_chain Vector of samples to play on startup
     */
    void SetVersionChain(std::span<const SamplePlayer16bit::Sample> version_chain)
    {
        version_chain_ = version_chain;
    }

private:
    ///> The sample rate of the system
    float sample_rate_;

    ///> The left oscillator
    Oscillator osc_left_;

    ///> The right oscillator
    Oscillator osc_right_;

    ///> Startup envelope not to blow up our ears
    AdsrEnv startup_env_;

    ///> Pitch envelope (for the "ding" sound)
    AdsrEnv pitch_env_;

    ///> The pitch envelope value for the "ding" sound
    q31_t pitch_env_value_;

    ///> Sample player for intro/success sounds
    SamplePlayer16bit sample_player_;

    ///> The version chain for the intro
    std::span<const SamplePlayer16bit::Sample> version_chain_;

    ///> The time for the next print state
    absolute_time_t next_time_print_state_;

    ///> Printing the test results over USB serial
    void PrintState();

    ///> The current stage of the test mode
    Stage stage_;

    ///> Oscillator frequencies
    static constexpr float kTestFrequencyLeft = 140.0f;
    static constexpr float kTestFrequencyRight = 210.0f;

    ///> We want to test just the minimum, since testing can be used with/without headphones which can lower the volume
    static constexpr q15_t kTestVolume = 1300;

    ///> Testing colors on startup
    static constexpr std::array<uint32_t, 3> kTestColors = {WS2812::RED, WS2812::GREEN, WS2812::BLUE};

    /**
     * @brief Sets the LEDs to a specific color and updates them.
     * @param color The color to set the LEDs to
     */
    void SetLeds(uint32_t color);

    /**
     * @brief Sets the current stage of the test mode and initializes it.
     * @param stage The stage to set
     */
    void SetStage(Stage stage);

    /**
     * @brief Intro loop of the test mode.
     */
    void StageIntro();

    /**
     * @brief Testing loop of the test mode.
     */
    void StageTesting();

    /**
     * @brief Success loop of the test mode.
     */
    void StageSuccess();

    /**
     * @brief After the success, there is a playground stage for testing switches and input detection (most useful on Citadel)
     */
    void StageCitadelPlayground();

    /**
     * @brief Plays the "ding" sound.
     */
    void Ding();

    ///> Helper declarations
    static constexpr Hardware::Version BOTH = Hardware::Version::COUNT;
    using TE = TestEntry;
    using TEC = TE::Config;
    using AO = Hardware::AnalogOutput;
    using AI = Hardware::AnalogInput;
    using DO = Hardware::DigitalOutput;
    using DI = Hardware::DigitalInput;
    using POT = Hardware::Pot;
    using BTN = Hardware::Button;

    ///> The test definitions
    std::array<TestEntry, 26> tests_ = {
        TE(TEC::DigitalCombo("PULSE -> TRIG", BOTH, DO::PULSE_OUT, DI::TRIG_IN)),
        TE(TEC::DigitalCombo("PULSE -> RESET", BOTH, DO::PULSE_OUT, DI::RESET_IN)),
        TE(TEC::DigitalCombo("SYNC OUT -> SYNC IN", BOTH, DO::SYNC_OUT, DI::SYNC_IN)),
        TE(TEC::DigitalIn("AUDIO IN JACK DETECT", BOTH, DI::AUDIO_IN_JACK_DETECT)),
        TE(TEC::DigitalIn("SYNC IN JACK DETECT", BOTH, DI::SYNC_IN_JACK_DETECT)),
        TE(TEC::AnalogCombo("ENV -> PARAM 1", BOTH, AO::ENV_OUT, AI::PARAM_1)),
        TE(TEC::AnalogCombo("ENV -> PARAM 3", BOTH, AO::ENV_OUT, AI::PARAM_3)),
        TE(TEC::AnalogCombo("CV -> PARAM 2", BOTH, AO::CV_OUT, AI::PARAM_2)),
        TE(TEC::AnalogCombo("CV -> PITCH 1", BOTH, AO::CV_OUT, AI::PITCH_1)),
        TE(TEC::AnalogCombo("CV -> PITCH 2", BOTH, AO::CV_OUT, AI::PITCH_2)),
        TE(TEC::AnalogCombo("TRI -> MODE", BOTH, AO::TRI_OUT, AI::MODE)),
        TE(TEC::DigitalOutAnalogIn("SYNC IN L -> FEED 1", BOTH, DO::SYNC_OUT, AI::FEED_1)),
        TE(TEC::DigitalOutAnalogIn("SYNC IN R -> FEED 3", BOTH, DO::SYNC_OUT, AI::FEED_3)),
        TE(TEC::DigitalOutAnalogIn("GATE -> FEED 2", BOTH, DO::GATE_OUT, AI::FEED_2)),
        TE(TEC::Fft("AUDIO LEFT", BOTH, TE::Channel::LEFT, kTestVolume, kTestFrequencyLeft)),
        TE(TEC::Fft("AUDIO RIGHT", BOTH, TE::Channel::RIGHT, kTestVolume, kTestFrequencyRight)),
        TE(TEC::MidiIn("MIDI CLK IN", Hardware::Version::CITADEL, midi::Message::Type::CLOCK)),
        TE(TEC::Button("BUTTON SHIFT", BOTH, BTN::SHIFT)),
        TE(TEC::Button("BUTTON MODE", BOTH, BTN::MODE)),
        TE(TEC::Pot("POT 1", BOTH, POT::POT_1)),
        TE(TEC::Pot("POT 2", BOTH, POT::POT_2)),
        TE(TEC::Pot("POT 3", BOTH, POT::POT_3)),
        TE(TEC::Pot("POT 4", BOTH, POT::POT_4)),
        TE(TEC::Pot("POT 5", BOTH, POT::POT_5)),
        TE(TEC::Pot("POT 6", BOTH, POT::POT_6)),
        TE(TEC::Pot("POT 7", BOTH, POT::POT_7))};

    ///> Longest test name (we need that for the printing buffer)
    static constexpr size_t kLongestTestName = 20;

    /**
     * @brief Moves to the next version sample in the version chain.
     * @return true if there is a next sample, false if we are at the end of the chain
     */
    bool NextVersionSample();

    ///> The index of the current version sample in the version chain
    size_t version_index_ = 0;
};
}
