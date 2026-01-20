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

#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "common/EnumTools.hpp"
#include "common/config.hpp"
#include "common/dsp/math/RunningAverage.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/peripherals/NAU88C22.hpp"
#include "I2S.hpp"
#include "Kastle2_parameters.hpp"

namespace kastle2
{

/**
 * @class Hardware
 * @ingroup core
 * @brief User interface and inputs abstraction layer.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2023-11-28
 */
class Hardware
{
public:
    /**
     * @brief Wiring of the RP2040 chip.
     * @note Using C-style enum to avoid using static_cast everywhere.
     */
    enum Pin
    {
        PIN_TX = 0,
        PIN_RX = 1,
        PIN_PULSE_OUT = 2,
        PIN_GATE_OUT = 3,
        PIN_LFO_OUT = 4,
        PIN_ENV_OUT = 5,
        PIN_CV_OUT = 6,
        PIN_SYNC_OUT = 7,
        PIN_MCLK = 8,
        PIN_DIN = 9,
        PIN_BCLK = 10,
        PIN_LRCLK = 11,
        PIN_DOUT = 12,
        PIN_MUX_C = 13,
        PIN_MUX_B = 14,
        PIN_MUX_A = 15,
        PIN_LEDS = 16,
        PIN_BUTTON_SHIFT = 17,
        PIN_BUTTON_MODE = 18,
        PIN_TRIG_IN = 19,
        PIN_SDA = 20,
        PIN_SCL = 21,
        PIN_RESET_IN = 22,
        PIN_AUDIO_IN_DETECT = 23,
        PIN_SYNC_IN_DETECT = 24,
        PIN_SYNC_IN = 25
    };

    /**
     * @brief Real hardware analog inputs.
     */
    enum class HwAnalogInput
    {
        ADC0_COMMON, ///< Shared ADC input, mostly for patch points
        ADC1_COMMON, ///< Shared ADC input, mostly for pots
        ADC2_PITCH1, ///< Direct ADC input (faster, more precise)
        ADC3_PITCH2, ///< Direct ADC input (faster, more precise)
        COUNT
    };

    /**
     * @brief Not all analog inputs are calibrated.
     */
    enum class CalibratedAnalogInput
    {
        PITCH1, ///< Free (all firmwares)
        PITCH2, ///< Step (FX Wizard), Note (Wave Bard)...
        COUNT
    };

    /**
     * @brief Pitch inputs are directly connected to ADCs, other inputs are multiplexed.
     * @warning Don't change the order! The order matches the multiplexer wiring (that's why the order is so weird).
     */
    enum class AnalogInput
    {
        PITCH_1, ///< Free (all firmwares)
        PITCH_2, ///< Step (FX Wizard), Note (Wave Bard)...
        RESET,   ///< LFO Reset (can be read both analog and digital way)
        PARAM_3, ///< Amount (FX Wizard) / Length (Wave Bard)...
        PARAM_1, ///< Feedback (FX Wizard) / Sample (Wave Bard)...
        MODE,    ///< Mode (FX Wizard) / Bank (Wave Bard)...
        FEED_1,  ///< Gate feed (all firmwares)
        FEED_2,  ///< Pattern reset (all firmwares)
        FEED_3,  ///< CV feed (all firmwares)
        PARAM_2, ///< LFO Mod (all firmwares)
        POT_5,   ///< Time (FX Wizard) / Pitch (Wave Bard)...
        POT_1,   ///< Time Mod (FX Wizard) / Pitch Mod (Wave Bard)...
        POT_4,   ///< Amount (FX Wizard) / Length (Wave Bard)...
        POT_6,   ///< Feedback (FX Wizard) / Sample (Wave Bard)...
        TRIG_IN, ///< Trigger input (digital, but can be read as analog)
        POT_7,   ///< LFO (all firmwares)
        POT_2,   ///< Feedback Mod (FX Wizard) / Sample Mod (Wave Bard)...
        POT_3,   ///< LFO Mod (all firmwares)
        COUNT
    };

