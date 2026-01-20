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
#include "common/EnumTools.hpp"
#include "common/controls/DirtyInputsHandler.hpp"
#include "common/controls/FancyMode.hpp"
#include "common/controls/FancyPot.hpp"
#include "common/core/App.hpp"
#include "common/core/Hardware.hpp"
#include "common/core/midi/Message.hpp"
#include "common/core/midi/NoteSender.hpp"
#include "common/dsp/control/AdsrEnv.hpp"
#include "common/dsp/control/EnvelopeFollower.hpp"
#include "common/dsp/effects/HardClipper.hpp"
#include "common/dsp/effects/SoftClipper.hpp"
#include "common/dsp/filters/DjFilterStereo.hpp"
#include "common/dsp/sampling/SamplePlayer.hpp"
#include "common/dsp/synthesis/Oscillator.hpp"
#include "common/dsp/utility/AdvancedDynamicDelayLine.hpp"
#include "common/dsp/utility/AutoFreeze.hpp"
#include "common/dsp/utility/Quantizer.hpp"
#include "common/dsp/utility/Slewer.hpp"
#include "common/fastcode.hpp"
#include "common/peripherals/WS2812.hpp"
#include "WaveBardFile.hpp"
#include "cc.hpp"

namespace kastle2
{

/**
 * @class AppWaveBard
 * @ingroup apps
 * @brief Sample Player firmware for Kastle 2. Released in May 2025.
 * @author Marek Mach (Bastl Instruments), Vaclav Mach (Bastl Instruments)
 * @date 2024-11-14
 */
class AppWaveBard : public virtual App
{
public:
    /**
     * @brief Initializes all the parameters etc.
     */
    void Init();

    /**
     * @brief Frees all the stuff.
     */
    void DeInit();

    /**
     * @brief Called each interupt loop. Audio processing is done here.
     * @param input Input buffer.
     * @param output Output buffer.
     * @param size Number of sample pairs in the buffer (real size of the buffer is 2*size).
     */
    FASTCODE void AudioLoop(q15_t *input, q15_t *output, size_t size);

    /**
     * @brief Called whenever CPU is not busy with audio processing. Implement ADC processing and all user inputs here.
     */
    void UiLoop();

    /**
     * @brief Called once on the startup of the app - while(true) loop isß be implemented inside.
     */
    FASTCODE void SecondCoreWorker();

    /**
     * @brief When the load is freshly loaded this function initializes the memory.
     */
    void MemoryInitialization();

    /**
     * @brief Called when a MIDI message is received.
     * @param msg Pointer to the MIDI message.
     */
    void MidiCallback(midi::Message *msg);

    /**
     * @brief Returns the unique ID of the app.
     * @return The ID of the app.
     */
    uint8_t GetId()
    {
        return 0xB3;
    }

private:
    /**
     * @brief Flag indicating whether the app has been initialized.
     */
    bool inited_ = false;

    /**
     * @brief Analog input helpers used by the app.
     */
    static constexpr Hardware::AnalogInput CV_PITCH_FREE = Hardware::AnalogInput::PITCH_1;
    static constexpr Hardware::AnalogInput CV_PITCH_STEP = Hardware::AnalogInput::PITCH_2;
    static constexpr Hardware::AnalogInput CV_SAMPLE = Hardware::AnalogInput::PARAM_1;
    static constexpr Hardware::AnalogInput CV_ENVELOPE = Hardware::AnalogInput::PARAM_3;
    static constexpr Hardware::AnalogInput CV_BANK = Hardware::AnalogInput::MODE;

    /**
     * @brief Memory addresses used by the app.
     */
    static constexpr size_t kMemBank = Memory::ADDR_APP_SPACE + 0x0;
    static constexpr size_t kMemAudioRouting = Memory::ADDR_APP_SPACE + 0x1;
    static constexpr size_t kMemPotFine = Memory::ADDR_APP_SPACE + 0x2;
    static constexpr size_t kMemPotFilter = Memory::ADDR_APP_SPACE + 0x3;
    static constexpr size_t kMemPotFX = Memory::ADDR_APP_SPACE + 0x4;
    static constexpr size_t kMemPotScale = Memory::ADDR_APP_SPACE + 0x5;
    static constexpr size_t kMemPotRoot = Memory::ADDR_APP_SPACE + 0x6;
    static constexpr size_t kMemPotLengthMod = Memory::ADDR_APP_SPACE + 0x7;
    static constexpr size_t kMemPotBankMod = Memory::ADDR_APP_SPACE + 0x8;

