/*
MIT License

Copyright (c) 2024 Marek Mach (Bastl Instruments)
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
#include "common/controls/FancyMode.hpp"
#include "common/controls/FancyPot.hpp"
#include "common/core/App.hpp"
#include "common/core/Hardware.hpp"
#include "common/dsp/control/AdsrEnv.hpp"
#include "common/dsp/control/BeatDetector.hpp"
#include "common/dsp/control/EnvelopeFollower.hpp"
#include "common/dsp/effects/SoftClipper.hpp"
#include "common/dsp/filters/DjFilter.hpp"
#include "common/dsp/math/Fraction.hpp"
#include "common/dsp/math/qmath.hpp"
#include "common/dsp/synthesis/Oscillator.hpp"
#include "common/dsp/utility/AdvancedDynamicDelayLine.hpp"
#include "common/dsp/utility/AutoFreeze.hpp"
#include "common/fastcode.hpp"
#include "common/peripherals/WS2812.hpp"
#include "FxWizardFile.hpp"
#include "FxWizardParameterMaps.hpp"
#include "cc.hpp"

// If the dry-wet starts being unresponsive, either change
// value of kDryWetChangeCounterMax and kLowSignal or
// disable this feature by writing a 0 below.
#define NEAR_ZERO_DRYWET 1

namespace kastle2
{

/**
 * @class AppFxWizard
 * @ingroup apps
 * @brief Effects processor firmware for Kastle 2. Released in November 2024.
 * @author Marek Mach (Bastl Instruments), Vaclav Mach (Bastl Instruments)
 * @date 2024-07-16
 */

class AppFxWizard : public virtual App
{
public:
    /**
     * @brief All the modes of the FX Wizard in order.
     * @note Make sure the order matches the LED colors in kLedColors.
     */
    enum class Mode
    {
        DELAY,    // GREEN
        FLANGER,  // CYAN
        FREEZER,  // BLUE
        PANNER,   // WHITE
        CRUSHER,  // YELLOW
        SLICER,   // LIGHT_GREEN
        PITCHER,  // RASPBERRY
        REPLAYER, // ORANGE
        SHIFTER,  // LIGHT PINK
        COUNT,
    };

    /**
     * @brief Mode LED colors, order must match order of modes in the enum above
     */
    static inline const EnumArray<Mode, uint32_t> kLedColors = {
        WS2812::GREEN,
        WS2812::CYAN,
        WS2812::BLUE,
        WS2812::WHITE,
        WS2812::YELLOW,
        WS2812::LIGHT_GREEN,
        WS2812::RASPBERRY,
        WS2812::ORANGE,
        WS2812::LIGHT_PINK,
    };

    /**
     * @brief Initializes all the parameters, memory, etc.
     */
    void Init();

    /**
     * @brief Deinitializes the app, stops all effects, etc.
     */
    void DeInit();

    /**
     * @brief Called each interrupt loop. Implements all the audio processing.
     * @param input Input buffer.
     * @param output Output buffer.
     * @param size Number of sample pairs in the buffer (real size of the buffer is 2*size).
     */
    FASTCODE void AudioLoop(q15_t *input, q15_t *output, size_t size);

    /**
     * @brief Called each time AudioLoop isn't busy.
     */
    void UiLoop();

    /**
     * @brief Called only ONCE when the app is started.
     */
    FASTCODE void SecondCoreWorker();

    /**
     * @brief Called when the app is first loaded - initializes the memory values.
     */
    void MemoryInitialization();

    /**
     * @brief Handles incoming MIDI messages.
     * @param msg The MIDI message to handle.
     */
    void MidiCallback(midi::Message *msg);

    /**
     * @brief Returns the app ID.
     * @return The app ID.
     */
    uint8_t GetId()
    {
        return 0xB1;
    }

private:
    bool inited_ = false;

    static constexpr Hardware::AnalogInput CV_FREE = Hardware::AnalogInput::PITCH_1;
    static constexpr Hardware::AnalogInput CV_STEP = Hardware::AnalogInput::PITCH_2;
    static constexpr Hardware::AnalogInput CV_DRYWET = Hardware::AnalogInput::PARAM_3;
    static constexpr Hardware::AnalogInput CV_FEEDBACK = Hardware::AnalogInput::PARAM_1;
    static constexpr Hardware::AnalogInput CV_MODE = Hardware::AnalogInput::MODE;

    static constexpr size_t kMemMode = Memory::ADDR_APP_SPACE + 0x0;

    // Notes above this trigger the current mode
    // Notes below this change the mode
    static constexpr size_t kMidiMinNote = 48; // C2

