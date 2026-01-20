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

#include <array>
#include <cstddef>
#include <cstdint>
#include "common/EnumTools.hpp"
#include "common/core/App.hpp"
#include "common/core/Hardware.hpp"
#include "common/core/Kastle2.hpp"
#include "common/dsp/control/EnvelopeFollower.hpp"
#include "common/dsp/sampling/SamplePlayer.hpp"
#include "Sentence.hpp"
#include "samples_calibration.hpp"

namespace kastle2
{

/**
 * @class AppCalibration
 * @ingroup apps
 * @brief Special calibration app for Kastle 2. Load it and calibrate your device using step-by-step instructions.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-09-02
 */
class AppCalibration : public virtual App
{
public:
    /**
     * @brief Audio sentence identifiers for calibration instructions
     */
    enum class SentenceName
    {
        KASTLE_2_ALREADY_CALIBRATED, ///< Kastle 2 already calibrated message
        CITADEL_ALREADY_CALIBRATED,  ///< Citadel already calibrated message
        CALIBRATION_CLEARED,         ///< Calibration cleared message
        KASTLE_2_CALIBRATION,        ///< Start Kastle 2 calibration message
        CITADEL_CALIBRATION,         ///< Start Citadel calibration message
        GREAT,                       ///< Success message
        VOLTAGE_TOO_LOW,             ///< Voltage too low error message
        VOLTAGE_TOO_HIGH,            ///< Voltage too high error message
        KASTLE_2_CALIBRATED_ENJOY,   ///< Kastle 2 calibration complete message
        CITADEL_CALIBRATED_ENJOY,    ///< Citadel calibration complete message
        COUNT                        ///< Total number of sentences
    };

    /**
     * @brief Calibration stages that define the current state of the calibration process
     */
    enum class Stage
    {
        ALREADY_CALIBRATED,  ///< Device is already calibrated
        CALIBRATION_CLEARED, ///< Calibration has been cleared
        INTRO,               ///< Introduction/welcome stage
        CALIBRATING,         ///< Currently performing calibration steps
        SUCCESS,             ///< Calibration completed successfully
        COUNT                ///< Total number of stages
    };

    /**
     * @brief Input types for calibration (corresponding to physical pitch inputs)
     */
    enum class Input
    {
        FREE = 0, ///< Free pitch input (PITCH1)
        NOTE = 1, ///< Note pitch input (PITCH2)
        COUNT     ///< Total number of inputs
    };

    /**
     * @brief Voltage levels used for calibration reference
     */
    enum class Voltage
    {
        V0,   ///< 0 volts reference
        V1,   ///< 1 volt reference
        V2,   ///< 2 volts reference
        V3,   ///< 3 volts reference
        V4,   ///< 4 volts reference
        COUNT ///< Total number of voltage levels
    };

    /**
     * @brief Defines a single calibration step with input, voltage, and hardware version
     */
    struct CalibrationStep
    {
        Input input;               ///< Which input to calibrate
        Voltage voltage;           ///< What voltage level to apply
        Hardware::Version version; ///< Which hardware version this step applies to

        /**
         * @brief Constructor for version-specific calibration step
         * @param _input Input type to calibrate
         * @param _voltage Voltage level to apply
         * @param _version Hardware version this step applies to
         */
        constexpr CalibrationStep(Input _input,
                                  Voltage _voltage,
                                  Hardware::Version _version) : input(_input),
                                                                voltage(_voltage),
                                                                version(_version) {}

        /**
         * @brief Constructor for universal calibration step (applies to all versions)
         * @param _input Input type to calibrate
         * @param _voltage Voltage level to apply
         */
        constexpr CalibrationStep(Input _input,
                                  Voltage _voltage) : input(_input),
                                                      voltage(_voltage)
        {
            // COUNT means it's for both versions
            version = Hardware::Version::COUNT;
        }
    };

    /**
     * @brief Initialize the calibration app
     *
     * Sets up sample player, envelope follower, disables hardware calibrations,
     * and determines initial stage based on existing calibration state.
     */
    void Init();

    /**
     * @brief Cleanup and deinitialize the calibration app
     */
    void DeInit();

    /**
     * @brief Audio processing loop that plays calibration instructions
     * @param input Audio input buffer (unused)
     * @param output Audio output buffer for calibration instructions
     * @param size Buffer size in samples
     */
    void AudioLoop(q15_t *input, q15_t *output, size_t size);

    /**
     * @brief UI processing loop that handles button presses and ADC readings
     *
     * Manages calibration state machine, processes user input,
     * reads ADC values, and controls LED feedback.
     */
    void UiLoop();

    /**
     * @brief Memory initialization (no-op for this app)
     */
    void MemoryInitialization() {}

    /**
     * @brief Set the current calibration stage and play appropriate audio
     * @param stage The new calibration stage to enter
     */
    void SetStage(Stage stage);

    /**
     * @brief Start the calibration sequence from the first step
     */
    void StartCalibrationSequence();

    /**
     * @brief Process the current calibration step
     *
     * Either plays instruction audio or tests voltage depending on state.
     * Automatically skips steps that don't apply to current hardware version.
     */
    void ProcessCurrentCalibrationStep();

    /**
     * @brief Move to the next calibration step
     *
     * Advances to next step if voltage was correct, otherwise retries current step.
     */
    void MoveToNextCalibrationStep();

    /**
     * @brief Play a predefined sentence by name
     * @param sentence_name The sentence identifier to play
     */
    void PlaySentence(SentenceName sentence_name);

    /**
     * @brief Generate and play instruction sentence for specific input and voltage
     * @param input Which input to give instructions for
     * @param voltage What voltage level to instruct user to apply
     */
    void PlayInstructionSentence(Input input, Voltage voltage);