    /**
     * @brief Analog outputs using PWM and filtering.
     */
    enum class AnalogOutput
    {
        TRI_OUT, ///< LFO triangle output (controlled by the kastle2::Base)
        ENV_OUT, ///< Envelope output (usually controlled by the app)
        CV_OUT,  ///< Sequencer CV output (controlled by the kastle2::Base)
        COUNT
    };

    /**
     * @brief Buttons on the top panel.
     */
    enum class Button
    {
        SHIFT, ///< Left button
        MODE,  ///< Right button (FX Mode, Bank...)
        COUNT
    };

    /**
     * @brief Digital inputs (top panel, jack, jacks detection).
     */
    enum class DigitalInput
    {
        TRIG_IN,              ///< Trigger input (preferred to be read as digital for precise triggers)
        RESET_IN,             ///< LFO reset input (preferred to be read as digital for precise triggers)
        SYNC_IN,              ///< Sync input (jack and pinheader)
        AUDIO_IN_JACK_DETECT, ///< Audio input jack detection
        SYNC_IN_JACK_DETECT,  ///< Sync input jack detection
        COUNT
    };

    /**
     * @brief Digital outputs (top panel, jack).
     */
    enum class DigitalOutput
    {
        GATE_OUT,
        SYNC_OUT,
        PULSE_OUT,
        COUNT
    };

    /**
     * @brief Smart RGB LEDs on the top panel.
     */
    enum class Led
    {
        LED_1, ///< Top left LED
        LED_2, ///< Top right LED
        LED_3, ///< Bottom LED
        COUNT
    };

    /**
     * @brief Potentiometers. They are all multiplexed and averaged.
     */
    enum class Pot
    {
        POT_1, ///< Time Mod (FX Wizard) / Pitch Mod (Wave Bard)...
        POT_2, ///< Feedback Mod (FX Wizard) / Sample Mod (Wave Bard)...
        POT_3, ///< LFO Mod (all firmwares)
        POT_4, ///< Amount (FX Wizard) / Length (Wave Bard)...
        POT_5, ///< Time (FX Wizard) / Pitch (Wave Bard)...
        POT_6, ///< Feedback (FX Wizard) / Sample (Wave Bard)...
        POT_7, ///< LFO (all firmwares)
        COUNT
    };

    /**
     * @brief UI Layers (default is NORMAL).
     */
    enum class Layer
    {
        NORMAL,   ///< No buttons pressed, default state
        SHIFT,    ///< Shift button pressed
        MODE,     ///< Mode (Bank) button pressed
        SETTINGS, ///< When both Shift and Mode are held for a while
        COUNT
    };

    /**
     * @brief LED blinking on the startup.
     */
    enum class StartupMessage
    {
        EEPROM_INIT_OK,
        EEPROM_INIT_FAIL,
        EEPROM_MISSING,
        EEPROM_WILL_CLEAR,
        I2S_FAIL,
        CODEC_FAIL,
        COUNT
    };

    /**
     * @brief Pattern inputs value abstraction (they use RP2040's pull-up resistor).
     */
    enum class FeedValue
    {
        LOW,
        UNCONNECTED,
        HIGH
    };

    /**
     * @brief Calibration values for the pitch inputs (1V, 4V).
     */
    enum class Calibration
    {
        PITCH1_0V,
        PITCH1_1V,
        PITCH1_2V,
        PITCH1_3V,
        PITCH1_4V,
        PITCH2_0V,
        PITCH2_1V,
        PITCH2_2V,
        PITCH2_3V,
        PITCH2_4V,
        COUNT
    };

    /**
     * @brief Map of calibrations for Pitch 1 and Pitch 2 inputs.
     */
    using CalibrationsType = EnumArray<Calibration, int32_t>;

    /**
     * @brief Version of the hardware.
     */
    enum class Version
    {
        KASTLE2, ///< KASTLE2 is the original Kastle 2 hardware
        CITADEL, ///< CITADEL is the eurorack version of Kastle 2
        COUNT    ///< COUNT is used to represent all versions
    };

    /**
     * @brief Type of the PCB from which is the top panel made.
     * @note This is used to display the LED colors correctly.
     */
    enum class PanelType
    {
        DEFAULT_GREY, ///< Default panel type, Bastl Instruments original panels
        YELLOWED,     ///< Yellowed panel type, for custom off-brand panels
        COUNT
    };