    /**
     * @brief Pots abstraction (can be in different layers etc.)
     */
    enum class Pot
    {
        SAMPLE,
        SAMPLE_MOD,
        PITCH,
        PITCH_MOD,
        PITCH_SCALE,
        PITCH_ROOT,
        PITCH_FINE,
        PITCH_QUANTIZED,
        ENVELOPE,
        ENVELOPE_MOD,
        BANK_MOD,
        FX,
        FILTER,
        AUDIO_ROUTE,
        COUNT
    };

    /**
     * @brief MIDI takes control over the patch when available.
     */
    enum class PitchSource
    {
        PATCH,
        MIDI
    };

    /**
     * @brief There are two decks which prevents clicks when retrigerring/switching samples.
     */
    enum class PlayerDeck
    {
        A,
        B,
        COUNT
    };

    /**
     * @brief By default, the patch uses the patch pitch. When MIDI is available, it switches to MIDI pitch.
     */
    PitchSource pitch_source_ = PitchSource::PATCH;

    /**
     * @brief Updates the quantized pot value based on current scale and root settings.
     */
    void UpdateQuantizedPot();

    /**
     * @brief Bank selection controller with MIDI support and memory persistence.
     */
    FancyMode bank_select_ = FancyMode(FancyMode::Config{
        .memory_addr = kMemBank,
        .midi_cc = cc::BANK,
        .midi_attenuator_cc = cc::BANK_MOD,
        .midi_output_cc = cc::OUT_BANK});

    /**
     * @brief Volume compensation for FX processing to maintain consistent output levels.
     */
    int32_t fx_volume_compensation_ = Q15_ZERO;

    /**
     * @brief Array of smart pot controllers for all app parameters.
     */
    EnumArray<Pot, std::unique_ptr<FancyPot>> pots_;

    /**
     * @brief Previous base pitch value for change detection.
     */
    float base_pitch_prev_ = 0;

    /**
     * @brief Previous trigger base pitch value for trigger detection.
     */
    float trigger_base_pitch_prev_ = 0;

    /**
     * @brief Previous quantizer scale value for change detection.
     */
    int32_t quantizer_scale_prev_ = 0;

    /**
     * @brief Previous quantizer root value for change detection.
     */
    int32_t quantizer_root_prev_ = 0;

    /**
     * @brief Current pitch note CV value.
     */
    int32_t pitch_note_cv_ = 0;

    /**
     * @brief Previous pitch note CV value for change detection.
     */
    int32_t pitch_note_cv_prev_ = 0;

    /**
     * @brief Flag indicating whether pitch quantization is enabled.
     */
    bool quantization_enabled_ = false;

    /**
     * @brief Previous trigger detection pitch value.
     */
    int32_t trigger_detect_pitch_prev_ = 0;

    /**
     * @brief Currently selected sample bank index.
     */
    size_t sample_bank_selected_;

    /**
     * @brief Currently selected sample number within the bank.
     */
    size_t sample_num_selected_;

    /**
     * @brief Previously selected sample number for change detection.
     */
    size_t sample_num_selected_prev_;

    /**
     * @brief If the difference between the current and previous sample CV is greater than this value, the sample is considered changed.
     */
    static constexpr int32_t kSampleChangeThreshold = 100;

    /**
     * @brief Current sample CV value.
     */
    int32_t sample_cv_;

    /**
     * @brief Previous sample CV value for change detection.
     */
    int32_t prev_sample_cv_;

    /**
     * @brief Flag indicating whether to play sample when it changes.
     */
    bool sample_play_on_change_;

    /**
     * @brief Returns maximum CV value for sample selection based on hardware version.
     * @return ADC_4V for KASTLE2 (to reach last sample on AA batteries), ADC_5V for others (=CITADEL).
     */
    inline int32_t SampleCvMaxValue()
    {
        return (Kastle2::hw.GetVersion() == Hardware::Version::KASTLE2 ? ADC_4V : ADC_5V);
    }

    /**
     * @brief Sticky map value for sample modulation to reduce jitter.
     */
    int32_t sticky_sample_mod_ = INT32_MAX;

    /**
     * @brief Sticky map value for continuous sample modulation to reduce jitter.
     */
    int32_t sticky_sample_continous_mod_ = INT32_MAX;

    /**
     * @brief Previous base envelope value for change detection.
     */
    int32_t prev_base_envelope_ = 0;

    /**
     * @brief Current envelope CV value.
     */
    int32_t envelope_cv_;

    /**
     * @brief Timer for UI change indication LED flash.
     */
    size_t ui_indicate_change_time_ = 0;

    /**
     * @brief UI loop counter for performance optimization (not all stuff needs to be done each UI loop).
     */
    size_t ui_loop_counter_ = 0;

    /**
     * @brief Maximum value for UI loop counter, after that it goes back to 0.
     */
    static constexpr size_t kUiLoopCounterMax = 8;