    FancyMode mode_selector_ = FancyMode(FancyMode::Config{
        .memory_addr = kMemMode,
        .modes_count = static_cast<size_t>(Mode::COUNT),
        .midi_cc = cc::MODE,
        .midi_attenuator_cc = cc::MODE_MOD,
        .midi_output_cc = cc::OUT_MODE,
        .midi_note_control = FancyPot::MidiNoteControl{
            .start = 0,
            .repeat = 12,
            .end = kMidiMinNote},
    });
    Mode mode_ = Mode::DELAY;
    Mode mode_prev_ = Mode::DELAY;

    size_t trigger_blink_counter = 0;

    q15_t input_left_ = Q15_ZERO;
    q15_t input_right_ = Q15_ZERO;
    q15_t output_left_ = Q15_ZERO;
    q15_t output_right_ = Q15_ZERO;

    EdgeDetector trigger_detect_ = EdgeDetector(EdgeDetector::Type::RISING);
    size_t sample_being_processed_ = 0;

    bool trigger_trigger_ = false;
    bool clock_trigger_ = false;
    bool mode_sh_trigger_ = false;
    bool time_sh_trigger_ = false;

    int32_t cv_step_ = 0;

    /**
     * @brief Potentiometer abstractions
     */
    enum class Pot
    {
        FEEDBACK,
        FEEDBACK_MOD,
        AMOUNT,
        AMOUNT_MOD,
        STEREO,
        TIME,
        TIME_MOD,
        FILTER,
        MODE_MOD,
        COUNT
    };
    EnumArray<Pot, std::unique_ptr<FancyPot>> pots_;

    int32_t cv_mode_ = 0;
    int32_t cv_mode_prev_ = 0;
    int32_t cv_mode_in_use_ = 0;

    static constexpr int32_t kModeCvStep = ADC_5V / static_cast<size_t>(Mode::COUNT);
    static constexpr int32_t kModeCvChanged = ADC_5V / 100;
    static constexpr int32_t kModeCvJumped = ADC_3V;

    q15_t feedback_volume_ = Q15_ZERO;
    size_t feedback_delay_ = 0;
    q15_t global_dry_wet_ = Q15_ZERO;
    q15_t global_dry_wet_queue_ = Q15_ZERO;

    // crusher
    q15_t crusher_threshold_ = Q15_ZERO;
    q15_t crusher_left_ = Q15_ZERO;
    q15_t crusher_right_ = Q15_ZERO;
    q31_t crusher_frequency_ = Q31_ZERO;
    q15_t crusher_stereo_mix_ = Q15_ZERO;
    q15_t crusher_env_mod_ = Q15_ZERO;
    AdsrEnv crusher_env_;
    q31_t crusher_left_frequency_ = Q31_ZERO;
    q31_t crusher_right_frequency_ = Q31_ZERO;
    int16_t crusher_xor_ = 0;

    // flanger
    q15_t flanger_depth_ = Q15_ZERO;
    q15_t flanger_dry_wet_ = Q15_ZERO;
    q31_t flanger_frequency_ = Q31_ZERO;
    q15_t flanger_stereo_mix_ = Q15_ZERO;
    q15_t flanger_stereo_mix_smooth_ = Q15_ZERO;
    size_t flanger_mix_timeout_ = 0;

    // panner
    q15_t panner_depth_ = Q15_ZERO;
    SoftClipper panner_clip_;
    q31_t panner_frequency_ = Q31_ZERO;
    q15_t panner_stereo_mix_ = Q15_ZERO;
    q31_t panner_stereo_left_ = Q31_ZERO;
    q31_t panner_stereo_right_ = Q31_ZERO;

    // freezer
    size_t freezer_length_left_ = 0;
    size_t freezer_length_right_ = 0;
    bool freezer_grab_continuous_ = false;
    bool freezer_grab_continuous_prev_ = false;
    bool freezer_grab_buffer_ = false;
    bool freezer_grab_prev_ = false;
    size_t freezer_grab_cnt_ = 0;
    bool freezer_clock_triggered_ = false;
    bool freezer_wait_for_clock_ = false;
    q15_t freezer_feedback_ = Q15_ZERO;
    q15_t freezer_feedback_dry = Q15_ZERO;
    size_t freezer_record_length_ = 0;
    bool freezer_can_use_feedback_prev_ = false;
    size_t freezer_start_ptr_left_ = 0;
    size_t freezer_start_ptr_right_ = 0;

    // replayer
    q15_t replayer_dry_wet_ = Q15_ZERO;
    size_t replayer_oversampling_ = 0;
    bool replayer_hold_ = false;
    size_t replayer_stereo_oversampling_ = 0;
    bool replayer_filling_ = false;

    // pitcher
    q31_t pitcher_frequency_ = 0.f;
    q15_t pitcher_dry_wet_ = Q15_ZERO;
    q31_t pitcher_depth_ = Q31_ZERO;
    q31_t pitcher_stereo_mix_ = Q15_ZERO;
    AdsrEnv pitcher_env_;
    q15_t pitcher_env_depth_ = Q15_ZERO;
    q15_t pitcher_depth_modulated_ = Q15_ZERO;