    /**
     * @brief Check if current sentence finished playing and advance if needed
     */
    void CheckSentencePlayState();

    /**
     * @brief Check if measured voltage is within acceptable range of target
     * @param value Measured ADC value
     * @param target Target ADC value
     * @return true if voltage is within tolerance, false otherwise
     */
    bool CheckVoltage(int32_t value, int32_t target);

    /**
     * @brief Get the calibration type for a specific input and voltage combination
     * @param input Input type (FREE or NOTE)
     * @param voltage Voltage level
     * @return Hardware::Calibration type for this combination
     */
    Hardware::Calibration GetCalibrationType(Input input, Voltage voltage);

    /**
     * @brief Get the app ID (always returns default ID for calibration)
     * @return Default app ID
     */
    uint8_t GetId()
    {
        return Kastle2::kDefaultAppId;
    }

private:
    static constexpr int32_t kTolerance = ADC_1V / 10; ///< 100mV tolerance for voltage calibration
    static constexpr q15_t kVolume = q15(0.25f);       ///< Audio output volume (25% to prevent ear damage)

    bool inited_ = false;                        ///< Flag indicating if app is initialized
    bool next_time_write_calibrations_ = false;  ///< Flag to write calibrations on next UI loop
    bool already_cleared_ = false;               ///< Flag to prevent multiple calibration clears
    absolute_time_t last_mode_press_ = nil_time; ///< Last mode button press time for debouncing

    static constexpr size_t kCalibrationStepsCount = static_cast<size_t>(Hardware::Calibration::COUNT);

    /**
     * @brief Complete calibration sequence defining which input and voltage to calibrate
     *
     * The sequence covers all required calibration points for both hardware versions.
     * Steps marked with specific hardware versions are only executed on that hardware.
     */
    inline static const std::array<CalibrationStep, kCalibrationStepsCount> calibration_sequence_ = {
        {{Input::NOTE, Voltage::V0, Hardware::Version::CITADEL},
         {Input::NOTE, Voltage::V1},
         {Input::NOTE, Voltage::V2},
         {Input::NOTE, Voltage::V3},
         {Input::NOTE, Voltage::V4},
         {Input::FREE, Voltage::V4},
         {Input::FREE, Voltage::V3},
         {Input::FREE, Voltage::V2},
         {Input::FREE, Voltage::V1},
         {Input::FREE, Voltage::V0, Hardware::Version::CITADEL}}};

    /**
     * @brief Audio samples for voltage level instructions
     *
     * Maps each voltage level to its corresponding audio instruction sample.
     */
    inline static const EnumArray<Voltage, SamplePlayer16bit::Sample> voltage_samples_ = {
        sample_keep_unconnected_or_patch0volts, ///< V0: Keep unconnected or patch 0 volts
        sample_patch1volt,                      ///< V1: Patch 1 volt
        sample_patch2volts,                     ///< V2: Patch 2 volts
        sample_patch3volts,                     ///< V3: Patch 3 volts
        sample_patch4volts                      ///< V4: Patch 4 volts
    };

    // Calibration state
    size_t current_calibration_step_ = 0; ///< Current step in the calibration sequence

    Hardware::CalibrationsType ref_calibrations_; ///< Reference calibration values for the selected hardware (Kastle 2 or Citadel)
    Hardware::CalibrationsType calibrations_;     ///< Measured calibration values to be saved

    SamplePlayer16bit sample_player_; ///< Audio player for calibration instructions
    EnvelopeFollower env_follower_;   ///< Envelope follower of the sample player for LED brightness control

    Stage current_stage_;             ///< Current calibration stage
    bool is_testing_voltage_ = false; ///< Flag indicating if currently testing voltage
    bool voltages_ok_ = false;        ///< Flag indicating if last voltage test passed

    /**
     * @brief Running averages of ADC readings for each input
     *
     * Maintains 20-sample running averages to smooth out ADC noise.
     */
    EnumArray<Input, RunningAverage<int32_t>> adc_readings_ = {
        RunningAverage<int32_t>(20),  ///< FREE input running average
        RunningAverage<int32_t>(20)}; ///< NOTE input running average

    Sentence *current_sentence_;            ///< Pointer to currently playing sentence
    Sentence voltage_instruction_sentence_; ///< Dynamically generated voltage instruction sentence

    /**
     * @brief Predefined sentences for calibration audio feedback
     *
     * Contains all the audio sentences used throughout the calibration process,
     * including device-specific messages and status updates.
     */
    EnumArray<SentenceName, Sentence> sentences_ = {
        Sentence(Sentence::Words{sample_kastle2, sample_already_calibrated}), ///< Kastle 2 already calibrated
        Sentence(Sentence::Words{sample_citadel, sample_already_calibrated}), ///< Citadel already calibrated
        Sentence(Sentence::Words{sample_calibration_cleared}),                ///< Calibration cleared
        Sentence({sample_kastle2, sample_calibration}),                       ///< Start Kastle 2 calibration
        Sentence({sample_citadel, sample_calibration}),                       ///< Start Citadel calibration
        Sentence(Sentence::Words{sample_great}),                              ///< Success feedback
        Sentence({sample_voltage_too, sample_low}),                           ///< Voltage too low error
        Sentence({sample_voltage_too, sample_high}),                          ///< Voltage too high error
        Sentence({sample_kastle2, sample_is_now_calibrated, sample_enjoy}),   ///< Kastle 2 calibration complete
        Sentence({sample_citadel, sample_is_now_calibrated, sample_enjoy})};  ///< Citadel calibration complete
};

}
