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

#include <bitset>
#include <cstddef>
#include <cstdint>
#include "common/controls/FancyPot.hpp"
#include "common/core/Clock.hpp"
#include "common/core/Codec.hpp"
#include "common/core/FakeBlinker.hpp"
#include "common/core/Hardware.hpp"
#include "common/core/Kastle2_parameters.hpp"
#include "common/core/Memory.hpp"
#include "common/core/midi/Message.hpp"
#include "common/dsp/control/AdsrEnv.hpp"
#include "common/dsp/control/EnvelopeFollower.hpp"
#include "common/dsp/control/Lfo.hpp"
#include "common/dsp/math/Fraction.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/utility/NumberFlasher.hpp"
#include "common/dsp/utility/Sequencer.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class Base
 * @ingroup core
 * @brief Core software running on the Kastle 2. Handles tempo, LFO, volume, sequencer, syncing etc.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-04-02
 * @note You can disable and enable features like LFO, CV outputs if you want to control them yourself.
 * @see Kastle2::SetFeatureEnabled()
 */
class Base
{
public:
    /**
     * @brief Set of features that can be enabled or disabled. By default all are enabled.
     */
    enum class Feature
    {
        BASE,                  ///< The base class itself (all features).
        LFO_OUT,               ///< Triangle and Pulse LFO outputs.
        ENV_OUT,               ///< ENV output, by default envelope follower output.
        CV_OUT,                ///< CV output, by default KastleRungler output.
        GATE_OUT,              ///< Gate output.
        SYNC_OUT,              ///< Sync output (both jack and pinheader).
        INPUT_GAIN,            ///< Input gain handling (SHIFT + POT).
        INPUT_INDICATION,      ///< Input loudness indication on the top left LED
        INPUT_INDICATION_CLIP, ///< Shows red clipping LED even if not adjusting input gain
        OUTPUT_GAIN,           ///< Output gain handling (SHIFT + POT).
        AUDIO_CHAIN,           ///< Audio chain (input -> output), disable in processing the audio by yourself.
        MIDI_CLOCK,            ///< MIDI input clock handling.
        GATE_INDICATION,       ///< Gate indication on the top right LED.
        COUNT
    };

    /**
     * @brief Initializes the Base class.
     */
    void Init();

    /**
     * @brief Enables or disables a feature.
     * @param feature The feature to enable or disable.
     * @param enabled True to enable, false to disable.
     */
    void SetFeatureEnabled(Feature feature, bool enabled);

    /**
     * @brief Checks if a feature is enabled.
     * @param feature The feature to check.
     * @return True if the feature is enabled, false otherwise.
     */
    inline bool IsFeatureEnabled(Feature feature) const;

    /**
     * @brief Enables or disables all features.
     * @param enabled True to enable, false to disable.
     */
    void SetAllFeaturesEnabled(bool enabled);

    /**
     * Should be called at the start of apps audio loop.
     * Handles input volume, steps LFO and clock.
     */
    FASTCODE void BeforeAudioLoop(q15_t *input, size_t size);

    /**
     * Should be called at the end of apps audio loop.
     * Handles output volume.
     */
    FASTCODE void AfterAudioLoop(q15_t *input, q15_t *output, size_t size);

    /**
     * Should be called at the start of apps UI loop.
     * Reads all the pots (input gain, output gain, LFO, tempo).
     * Handles switching pot layers.
     */
    void BeforeUiLoop();

    /**
     * @brief Called after the app UI loop finished...
     */
    void AfterUiLoop();

    /**
     * Midi callback for syncing to midi clock
     */
    void MidiCallback(midi::Message *msg);

    /**
     * Returns the main Clock object for use in synced LFOs etc.
     * @return Clock& Reference to the Clock object.
     */
    Clock &GetClock();

    /**
     * Returns the sequencer object
     * @return Sequencer& Reference to the sequencer object.
     */
    Sequencer &GetSequencer();

    /**
     * Returns the LFO object
     * @return Lfo& Reference to the LFO object.
     */
    Lfo &GetLfo();

    /**
     * @brief Set the HP amp output volume in range 0-63. Ideally keep it under 60 to prevent noise etc.
     * @note Default is set by `kDefaultMaxVolume` which is 53.
     * @param val HP amp volume to set (0-63).
     */
    void SetMaxVolume(size_t max_volume);

    /**
     * @brief Returns the pulsing LED value of Advanced Settings
     * @return uint8_t LFO value between 0-255
     */
    inline uint8_t GetSettingsLedPulse() const
    {
        return std::abs(settings_pulse_) + 64;
    }

    /**
     * @brief Returns interrupt ticks inside the current layer
     * @return size_t Ticks inside the current layer
     */
    inline size_t GetLayerTimer() const
    {
        return layer_timer_;
    }

    /**
     * @brief Returns previous layer
     * @return Hardware::Layer Previous layer
     */
    inline Hardware::Layer GetPrevLayer() const
    {
        return prev_layer_;
    }