    /**
     * @brief Delay feedback amount (0.0 to 1.0 in Q15 format).
     */
    q15_t delay_feedback_ = Q15_ZERO;

    /**
     * @brief Delay wet/dry mix (0.0 to 1.0 in Q15 format).
     */
    q15_t delay_mix_ = Q15_ZERO;

    /**
     * @brief Left channel delay stereo offset.
     */
    int32_t delay_stereo_left_ = 0;

    /**
     * @brief Right channel delay stereo offset.
     */
    int32_t delay_stereo_right_ = 0;

    /**
     * @brief Previous left channel delay value for interpolation.
     */
    int32_t prev_delay_left_val_ = 0;

    /**
     * @brief Previous right channel delay value for interpolation.
     */
    int32_t prev_delay_right_val_ = 0;

    /**
     * @brief Maximum compressor gain for FX processing.
     */
    q15_t fx_compressor_max_ = Q15_ZERO;

    /**
     * @brief Delay time in audio samples.
     */
    size_t delay_ticks_ = 0;

    /**
     * @brief ADSR envelope for sample playback.
     */
    AdsrEnv envelope_;

    /**
     * @brief ADSR envelope for UI indication feedback.
     */
    AdsrEnv envelope_indicator_;

    /**
     * @brief ADSR envelope for audio output processing.
     */
    AdsrEnv envelope_out_;

    /**
     * @brief ADSR envelope for sample fadeout transitions.
     */
    AdsrEnv fadeout_envelope_;

    /**
     * @brief Maximum value for fadeout envelope.
     */
    q15_t fadeout_envelope_max_ = 0;

    /**
     * @brief Array of sample players for seamless switching between samples.
     */
    EnumArray<PlayerDeck, SamplePlayer16bit> players_;

    /**
     * @brief Currently active player deck (A or B).
     */
    PlayerDeck active_player_ = PlayerDeck::A;

    /**
     * @brief Pitch quantizer for musical scales.
     */
    Quantizer quantizer_;

    /**
     * @brief Stereo DJ-style filter for frequency shaping.
     */
    DjFilterStereo filter_;

    /**
     * @brief Slewer for smooth filter volume compensation transitions.
     */
    Slewer filter_volume_compensation_slewer_;

    /**
     * @brief Soft clipper for playback audio processing.
     */
    SoftClipper playback_clipper_;

    /**
     * @brief Soft clipper for FX input processing.
     */
    SoftClipper fx_input_clipper_;

    /**
     * @brief General purpose soft clipper.
     */
    SoftClipper soft_clipper_;

    /**
     * @brief Dynamic delay line for left channel processing.
     */
    std::unique_ptr<AdvancedDynamicDelayLine<q15least_t>> delay_left_;

    /**
     * @brief Dynamic delay line for right channel processing.
     */
    std::unique_ptr<AdvancedDynamicDelayLine<q15least_t>> delay_right_;

    /**
     * @brief Envelope follower for FX compression.
     */
    EnvelopeFollower fx_compressor_;

    /**
     * @brief Resets the timer keeping time of LED flash length.
     */
    void inline UiIndicateChange();

    /**
     * @brief Here the second core processes the audio. It is called by the SecondCoreWorker.
     */
    FASTCODE void SecondCoreProcess(size_t index);

    /**
     * @brief How many samples are processed by the second core.
     */
    size_t second_core_processed_samples_ = 0;

    /**
     * @brief Total samples second core should process.
     */
    size_t buffer_size_ = 0;

    /**
     * @brief Buffer for the second core to use for clean passthrough (usually the input buffer).
     */
    q15_t *input_buffer_;

    /**
     * @brief Buffer for the second core to process (usually the output buffer).
     */
    q15_t *output_buffer_;

    /**
     * @brief Updates the currently selected sample bank based on CV input.
     */
    inline void UpdateSelectedBank();

    /**
     * @brief Updates the currently selected sample based on CV input.
     */
    inline void UpdateSelectedSample();

    /**
     * @brief Returns the current patched sample index (stepped/quantized).
     * @return Sample index.
     */
    inline size_t GetPatchedSample();

    /**
     * @brief Returns the current patched sample as continuous value (0-127) for MIDI.
     * @return Continuous sample value for MIDI output.
     */
    inline size_t GetContinousPatchedSample();

    /**
     * @brief Triggers sample playback.
     * @param manual True if triggered manually (using SHIFT), false otherwise.
     */
    inline void Trigger(bool manual = false);

    /**
     * @brief Checks for trigger conditions and executes trigger if needed.
     */
    inline void TriggerCheck();

    /**
     * @brief Performs the actual sample trigger with deck switching.
     * @param force True to force trigger regardless of timing constraints.
     */
    FASTCODE void ActualTrigger(bool force);

