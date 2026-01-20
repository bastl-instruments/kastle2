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
#include <cstddef>
#include "common/core/Hardware.hpp"
#include "common/core/midi/Message.hpp"
#include "common/dsp/control/EnvelopeFollower.hpp"
#include "common/dsp/math/Fft.hpp"

namespace kastle2
{

/**
 * @class TestEntry
 * @ingroup testmode
 * @brief Single test entry to be used in Test Mode.
 * @details Can test buttons, pots, digital inputs, digital outputs, analog inputs, analog outputs and FFT.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-07-23
 */
class TestEntry
{
public:
    /**
     * @brief Type of the test.
     */
    enum class Type
    {
        BUTTON,                ///< Button press test
        POT,                   ///< Pot needs to be turned to the minimum and maximum
        DIGITAL_IN,            ///< Digital input works (just needs triggering from anything)
        DIGITAL_COMBO,         ///< Digital output is connected to digital input (tests low and high)
        ANALOG_COMBO,          ///< Analog output is connected to analog input (tests 0V, 1V, 2V, 3V, 4V)
        DIGITAL_OUT_ANALOG_IN, ///< Digital output is connected to analog input (tests LOW, HIGH)
        FFT,                   ///< Audio output is connected to audio input (tests FFT frequency and amplitude)
        MIDI_IN,               ///< Test MIDI message recognition
    };

    /**
     * @brief Audio channel.
     */
    enum class Channel
    {
        LEFT,
        RIGHT
    };

    /**
     * @brief Configuration struct for creating test entries
     */
    struct Config
    {
        const char *name;          ///< Name of the test
        Type type;                 ///< Type of test to perform
        Hardware::Version version; ///< Hardware version this test applies to (COUNT means all versions)

        // All possible parameters, only relevant ones used based on type
        Hardware::Button button = {};                ///< For BUTTON tests
        Hardware::Pot pot = {};                      ///< For POT tests
        Hardware::DigitalInput digital_input = {};   ///< For DIGITAL_IN tests
        Hardware::DigitalOutput digital_output = {}; ///< For DIGITAL_COMBO and DIGITAL_OUT_ANALOG_IN tests
        Hardware::AnalogOutput analog_output = {};   ///< For ANALOG_COMBO tests
        Hardware::AnalogInput analog_input = {};     ///< For ANALOG_COMBO and DIGITAL_OUT_ANALOG_IN tests
        Channel channel = Channel::LEFT;             ///< For FFT tests
        q15_t match_volume = 0;                      ///< For FFT tests
        float match_frequency = 0.0f;                ///< For FFT tests
        midi::Message::Type midi_message_type = {};  ///< For MIDI_IN tests

        // Named parameter constructors for type-safe construction
        static Config Button(const char *name, Hardware::Version ver, Hardware::Button btn)
        {
            return Config{
                .name = name,
                .type = Type::BUTTON,
                .version = ver,
                .button = btn};
        }

        static Config Pot(const char *name, Hardware::Version ver, Hardware::Pot p)
        {
            return Config{
                .name = name,
                .type = Type::POT,
                .version = ver,
                .pot = p};
        }

        static Config DigitalIn(const char *name, Hardware::Version ver, Hardware::DigitalInput input)
        {
            return Config{
                .name = name,
                .type = Type::DIGITAL_IN,
                .version = ver,
                .digital_input = input};
        }

        static Config DigitalCombo(const char *name, Hardware::Version ver, Hardware::DigitalOutput output, Hardware::DigitalInput input)
        {
            return Config{
                .name = name,
                .type = Type::DIGITAL_COMBO,
                .version = ver,
                .digital_input = input,
                .digital_output = output};
        }

        static Config AnalogCombo(const char *name, Hardware::Version ver, Hardware::AnalogOutput output, Hardware::AnalogInput input)
        {
            return Config{
                .name = name,
                .type = Type::ANALOG_COMBO,
                .version = ver,
                .analog_output = output,
                .analog_input = input};
        }

        static Config DigitalOutAnalogIn(const char *name, Hardware::Version ver, Hardware::DigitalOutput output, Hardware::AnalogInput input)
        {
            return Config{
                .name = name,
                .type = Type::DIGITAL_OUT_ANALOG_IN,
                .version = ver,
                .digital_output = output,
                .analog_input = input};
        }

        static Config Fft(const char *name, Hardware::Version ver, Channel ch, q15_t volume, float frequency)
        {
            return Config{
                .name = name,
                .type = Type::FFT,
                .version = ver,
                .channel = ch,
                .match_volume = volume,
                .match_frequency = frequency};
        }

        static Config MidiIn(const char *name, Hardware::Version ver, midi::Message::Type msg_type)
        {
            return Config{
                .name = name,
                .type = Type::MIDI_IN,
                .version = ver,
                .midi_message_type = msg_type};
        }
    };

    /**
     * @brief Constructs a new test entry with the given configuration
     * @param config The test configuration
     */
    explicit TestEntry(const Config &config);

    /**
     * @brief Runs the test.
     */
    void Run();

    /**
     * @brief Stores the audio samples so the Run can check them (only in FFT mode)
     */
    void TestAudioBlock(q15_t *audio_block, size_t size);

    /**
     * @brief Returns if the test has passed.
     * @return true if the test has passed
     */
    bool HasPassed() const;

    /**
     * @brief User interaction needed for passing this test
     * @return true if manual interaction is needed
     */
    bool IsManual() const;

    /**
     * @brief Returns the name of the test
     * @return name of the test
     */
    const char *GetName() const;

    /**
     * @brief MIDI callback for this test entry
     * @param msg Received MIDI message
     */
    void MidiCallback(midi::Message *msg);

    /**
     * @brief Checks if this test should be skipped on current hardware
     * @return true if the test should be skipped
     */
    bool IsSkipped() const;

    ///> ADC low threshold
    static constexpr int32_t kLow = 400;

    ///> ADC high threshold
    static constexpr int32_t kHigh = 3400;

    ///> How many ADC values we are testing
    static constexpr int32_t kAnalogTestsCount = 5;

    ///> DAC values we set
    static constexpr std::array<int32_t, kAnalogTestsCount> kAnalogTestValues = {DAC_0V, DAC_1V, DAC_2V, DAC_3V, DAC_4V};

    ///> ADC values we expect
    static constexpr std::array<int32_t, kAnalogTestsCount> kAnalogTestResults = {ADC_0V, ADC_1V, ADC_2V, ADC_3V, ADC_4V};

    ///> Kinda generous, but should be all right if we match it
    static constexpr int32_t kAnalogTestsTolerance = 500;

    ///> How much we allow the FFT frequency to differ from the expected one
    static constexpr float kFftTestFrequencyTolerance = 20.0f;

    ///> Hold frequency for a bit to make sure it's stable
    static constexpr size_t kFftHoldCount = 4;

    ///> FFT buffer size
    static constexpr size_t kFftBufferSize = 2048;

private:
    Config config_;
    bool passed_ = false;

    void RunButton();
    void RunPot();
    void RunDigitalInput();
    void RunDigitalCombo();
    void RunAnalogCombo();
    void RunDigitalOutputAnalogInput();
    void RunFft();
    void RunMidiIn();

    void UpdateAllAdcs();

    bool prev_state_ = false;

    bool low_reached_ = false;
    bool high_reached_ = false;

    // Audio analysis stuff
    EnvelopeFollower env_follower_;
    std::unique_ptr<Fft> fft_;
    size_t fft_hold_count_ = 0;

    // MIDI message stuff
    midi::Message::Type midi_message_type_;
    bool midi_message_received_ = false;
};
}