    /**
     * @brief Initializes the hardware (pins, I2C, PWM, etc.).
     */
    void Init();

    /**
     * @brief Reads the buttons, updates all the "pressed" and "released" states.
     */
    void ReadButtons();

    /**
     * @brief Clears the "JustPressed" and "JustReleased" states of the buttons.
     */
    void ClearButtonJusts();

    /**
     * @brief We have two debug pins (TX and RX) that can be used for debugging.
     * @param pin 0 or 1
     * @param value Pin value
     */
    void SetDebugPin(const size_t pin, const bool value);

    /**
     * @brief Sets the color of the LED. LatchLeds() needs to be called to set the changes.
     * @param led LED to set
     * @param color RGB color
     */
    void SetLed(const Led led, const uint32_t color);

    /**
     * @brief Sets the color of the LED. LatchLeds() needs to be called to set the changes.
     * @param led LED to set
     * @param r Red component
     * @param g Green component
     * @param b Blue component
     */
    void SetLed(const Led led, const uint8_t r, const uint8_t g, const uint8_t b);

    /**
     * @brief Checks if the button was just pressed.
     * @param button Button to check
     * @return True if the button was just pressed
     */
    bool JustPressed(const Button button) const;

    /**
     * @brief Checks if the button was just released.
     * @param button Button to check
     * @return True if the button was just released
     */
    bool JustReleased(const Button button) const;

    /**
     * @brief Checks if the button is pressed.
     * @param button Button to check
     * @return True if the button is pressed
     */
    bool Pressed(const Button button) const;

    /**
     * @brief Checks if the button is pressed and how long.
     * @param button Button to check
     * @return Number of milliseconds the button is pressed, 0 if not pressed
     */
    uint32_t PressedMillis(const Button button) const;

    /**
     * @brief Reads the digital input.
     * @param input Digital input to read
     * @return True if the input is high
     */
    bool GetDigitalIn(const DigitalInput input) const;

    /**
     * @brief Reads the top panel trigger RAW input.
     * @warning USE ONLY IN AUDIO LOOP!
     * @return True if the trigger input is high
     */
    bool GetTriggerIn() const;

    /**
     * @brief Reads the top panel reset RAW input.
     * @warning USE ONLY IN AUDIO LOOP!
     * @return True if the reset is high
     */
    bool GetResetIn() const;

    /**
     * @brief Reads the top panel/rear jack sync RAW input.
     * @warning USE ONLY IN AUDIO LOOP!
     * @return True if the sync is high
     */
    bool GetSyncIn() const;

    /**
     * @brief Whether Audio In jack is plugged in. On Citadel this is true if either left or right jack plugged.
     * @note BE CAREFUL! On Kastle 2 it can product false positives if a signal is patched into patch bay.
     * @return Jack is probably plugged in.
     */
    bool IsAudioInJackProbablyPlugged() const;

    /**
     * @brief Whether Sync In jack is plugged in.
     * @note BE CAREFUL! On Kastle 2 it can product false positives if a signal is patched into patch bay.
     * @return Jack is probably plugged in.
     */
    bool IsSyncInJackProbablyPlugged() const;

    /**
     * @brief Sets the digital output.
     * @param output Digital output to set
     * @param state True for high, false for low
     */
    void SetDigitalOut(const DigitalOutput output, const bool state);

    /**
     * @brief Sets the top panel gate output.
     * @param state True for high, false for low
     */
    void SetGateOut(const bool state);

    /**
     * @brief Sets the top panel sync output.
     * @param state True for high, false for low
     */
    void SetSyncOut(const bool state);

    /**
     * @brief Sets the top panel pulse output.
     * @param state True for high, false for low
     */
    void SetPulseOut(const bool state);

    /**
     * @brief Reads the raw state of the button pin, without the abstraction layer.
     * @param button Button to read
     * @return True if the button is pressed
     */
    bool GetRawButtonState(const Button button) const;

    /**
     * @brief Changes the UI layer, freezing the pots.
     * @param layer Layer to change to
     */
    void ChangeLayer(const Layer layer);

    /**
     * @brief Gets the current UI layer.
     * @return Current layer
     */
    Layer GetLayer() const;