    /**
     * @brief Returns interrupt ticks inside the current layer
     * @return size_t Ticks inside the current layer
     */
    inline size_t GetPrevLayerTimer() const
    {
        return prev_layer_timer_;
    }
    /**
     * @brief Returns Lfo Triangle output
     * @return size_t value from 0-1023
     */
    inline size_t GetLfoTriangle() const
    {
        return lfo_triangle_value_;
    }

    /**
     * @brief Enables or disables the MIDI CC output for a specific pot.
     * @param pot The pot to enable or disable.
     * @param enabled True to enable the MIDI CC output, false to disable it.
     * @note All pots are enabled by default.
     */
    void SetMidiOutPotEnabled(Hardware::Pot pot, bool enabled)
    {
        midi_pots_enabled_.set(static_cast<size_t>(pot), enabled);
    }

    /**
     * @brief Returns the FakeBlinker instance.
     * @return FakeBlinker& Reference to the FakeBlinker instance.
     */
    inline FakeBlinker &GetFakeBlinker()
    {
        return fake_blinker_;
    }

    /**
     * @brief Returns the input envelope follower.
     * @return EnvelopeFollower& Reference to the input envelope follower.
     */
    inline EnvelopeFollower &GetInputEnvelopeFollower()
    {
        return input_envelope_follower_;
    }

private:
    // How much amplify the input volume
    static constexpr int8_t kInputGainShiftLeft = 3;

    // Max volume
    static constexpr size_t kDefaultMaxVolume = 53;
    size_t max_volume_ = 0;

    // LFO (can be tempo synced or free running)
    Lfo lfo_;
    uint8_t lfo_self_patched_ = 0;
    bool lfo_state_prev_ = false;
    uint32_t lfo_mod_state_prev = 0;
    uint32_t lfo_change_timer_ = 0;
    bool lfo_last_timer_source_ = false;

    // Main tempo of the device
    Clock clock_;
    bool clock_midi_pulse_ = false;

    // Fake blinker to prevent interferences when LFO/Tempo is too fast
    FakeBlinker fake_blinker_;

    // Outputs
    void UpdateCvOut();
    void UpdateGateOut();

    // Keeping the value here to handle hysteresis
    bool lfo_sync_ = false;
    uint32_t lfo_pot_ratio_ = 0;

    // Features
    std::bitset<static_cast<unsigned int>(Feature::COUNT)> features_enabled_;

    // Current LFO 10-bits value (0-1023)
    uint32_t lfo_triangle_value_ = 0;
    // Current pulse LFO 1-but value
    bool lfo_pulse_value_ = false;

    // Sequencer
    Sequencer sequencer_;
    EdgeDetector sequencer_edge_detector_{EdgeDetector::Type::RISING};

    // Volumes
    q15_t sw_input_gain_ = 0;
    q15_t sw_output_gain_ = 0;

    AdsrEnv startup_env_;

    /**
     * @brief First fade in output, then input, then it's ready.
     */
    enum class StartupEnvState
    {
        OUTPUT,
        INPUT,
        FINISHED
    };
    StartupEnvState startup_env_state_ = StartupEnvState::OUTPUT;

    // Pots
    enum class Pot
    {
        TEMPO,
        INPUT,
        LFO,
        LFO_MOD,
        OUTPUT,
        RHYTHM,
        MONO_INPUT,
        SYNC_INPUT,
        COUNT
    };
    EnumArray<Pot, std::unique_ptr<FancyPot>> pots_;

    // MIDI Out Pot stuff
    EnumArray<Hardware::Pot, std::unique_ptr<FancyPot>> midi_pots_;
    std::bitset<static_cast<unsigned int>(Hardware::Pot::COUNT)> midi_pots_enabled_;

    // Input states
    bool prev_reset_ = false;

    // Settings (audio, sync)
    Memory::MonoSetting mono_setting_ = Memory::MonoSetting::STEREO;
    Memory::SyncSetting sync_setting_ = Memory::SyncSetting::NONE_DISABLED;

    // Sync stuff
    bool sync_thru_ = false; // Whether to pass the sync signal through or whether to generate it (dividers/multipliers)

    // Layers stuff
    void LayersHandling();
    size_t shift_and_mode_pressed_count_ = 0;
    bool settings_toggled_ = false;
    bool leds_should_be_off_ = false;
    Hardware::Layer prev_layer_ = Hardware::Layer::NORMAL;
    size_t layer_timer_ = kShiftShortPressTicks;
    size_t prev_layer_timer_ = 0;

    // Input loudness indication
    static constexpr size_t kClippingShowTicks = 300;
    EnvelopeFollower input_envelope_follower_;
    size_t input_clipping_counter_ = 0;

    // Settings stuff
    int32_t settings_pulse_ = 0;
    static constexpr int32_t kSettingsHysteresis = 40;

    // MIDI stuff
    void MidiAdvancedSettings();
    NumberFlasher midi_number_flasher_;
    uint32_t midi_channel_taps_ = 0;
    bool midi_channel_tapping_active = false;
    bool midi_learn_start_allowed_ = false;
};
}