    /**
     * @brief Returns the current sample data structure for the player.
     * @return Sample data with pointer, length, and channel configuration.
     */
    inline SamplePlayer16bit::Sample GetSample() const;

    /**
     * @brief Returns the color associated with a sample bank.
     * @param bank Bank index.
     * @return RGB color value as 24-bit integer.
     */
    inline uint32_t GetColor(size_t bank) const;

    /**
     * @brief Edge detector for trigger input processing (rising edge).
     */
    EdgeDetector trigger_ = EdgeDetector(EdgeDetector::Type::RISING);

    /**
     * @brief Flag indicating whether a trigger has occurred.
     */
    bool triggered_;

    /**
     * @brief Flag indicating whether the last trigger was manual (using SHIFT).
     */
    bool trigger_was_manual_ = false;

    /**
     * @brief Spacing between triggers in microseconds.
     */
    uint64_t trigger_spacing_us_;

    /**
     * @brief Timestamp of the last trigger event.
     */
    absolute_time_t last_trigger_time_;

    /**
     * @brief Flag indicating whether the external input audio should be routed through FX.
     */
    bool input_audio_through_fx_ = false;

    /**
     * @brief Amount of compression to apply (Q15 format).
     */
    int32_t compress_amount_ = Q15_MAX;

    /**
     * @brief Sample file manager for loading and accessing wave data and other settings.
     */
    WaveBardFile samples_;

    /**
     * @brief Loads sample files and other settings from USER_DATA memory section.
     * @return True if loaded successfully, false otherwise.
     */
    bool LoadSamples();

    /**
     * @brief LFO oscillator for chorus/modulation effects.
     */
    Oscillator chorus_lfo_;

    /**
     * @brief Current value of the chorus LFO.
     */
    int32_t chorus_lfo_val_ = 0;

    /**
     * @brief Used for making sure we read this inputs before triggering the sample playback.
     */
    DirtyInputsHandler<4> dirty_inputs_handler_{
        std::array<Hardware::AnalogInput, 4>{
            Hardware::AnalogInput::PITCH_2, // NOTE
            Hardware::AnalogInput::PARAM_1, // SAMPLE MOD
            Hardware::AnalogInput::MODE,    // BANK
            Hardware::AnalogInput::PARAM_3, // LENGTH
        }};

    // MIDI constants

    /**
     * @brief MIDI notes below this value trigger sample playback (C2).
     */
    static constexpr uint32_t kMidiMinNote = 48;

    /**
     * @brief MIDI notes below this value trigger sample at original pitch (C0).
     */
    static constexpr uint32_t kMidiTriggerOriginalPitchBelow = 24;

    /**
     * @brief Number of MIDI notes used for sample selection cycling (=octave).
     */
    static constexpr uint32_t kMidiNoteControlRepeat = 12;

    /**
     * @brief Center MIDI note for pitch calculations (C4).
     */
    static constexpr uint32_t kMidiBaseNote = 72;

    /**
     * @brief MIDI pitch bend range in semitones (+/- 7 semitones).
     */
    static constexpr float kPitchBendRange = 7.0f;

    // MIDI variables

    /**
     * @brief Currently received MIDI note value.
     */
    uint32_t midi_note_ = 0;

    /**
     * @brief Current pitch bend multiplier from MIDI input.
     */
    float midi_pitch_bend_multiplier_ = 1.0f;

    /**
     * @brief MIDI note value corresponding to current patch pitch.
     */
    uint8_t patch_pitch_midi_note_ = 0;

    /**
     * @brief Previous forward length CC value for change detection.
     */
    int32_t prev_forward_length_cc_ = INT32_MAX;

    /**
     * @brief Previous backward length CC value for change detection.
     */
    int32_t prev_backward_length_cc_ = INT32_MAX;

    /**
     * @brief Flag to send MIDI note on next time.
     */
    bool next_time_send_note_on_ = false;

    /**
     * @brief MIDI note sender for automatic note on/off timing.
     */
    midi::NoteSender note_sender_;

    /**
     * @brief Sends MIDI CC messages for sample length/envelope parameters.
     * @param force If true, bypasses MIDI rate limiting and value caching to send immediately.
     */
    void SendMidiLength(bool force);

    /**
     * @brief Sends MIDI CC message for combined bank and sample selection.
     * @param force If true, bypasses MIDI rate limiting and value caching to send immediately.
     */
    void SendMidiCombo(bool force);

    /**
     * @brief Sends MIDI CC message for currently selected bank.
     * @param force If true, bypasses MIDI rate limiting and value caching to send immediately.
     */
    void SendMidiBank(bool force);

    /**
     * @brief Sends MIDI CC message for currently selected sample.
     * @param force If true, bypasses MIDI rate limiting and value caching to send immediately.
     */
    void SendMidiSample(bool force);
};
}