    /**
     * @brief Reads the value of a potentiometer into supplied `value` variable.
     *
     * Abstraction over "GetAnalogValue" for accessing pots via Shift.
     *
     * @param value Pointer to the variable where the value will be stored.
     * @param pot Potentiometer to read.
     * @param layer Layer to read from (normal or shift)
     * @return True if read successful
     */
    bool ReadPot(int32_t *value, const Pot pot, const Layer layer);

    /**
     * @brief Returns the raw value of a potentiometer (0-4095) without any layers or freezing.
     * @note Don't use this, unless you are absolutely sure! Use ReadPot() most of the time.
     * @param pot Potentiometer to read.
     * @return int32_t Raw potentiometer value (0-4095)
     */
    int32_t GetRawPotValue(const Pot pot) const;

    /**
     * @brief Returns value from patchbay (usually 0-4095 on Kastle 2 and -4095-4095 on Citadel) and pots (12-bit range 0-4095)
     * @note Just remember the hardware isn't ideal - allow tolerances.
     * @warning PITCH inputs (both) on Kastle 2 are ADC-0V to ADC_5V and on Citadel they're ADC_N1V to ADC_8V
     * @param input Analog input to read
     * @return int32_t Normalized value
     */
    int32_t GetAnalogValue(const AnalogInput input) const;

    /**
     * @brief Wrapper around Feed inputs (they are pulled up so we can get a tri-state). Labeled "GRC on Kastle 2".
     * @param input Feed input to read (1, 2, 3)
     * @note Gate = FEED_1, Pattern Reset = FEED_2, CV = FEED_3
     * @return FeedValue (low, unconnected, high)
     */
    FeedValue GetFeedValue(const AnalogInput input) const;

    /**
     * @brief Sets the voltage of an analog output
     * @param val 10-bit value (range 0-1023)
     */
    void SetAnalogOut(const AnalogOutput output, const int32_t val);

    /**
     * @brief Sets the voltage of the LFO triangle output
     * @param val 10-bit value (range 0-1023)
     */
    void SetTriOut(const int32_t val);

    /**
     * @brief Sets the voltage of the ENV output
     * @param val 10-bit value (range 0-1023)
     */
    void SetEnvOut(const int32_t val);

    /**
     * @brief Sets the voltage of the CV output (usually ENV OUT)
     * @param val 10-bit value (range 0-1023)
     */
    void SetCvOut(const int32_t val);

    /**
     * @brief Freezes the current pot values, so they need to be wiggled to receive new values
     */
    void FreezePots();

    /**
     * @brief Checks if the pot is frozen
     * @param pot Pot to check
     * @return True if frozen
     */
    bool IsPotFrozen(const Pot pot) const;

    /**
     * @brief Blink LEDs according to the message.
     */
    void ShowStartupMessage(const StartupMessage message);

    /**
     * @brief Enables the calibrations (you need to set them first)
     * @param enabled True to enable
     */
    void SetCalibrationsEnabled(const bool enabled);

    /**
     * @brief Sets the calibration values
     * @param calibrations Array of 4 calibration values
     */
    void SetCustomCalibrations(const CalibrationsType &calibrations);

    /**
     * @brief Gets the version we are running on (Kastle 2 desktop unit or Citadel eurorack module)
     * @return Kastle 2 or Citadel
     */
    Hardware::Version GetVersion() const
    {
        return version_;
    }

    /**
     * @brief Flags the ADC input as dirty, so it will be read again.
     * @note The dirty counter is set to `kAdcDirtyCounterMax`, so we are sure to get the new value.
     *       Some sequencers put out CV later so we need to compensate that and sadly play our sample later.
     * @param input Analog input to flag
     */
    void SetAnalogInputDirty(const AnalogInput input)
    {
        adc_dirty_counter_[input] = kAdcDirtyCounterMax;
    }

    /**
     * @brief Checks if the ADC input is dirty or has been stored succesfully (for S&H especially).
     * @param input Analog input to check
     * @return True if dirty and the output shouldn't be used
     */
    bool GetAnalogInputDirty(const AnalogInput input) const
    {
        return adc_dirty_counter_[input] > 0;
    }