    // shifter
    bool shifter_direction_ = 0;
    q15_t shifter_dry_ = Q15_ZERO;
    q31_t shifter_frequency_ = 0.f;
    q31_t shifter_left_frequency_ = Q31_ZERO;
    q31_t shifter_right_frequency_ = Q31_ZERO;
    AdsrEnv shifter_env_;
    q15_t shifter_env_mod_ = Q15_ZERO;

    // delay
    q15_t delay_wet_ = Q15_ZERO;
    q15_t delay_dry_ = Q15_ZERO;
    q15_t delay_feedback_ = Q15_ZERO;
    SoftClipper delay_clip_;
    EnvelopeFollower delay_compressor_;
    size_t delay_peak_counter_ = 0;
    static constexpr size_t delay_peak_counter_max_ = 8;
    q15_t delay_peak_ = Q15_ZERO;
    size_t delay_stereo_ = 0;
    size_t delay_length_ = 0;
    size_t delay_ticks_time_ = 0;
    size_t delay_ticks_time_last_ = 0;
    size_t delay_ticks_time_last_prev_ = 0;
    bool delay_synced_ = false;
    bool delay_synced_first_measurement_ = false;
    std::array<size_t, kDelaySyncedDividers.size()> delay_synced_times_ = {0};
    size_t delay_synced_highest_index_ = 0;
    size_t delay_time_left_prev_ = 0;
    size_t delay_time_right_prev_ = 0;

    // slicer
    size_t slicer_pattern_left_ = 0;
    size_t slicer_pattern_right_ = 0;
    size_t slicer_step_ = 0;
    AdsrEnv slicer_env_left_;
    AdsrEnv slicer_env_right_;
    bool slicer_clock_trigger = false;
    float slicer_decay_ = 0.f;
    bool slicer_lfo_prev_ = false;
    int32_t slicer_stereo_ = 0;
    uint16_t slicer_chance_ = 0;
    bool slicer_flip_bit_ = false;

    std::unique_ptr<AdvancedDynamicDelayLine<q15least_t>> feedback_delay_left_;
    std::unique_ptr<AdvancedDynamicDelayLine<q15least_t>> feedback_delay_right_;
    std::unique_ptr<AdvancedDynamicDelayLine<q15least_t>> delay_left_;
    std::unique_ptr<AdvancedDynamicDelayLine<q15least_t>> delay_right_;
    SoftClipper feedback_clip_;
    Svf feedback_filter_left_;
    Svf feedback_filter_right_;
    Svf feedback_filter_lp_left_;
    Svf feedback_filter_lp_right_;
    DjFilter dj_filter_left_;
    DjFilter dj_filter_right_;
    Oscillator lfo_left_;
    Oscillator lfo_right_;

    void ModeInit();
    void ModeTrigger();
    void WorkLongDelay();

    void ModeCrusherInit();
    void ModeCrusher();
    void ModeFlangerInit();
    void ModeFlanger();
    void ModePannerInit();
    void ModePanner();
    void ModeFreezerInit();
    void ModeFreezer();
    void ModeReplayerInit();
    void ModeReplayer();
    void ModePitcherInit();
    void ModePitcher();
    void ModeSlicerInit();
    void ModeSlicer();
    void ModeShifterInit();
    void ModeShifter();
    void ModeDelayInit();
    void ModeDelay();

    /**
     * @brief Here the second core processes the audio. It is called by the SecondCoreWorker.
     */
    void SecondCoreProcess(size_t index);

    /**
     * @brief How many samples are processed by the second core
     */
    size_t second_core_processed_samples_ = 0;

    /**
     * @brief Total samples second core should process
     */
    size_t buffer_size_ = 0;

    /**
     * @brief Buffer for the second core to use for clean passthrough (usually the input buffer)
     */
    q15_t *input_buffer_;

    /**
     * @brief Buffer for the second core to process (usually the output buffer)
     */
    q15_t *output_buffer_;

    /**
     * @brief Sets dry wet, will wait signals to be close to each other (avoid crackle)
     * @param dry_wet
     */
    void SetDryWet(q15_t dry_wet);

    /**
     * @brief Waits for the signals to get close to each other and than sets the dry-wet
     */
    void CheckDryWet();

    size_t dry_wet_change_counter_ = 0;
    static constexpr size_t kDryWetChangeCounterMax = 200;
    static constexpr q15_t kLowSignal = 3277; // (0.1)

    // For the mode change you need to press it shorter than 1.5s
    static constexpr size_t kModeShortPressUnder = s2alr(1.5f);

    /**
     * @brief Structure which contains rhythms and other settings loaded from file
     */
    FxWizardFile settings_file_;

    /**
     * @brief Loads the settings file from memory
     * @return true if loading was successful, false otherwise
     */
    bool LoadFile();
};
}