    /**
     * @brief Wait time between UI readings
     */
    static constexpr size_t kUiRefreshWaitMs = 10;

    /**
     * @brief Pot not ready yet.
     */
    static constexpr int32_t kPotUndefined = UINT32_MAX;

    /**
     * @brief Instance of I2C interface we are using.
     */
    static constexpr i2c_inst_t *I2C_INSTANCE = i2c0;

    /**
     * @brief Handler for ADC interrupts - called from the global interrupt handler
     */
    void AdcIrqHandler();
    void StartAdcConversion(const bool increment = true);

    /**
     * @brief Whether the LEDs were just updated. Useful for interefence handling.
     * @note Can be called only once every time we want to get the result - the flag is cleared after the call.
     * @return True if just updated, next update in few ms.
     */
    bool HasLedsJustUpdated()
    {
        if (leds_just_updated_)
        {
            leds_just_updated_ = false;
            return true;
        }
        return false;
    }

    /**
     * @brief Latches the LEDs to the current buffer.
     * @note This is called automatically in the UI loop, most-likely you don't need to call it manually.
     */
    void LatchLeds();

    /**
     * @brief Returns reference calibrations for the current version the FW is running.
     */
    inline CalibrationsType GetReferenceCalibrations() const
    {
        return reference_calibrations_[version_];
    }

    /**
     * @brief Returns custom calibrations if they are set or reference calibrations otherwise.
     * @return EnumArray of calibrations
     */
    inline CalibrationsType GetCalibrations() const
    {
        if (calibration_source_ == CalibrationSource::CUSTOM)
        {
            return custom_calibrations_;
        }
        return reference_calibrations_[version_];
    }

    /**
     * @brief Returns the I2S driver instance.
     */
    I2S &GetI2S()
    {
        return i2s_;
    }

    /**
     * @brief Override the detected hardware.
     * @note Use after Kastle2::Init();
     */
    void ForceVersion(const Version version)
    {
        version_ = version;
        UpdateCalibrationMaps();
    }

private:
    // I2S driver
    I2S i2s_;

    // ADC stuff
    bool srand_active_ = false;
    HwAnalogInput current_hw_adc_ = Hardware::HwAnalogInput::ADC0_COMMON;
    size_t current_adc_mux_ = 0;
    EnumArray<AnalogInput, int32_t> adc_values_;
    void SelectAdcMux_(const size_t mux);
    EnumArray<AnalogInput, size_t> adc_dirty_counter_;
    static constexpr size_t kAdcDirtyCounterMax = 2; // Leave at least at 2!!!
    size_t adc_discard_readings_counter_ = 0;

    // DAC/ADC settings
    // static constexpr int32_t kAnalogInputMax = ADC_MAX;
    static constexpr int32_t kPwmResolution = DAC_MAX;

    // Pots stuff
    static constexpr int32_t kPotUnfreezeThreshold = POT_MOVE_THRESHOLD;

    // PWM stuff
    static constexpr uint32_t kLfoEnvSliceNum = 2;
    static constexpr uint32_t kLfoPwmChannel = PWM_CHAN_A;
    static constexpr uint32_t kEnvPwmChannel = PWM_CHAN_B;
    static constexpr uint32_t kCvSliceNum = 3;
    static constexpr uint32_t kCvPwmChannel = PWM_CHAN_A;

    // FEED stuff
    static constexpr bool kTriStateFeedEnabled = true;  // default is true, but can be disabled to get regular ADC inputs
    static constexpr int32_t kFeedLowThreshold = 1000;  // minus (low)
    static constexpr int32_t kFeedHighThreshold = 2200; // plus (high)
    static constexpr int32_t kFeedCenterApprox = 1620;  // center (unconnected)

    // LEDs
    EnumArray<Led, uint32_t> led_buffer_ = {0x000000, 0x000000, 0x000000};
    bool leds_just_updated_ = false;                  // For faking quick LED blinking - preventing interferences
    size_t led_update_counter_ = 0;                   // Counts how many ADC readings we have done since the last LED update
    static constexpr size_t kLedUpdateInterval = 512; // Updated every 256 ADC readings

    /**
     * @brief Sends the LEDs. It's called automatically in the ADC loop. You don't need to call it manually.
     */
    void SendLeds();

    /**
     * @brief Because of using multiplexers, zener diodes, voltage dividers etc, the ADC result isn't always perfect 0-4095.
     *
     * Pots are usually in 17-4078 range, inputs are all over the place.
     * Pitch inputs are calibrated using 1V and 4V values.
     * This function normalizes them into nice ranges.
     * Pots: 0-4095
     * Pitch inputs: 0-4095 (Kastle2), -164 to 6552 (Citadel)
     * Other ADCs: 0-4095 (Kastle2), -4095 to 4095 (Citadel)
     *
     * @param input Analog input to read
     * @param value 12-bit ADC result
     * @return Normalized value (ADCs -4095 to 4095, POTs into 0 to 4095 range)
     */
    int32_t NormalizeAdcResult(const AnalogInput input, const int32_t result);

    /**
     * @brief Averages pot values, passes through the CV values
     * @param input Analog input to read
     * @param result 12-bit-ish ADC result
     * @return Averaged value
     */
    int32_t AverageAdcResult(const AnalogInput input, const int32_t result);

    void UpdateCalibrationMaps();

    // Pots (ADC abstraction)
    Layer layer_ = Layer::NORMAL;
    EnumArray<Pot, int32_t> pot_freeze_values_;

    EnumArray<Pot, RunningAverage<int32_t>> pot_averages_ = {
        RunningAverage<int32_t>(kBasePotsRunningAverage),
        RunningAverage<int32_t>(kBasePotsRunningAverage),
        RunningAverage<int32_t>(kBasePotsRunningAverage),
        RunningAverage<int32_t>(kBasePotsRunningAverage),
        RunningAverage<int32_t>(kBasePotsRunningAverage),
        RunningAverage<int32_t>(kBasePotsRunningAverage),
        RunningAverage<int32_t>(kBasePotsRunningAverage)};

    // Buttons (Fancy abstraction!)
    EnumArray<Button, bool> buttons_pressed_;
    EnumArray<Button, bool> prev_buttons_pressed_;
    EnumArray<Button, bool> buttons_just_pressed_;
    EnumArray<Button, bool> buttons_just_released_;
    EnumArray<Button, absolute_time_t> buttons_pressed_time_;

    // Calibrations
    enum class CalibrationSource
    {
        NONE,
        REFERENCE,
        CUSTOM
    };
    CalibrationSource calibration_source_ = CalibrationSource::REFERENCE;
    CalibrationsType custom_calibrations_;
    EnumArray<Version, CalibrationsType> reference_calibrations_ = {
        {
            // Measured values from multiple real Kastle 2s
            0,    // PITCH1_0V
            825,  // PITCH1_1V
            1640, // PITCH1_2V
            2460, // PITCH1_3V
            3265, // PITCH1_4V
            0,    // PITCH2_0V
            825,  // PITCH2_1V
            1640, // PITCH2_2V
            2460, // PITCH2_3V
            3265  // PITCH2_4V
        },
        {
            // Measured values from multiple real Citadels
            3866, // PITCH1_0V
            3329, // PITCH1_1V
            2782, // PITCH1_2V
            2243, // PITCH1_3V
            1694, // PITCH1_4V
            3866, // PITCH2_0V
            3329, // PITCH2_1V
            2782, // PITCH2_2V
            2243, // PITCH2_3V
            1694  // PITCH2_4V
        }};

    // static constexpr size_t kCalibrationMapSize = static_cast<size_t>(Hardware::Calibration::COUNT) / 2;
    static constexpr size_t kCalibrationMapSize = 8; // ADC_0V to ADC_7V
    EnumArray<CalibratedAnalogInput, MapDef<int32_t, kCalibrationMapSize>> calibration_maps_;

    // Detecting audio in jack mechanism
    size_t audio_in_jack_plugged_counter_ = 0;

    // What hardware we are running on
    void ReadVersion();
    Hardware::Version version_;

    // Panel type is used to display the LEDs correctly
    // All original panels from Bastl should be DEFAULT_GREY
    PanelType panel_type_;
};
}
