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

#include <cstddef>
#include <cstdint>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"

#include "common/core/Kastle2.hpp"
#include "common/debug.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/peripherals/WS2812.hpp"
#include "common/utils.hpp"

#include "Hardware.hpp"

using namespace kastle2;

WS2812 pixels = WS2812(Hardware::PIN_LEDS, 3, pio0, 3);

EnumArray<Hardware::Button, size_t> ButtonsPins = {
    Hardware::PIN_BUTTON_SHIFT,
    Hardware::PIN_BUTTON_MODE};

EnumArray<Hardware::DigitalInput, size_t> DigitalInputsPins = {
    Hardware::PIN_TRIG_IN,
    Hardware::PIN_RESET_IN,
    Hardware::PIN_SYNC_IN,
    Hardware::PIN_AUDIO_IN_DETECT,
    Hardware::PIN_SYNC_IN_DETECT};

EnumArray<Hardware::Pot, Hardware::AnalogInput> PotToAnalogInputMap = {
    Hardware::AnalogInput::POT_1,
    Hardware::AnalogInput::POT_2,
    Hardware::AnalogInput::POT_3,
    Hardware::AnalogInput::POT_4,
    Hardware::AnalogInput::POT_5,
    Hardware::AnalogInput::POT_6,
    Hardware::AnalogInput::POT_7};

// Hardware class instance for ADC interrupt handler
static Hardware *hardware_instance = nullptr;

// ADC interrupt handler
static void adc_irq_handler()
{
    if (hardware_instance != nullptr)
    {
        hardware_instance->AdcIrqHandler();
    }
}

void Hardware::Init()
{
    // Set handler access to this instance
    hardware_instance = this;

    // Detect whether Kastle 2 or Citadel
    ReadVersion();

    // Set panel type (so the LEDs are displayed correctly)
    panel_type_ = PanelType::DEFAULT_GREY;

    // Update calibrations
    for (Calibration c : EnumRange<Calibration>())
    {
        custom_calibrations_[c] = 0; // Initialize custom calibrations to 0
    }
    UpdateCalibrationMaps();

    // Init I2C
    i2c_init(I2C_INSTANCE, 100 * 1000); // 100 kHz
    gpio_set_pulls(PIN_SDA, true, false);
    gpio_set_pulls(PIN_SCL, true, false);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_set_drive_strength(PIN_SDA, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_drive_strength(PIN_SCL, GPIO_DRIVE_STRENGTH_8MA);

    // Init I2S driver
    i2s_.Init(
        SAMPLE_RATE,
        {
            .mclk = PIN_MCLK,
            .dout = PIN_DOUT,
            .din = PIN_DIN,
            .bclk = PIN_BCLK,
        });

    // Init TX & RX pin
    gpio_init(PIN_TX);
    gpio_set_dir(PIN_TX, GPIO_OUT);

#if !DEBUG_TWO_USING_RX
    gpio_init(PIN_RX);
    gpio_set_dir(PIN_RX, GPIO_IN);
    // RP2040 has pull down by default, we need to turn it off
    gpio_set_pulls(PIN_RX, false, false);
#else
    gpio_init(PIN_RX);
    gpio_set_dir(PIN_RX, GPIO_OUT);
#endif

    // Init NeoPixels
    for (Led led : EnumRange<Led>())
    {
        pixels.SetPixelColor(static_cast<size_t>(led), 0x000000);
    }
    pixels.Show();

    // Inits ADCs
    gpio_init(PIN_MUX_A);
    gpio_set_dir(PIN_MUX_A, GPIO_OUT);
    gpio_init(PIN_MUX_B);
    gpio_set_dir(PIN_MUX_B, GPIO_OUT);
    gpio_init(PIN_MUX_C);
    gpio_set_dir(PIN_MUX_C, GPIO_OUT);
    adc_init();

    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);
    adc_gpio_init(29);
    adc_values_.fill(0);
    adc_dirty_counter_.fill(0);

    // Enable ADC FIFO and interrupts
    adc_fifo_setup(
        true,  // Write each result to the FIFO
        false, // Enable DMA data request (not used here)
        1,     // DREQ at least 1 sample in FIFO
        false, // No ERR bit
        false  // Shift 12-bit to 8-bit (we want full 12-bit)
    );

    // Enable ADC IRQ and set handler
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_irq_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);
    adc_irq_set_enabled(true);

    // Start first conversion
    StartAdcConversion();

    // Init buttons
    for (Button b : EnumRange<Button>())
    {
        size_t pin = ButtonsPins[b];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_set_pulls(pin, true, false);
    }

    buttons_just_pressed_.fill(false);
    buttons_just_released_.fill(false);
    prev_buttons_pressed_.fill(false);
    buttons_pressed_.fill(false);

    // Init LFO and ENV outs (they share the same PWM channel)
    gpio_set_function(PIN_LFO_OUT, GPIO_FUNC_PWM);
    gpio_set_function(PIN_ENV_OUT, GPIO_FUNC_PWM);
    pwm_set_clkdiv(kLfoEnvSliceNum, 1);                                  // PWM clock should now be running at max speed
    pwm_set_wrap(kLfoEnvSliceNum, kPwmResolution);                       // Set period (=resolution)
    pwm_set_chan_level(kLfoEnvSliceNum, kLfoPwmChannel, kPwmResolution); // Set output to zero (=max because of inverted)
    pwm_set_chan_level(kLfoEnvSliceNum, kEnvPwmChannel, kPwmResolution); // Set output to zero (=max because of inverted)
    pwm_set_enabled(kLfoEnvSliceNum, true);                              // Set the PWM running

    // Init CV out
    gpio_set_function(PIN_CV_OUT, GPIO_FUNC_PWM);
    pwm_set_clkdiv(kCvSliceNum, 1);                                 // PWM clock should now be running at max speed
    pwm_set_wrap(kCvSliceNum, kPwmResolution);                      // Set period (=resolution)
    pwm_set_chan_level(kCvSliceNum, kCvPwmChannel, kPwmResolution); // Set output to zero (=max because of inverted)
    pwm_set_enabled(kCvSliceNum, true);                             // Set the PWM running

    // Init Pulse LFO
    gpio_init(PIN_PULSE_OUT);
    gpio_set_dir(PIN_PULSE_OUT, GPIO_OUT);
    gpio_put(PIN_PULSE_OUT, true); // Setting it to 0 but it's inverse

    // Init other outputs
    gpio_init(PIN_GATE_OUT);
    gpio_set_dir(PIN_GATE_OUT, GPIO_OUT);
    gpio_put(PIN_GATE_OUT, true); // Setting it to 0 but it's inverse

    gpio_init(PIN_SYNC_OUT);
    gpio_set_dir(PIN_SYNC_OUT, GPIO_OUT);
    gpio_put(PIN_SYNC_OUT, true); // Setting it to 0 but it's inverse

    // Init digital Inputs
    for (DigitalInput i : EnumRange<DigitalInput>())
    {
        size_t pin = DigitalInputsPins[i];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_set_pulls(pin, pin == PIN_AUDIO_IN_DETECT ? false : true, false);
    }

    // Jack detect for audio in counter (for filtering)
    audio_in_jack_plugged_counter_ = 0;

    // Init pots
    layer_ = Layer::NORMAL;
    pot_freeze_values_.fill(kPotUndefined);
}

void Hardware::ReadVersion()
{
    gpio_init(PIN_RX);
    gpio_set_dir(PIN_RX, GPIO_IN);

    gpio_set_pulls(PIN_RX, false, true);
    version_ = Version::KASTLE2;
    for (size_t i = 0; i < 50; i++)
    {
        sleep_ms(1);
        if (gpio_get(PIN_RX))
        {
            version_ = Version::CITADEL;
            break;
        }
    }
    gpio_deinit(PIN_RX);
}

void Hardware::ShowStartupMessage(const StartupMessage message)
{
    uint32_t color = 0;
    size_t flashes = 3;

    switch (message)
    {
    case StartupMessage::EEPROM_WILL_CLEAR:
        color = 0x00FFFF;
        flashes = 2;
        break;
    case StartupMessage::EEPROM_INIT_OK:
        color = 0x00FF00;
        break;
    case StartupMessage::EEPROM_INIT_FAIL:
        color = 0x0000FF;
        flashes = 10;
        break;
    case StartupMessage::EEPROM_MISSING:
        color = 0x0000FF;
        break;
    case StartupMessage::CODEC_FAIL:
        color = 0x00AAFF;
        break;
    case StartupMessage::I2S_FAIL:
        color = 0x6600FF;
        break;
    }
    for (size_t j = 0; j < flashes; j++)
    {
        for (Led led : EnumRange<Led>())
        {
            pixels.SetPixelColor(static_cast<size_t>(led), color);
        }
        pixels.Show();
        sleep_ms(200);
        for (Led led : EnumRange<Led>())
        {
            pixels.SetPixelColor(static_cast<size_t>(led), 0x000000);
        }
        pixels.Show();
        sleep_ms(200);
    }
}

void Hardware::ReadButtons()
{
    // Input jack state can be also detected if a strong audio signal is present at patchbay (eg square wave from LFO)
    // With this hack we keep the jack plugged signal on to prevent quick toggling on square wave input via patchbay
    if (gpio_get(DigitalInputsPins[DigitalInput::AUDIO_IN_JACK_DETECT]))
    {
        audio_in_jack_plugged_counter_ = 10;
    }
    else
    {
        if (audio_in_jack_plugged_counter_ > 0)
        {
            audio_in_jack_plugged_counter_--;
        }
    }

    // Read buttons
    for (Button b : EnumRange<Button>())
    {
        prev_buttons_pressed_[b] = buttons_pressed_[b];
        buttons_pressed_[b] = !gpio_get(ButtonsPins[b]);
        buttons_just_pressed_[b] = buttons_pressed_[b] && !prev_buttons_pressed_[b];
        buttons_just_released_[b] = !buttons_pressed_[b] && prev_buttons_pressed_[b];
        if (buttons_just_pressed_[b])
        {
            buttons_pressed_time_[b] = get_absolute_time();
        }
    }
}

bool Hardware::JustPressed(const Button button) const
{
    return buttons_just_pressed_[button];
}

bool Hardware::JustReleased(const Button button) const
{
    return buttons_just_released_[button];
}

bool Hardware::Pressed(const Button button) const
{
    return buttons_pressed_[button];
}

uint32_t Hardware::PressedMillis(const Button button) const
{
    if (buttons_pressed_[button])
    {
        // If the button is pressed, return the time since it was pressed
        return absolute_time_diff_us(buttons_pressed_time_[button], get_absolute_time()) / 1000;
    }
    // If the button is not pressed, return 0
    return 0;
}

void Hardware::ClearButtonJusts()
{
    buttons_just_pressed_.fill(false);
    buttons_just_released_.fill(false);
}

int32_t Hardware::GetAnalogValue(const AnalogInput input) const
{
    return adc_values_[input];
}

void Hardware::SetAnalogOut(const AnalogOutput output, const int32_t val)
{
    switch (output)
    {
    case AnalogOutput::TRI_OUT:
        SetTriOut(val);
        break;
    case AnalogOutput::ENV_OUT:
        SetEnvOut(val);
        break;
    case AnalogOutput::CV_OUT:
        SetCvOut(val);
        break;
    }
}

void Hardware::SetTriOut(const int32_t val)
{
    // triangle LFO out (inverted, that's why kPwmResolution - val)
    pwm_set_chan_level(kLfoEnvSliceNum, kLfoPwmChannel, kPwmResolution - constrain(val, 0, kPwmResolution));
}

void Hardware::SetEnvOut(const int32_t val)
{
    // envelope out (inverted, that's why kPwmResolution - val)
    pwm_set_chan_level(kLfoEnvSliceNum, kEnvPwmChannel, kPwmResolution - constrain(val, 0, kPwmResolution));
}

void Hardware::SetCvOut(const int32_t val)
{
    // CV out (inverted, that's why kPwmResolution - val)
    pwm_set_chan_level(kCvSliceNum, kCvPwmChannel, kPwmResolution - constrain(val, 0, kPwmResolution));
}

void Hardware::SetDebugPin(const size_t pin, const bool value)
{
    switch (pin)
    {
    case 0:
#if DEBUG_ONE_USING_TX
        gpio_put(PIN_TX, value);
#endif
#if DEBUG_ONE_USING_SYNC_OUT
        gpio_put(PIN_SYNC_OUT, !value);
#endif
        break;
    case 1:
#if DEBUG_TWO_USING_RX
        gpio_put(PIN_RX, value);
#endif
        break;
    }
}

void Hardware::SelectAdcMux_(const size_t mux)
{
    gpio_put(PIN_MUX_A, mux & 0b001);
    gpio_put(PIN_MUX_B, mux & 0b010);
    gpio_put(PIN_MUX_C, mux & 0b100);

    // Set counter to discard multiple readings after changing the multiplexer
    // This is needed because the multiplexer needs some time to settle down
    // and the first readings after changing the mux are usually wrong.
    adc_discard_readings_counter_ = 16;
}

void Hardware::StartAdcConversion(const bool increment)
{
    if (increment)
    {
        current_hw_adc_ = EnumIncrement(current_hw_adc_);
        // When starting to read ADC2, select new mux so it's ready when reading ADC0
        if (current_hw_adc_ == HwAnalogInput::ADC2_PITCH1)
        {
            current_adc_mux_ = (current_adc_mux_ + 1) % 8;
            SelectAdcMux_(current_adc_mux_);

            // Needs some time to settle down
            // We solve it by `adc_discard_readings_counter_`

            // Support for tri-state feed inputs, kinda hacky but it works
            if (kTriStateFeedEnabled)
            {
                // Set the pullup for ADC0 if reading feed inputs (4, 5, 6 on the mux)
                gpio_set_pulls(26, current_adc_mux_ >= 4 && current_adc_mux_ <= 6, false);
            }
        }
        // Select the input
        adc_select_input(static_cast<size_t>(current_hw_adc_));

        // Send LEDs
        led_update_counter_++;
        if (led_update_counter_ >= kLedUpdateInterval)
        {
            led_update_counter_ = 0;
            SendLeds();
        }
    }
    hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);
}

void Hardware::AdcIrqHandler()
{
    // Read the result
    int32_t result = adc_fifo_get();

    // If we need to discard this reading because we recently changed the multiplexer
    if (adc_discard_readings_counter_ > 0)
    {
        adc_discard_readings_counter_--;
        // Start a new conversion and exit without processing this result
        StartAdcConversion(false);
        return;
    }

    // Process the result
    size_t index = 0;
    switch (current_hw_adc_)
    {
    case HwAnalogInput::ADC0_COMMON:
        index = static_cast<size_t>(AnalogInput::RESET) + current_adc_mux_;
        break;
    case HwAnalogInput::ADC1_COMMON:
        index = static_cast<size_t>(AnalogInput::POT_5) + current_adc_mux_;
        break;
    case HwAnalogInput::ADC2_PITCH1:
        index = static_cast<size_t>(AnalogInput::PITCH_1);
        break;
    case HwAnalogInput::ADC3_PITCH2:
        index = static_cast<size_t>(AnalogInput::PITCH_2);
        break;
    }

    // Initialize srand if not already done
    if (!srand_active_ && current_hw_adc_ == HwAnalogInput::ADC1_COMMON)
    {
        srand(result);
        srand_active_ = true;
    }

    result = NormalizeAdcResult((AnalogInput)index, result);
    result = AverageAdcResult((AnalogInput)index, result);

    adc_values_.at(index) = result;
    if (adc_dirty_counter_.at(index) > 0)
    {
        adc_dirty_counter_.at(index)--;
    }

#if MEASURE_ADC_CYCLE
    if (current_hw_adc_ == HwAnalogInput::ADC0_COMMON && current_adc_mux_ == 0)
    {
        SetDebugPin(0, 1);
    }
    else
    {
        SetDebugPin(0, 0);
    }
#endif

    // Start the next conversion
    StartAdcConversion();
}

void Hardware::UpdateCalibrationMaps()
{
    auto &source_calibrations = (calibration_source_ == CalibrationSource::CUSTOM) ? custom_calibrations_
                                                                                   : reference_calibrations_[version_];

    auto voltages_array = std::array<int32_t, kCalibrationMapSize>{ADC_0V, ADC_1V, ADC_2V, ADC_3V, ADC_4V, ADC_5V, ADC_6V, ADC_7V};
    static constexpr size_t known_calibrations_count = (static_cast<size_t>(Hardware::Calibration::COUNT) / 2);

    // Helper function to create calibrated array with extrapolation
    auto createCalibratedArray = [](const EnumArray<Calibration, int32_t> &calibrations, Calibration startCal)
    {
        std::array<int32_t, kCalibrationMapSize> values;
        // Copy known calibration values (0-4V)
        for (size_t i = 0; i < known_calibrations_count; i++)
        {
            values[i] = calibrations[static_cast<Calibration>(static_cast<int>(startCal) + i)];
        }

        // Use 1V and 4V points as reference for slope calculation
        static constexpr size_t v1_index = 1;
        static constexpr size_t v4_index = 4;
        float slope = static_cast<float>(values[v4_index] - values[v1_index]) / (v4_index - v1_index);

        // Extrapolate 5-7V using the slope from 1V-4V range
        for (size_t i = known_calibrations_count; i < kCalibrationMapSize; i++)
        {
            // Don't ever ask about the magic coefficient...
            int32_t magic_coeff = (kCalibrationMapSize - i) * 2;
            values[i] = values[v4_index] + static_cast<int32_t>(slope * (i - v4_index)) + magic_coeff;
        }
        return values;
    };

    // Create maps for both pitch inputs using the helper function
    calibration_maps_[CalibratedAnalogInput::PITCH1] = MapDef<int32_t, kCalibrationMapSize>(
        createCalibratedArray(source_calibrations, Calibration::PITCH1_0V),
        voltages_array);

    calibration_maps_[CalibratedAnalogInput::PITCH2] = MapDef<int32_t, kCalibrationMapSize>(
        createCalibratedArray(source_calibrations, Calibration::PITCH2_0V),
        voltages_array);
}

/**
 * Because of using multiplexers, voltage dividers etc, the ADC result isn't always perfect 0-4095.
 * Pots are usually in 17-4078 range, pitch inputs on Kastle 2 are usually in 18-4005 range.
 * This function normalizes the ADC results.
 */
int32_t Hardware::NormalizeAdcResult(const AnalogInput input, const int32_t result)
{
    int32_t output = result;

    // There are lot of "magic numbers" below - measured from the real units
    switch (input)
    {
    case AnalogInput::POT_1:
    case AnalogInput::POT_2:
    case AnalogInput::POT_3:
    case AnalogInput::POT_4:
    case AnalogInput::POT_5:
    case AnalogInput::POT_6:
    case AnalogInput::POT_7:
        // Pots are same for Citadel and Kastle 2
        output = map(output, 20, 4078, POT_MIN, POT_MAX);
        output = constrain(output, POT_MIN, POT_MAX);
        break;
    case AnalogInput::PITCH_1:
        if (calibration_source_ != CalibrationSource::NONE)
        {
            output = curve_map(output, calibration_maps_[CalibratedAnalogInput::PITCH1], MapClamp::FALSE, MapSafe::FALSE);
        }
        break;
    case AnalogInput::PITCH_2:
        if (calibration_source_ != CalibrationSource::NONE)
        {
            output = curve_map(output, calibration_maps_[CalibratedAnalogInput::PITCH2], MapClamp::FALSE, MapSafe::FALSE);
        }
        break;
    case AnalogInput::PARAM_1:
    case AnalogInput::PARAM_2:
    case AnalogInput::PARAM_3:
        switch (version_)
        {
        case Version::KASTLE2:
            output = map(output, 37, 3550, ADC_0V, ADC_5V);
            output = constrain(output, ADC_0V, ADC_5V);
            break;
        case Version::CITADEL:
            output = map(output, 2049, 192, ADC_0V, ADC_5V);
            output = constrain(output, ADC_N5V, ADC_5V);
            break;
        }
        break;
    case AnalogInput::FEED_1:
    case AnalogInput::FEED_2:
    case AnalogInput::FEED_3:
        // Identic
        output = map(output, kTriStateFeedEnabled ? 980 : 37, 3750, ADC_0V, ADC_5V);
        output = constrain(output, ADC_0V, ADC_5V);
        break;
    case AnalogInput::RESET:
    case AnalogInput::MODE:
    case AnalogInput::TRIG_IN:
        switch (version_)
        {
        case Version::KASTLE2:
            output = map(output, 38, 3370, ADC_0V, ADC_5V);
            output = constrain(output, ADC_0V, ADC_5V);
            break;
        case Version::CITADEL:
            output = map(output, 2049, 194, ADC_0V, ADC_5V);
            output = constrain(output, ADC_N5V, ADC_5V);
            break;
        }
        break;
    }

    // Clamp and constrain pitches
    // We loose some small values here, but it's better than having noise in pitch
    if (
        input == AnalogInput::PITCH_1 ||
        input == AnalogInput::PITCH_2)
    {
        switch (version_)
        {
        case Version::KASTLE2:
            // Clamp low values to 0 to prevent unwanted modulations
            if (output < 20)
            {
                output = 0;
            }
            break;
        case Version::CITADEL:
            // Kinda ugly solution but only way how to prevent noise without calibrations
            // When calibrations are valid, we can use the real values
            if (calibration_source_ == CalibrationSource::REFERENCE && output < 10)
            {
                output = 0;
            }
            break;
        }

        // Constrain the range to get known values
        switch (version_)
        {
        case Version::KASTLE2:
            output = constrain(output, ADC_0V, ADC_5V);
            break;
        case Version::CITADEL:
            output = constrain(output, ADC_N1V, ADC_8V);
            break;
        }
    }

    return output;
}

int32_t Hardware::AverageAdcResult(const AnalogInput input, const int32_t result)
{
    Pot pot;
    switch (input)
    {
    case AnalogInput::POT_1:
        pot = Pot::POT_1;
        break;
    case AnalogInput::POT_2:
        pot = Pot::POT_2;
        break;
    case AnalogInput::POT_3:
        pot = Pot::POT_3;
        break;
    case AnalogInput::POT_4:
        pot = Pot::POT_4;
        break;
    case AnalogInput::POT_5:
        pot = Pot::POT_5;
        break;
    case AnalogInput::POT_6:
        pot = Pot::POT_6;
        break;
    case AnalogInput::POT_7:
        pot = Pot::POT_7;
        break;
    default:
        return result;
    }
    pot_averages_[pot].Add(result);
    return pot_averages_[pot].GetAverage();
}

void Hardware::SetLed(const Led led, const uint32_t color)
{
    // Apply the color correction based on the specific panel type
    if (panel_type_ == PanelType::YELLOWED)
    {
        uint32_t r = WS2812::R(color);
        uint32_t g = WS2812::G(color);
        uint32_t b = WS2812::B(color);
        r = r / 2;
        g = g / 2;
        b = b * 2;
        r = constrain(r, 0, 255);
        g = constrain(g, 0, 255);
        b = constrain(b, 0, 255);
        led_buffer_[led] = WS2812::RGB(r, g, b);
        return;
    }

    led_buffer_[led] = color;
}

void Hardware::SetLed(const Led led, const uint8_t r, const uint8_t g, const uint8_t b)
{
    SetLed(led, WS2812::RGB(r, g, b));
}

void Hardware::LatchLeds()
{
    // Disable interrupts during the critical LED data transfer
    uint32_t interrupt_state = save_and_disable_interrupts();
    for (Led led : EnumRange<Led>())
    {
        pixels.SetPixelColor(static_cast<size_t>(led), led_buffer_[led]);
    }
    // Restore interrupts
    restore_interrupts(interrupt_state);
}

void Hardware::SendLeds()
{
    leds_just_updated_ = true;
    pixels.Show();
}

bool Hardware::GetDigitalIn(const DigitalInput input) const
{
    switch (input)
    {
    case DigitalInput::TRIG_IN:
        return GetTriggerIn();
    case DigitalInput::RESET_IN:
        return GetResetIn();
    case DigitalInput::SYNC_IN:
        return GetSyncIn();
    case DigitalInput::AUDIO_IN_JACK_DETECT:
        return IsAudioInJackProbablyPlugged();
    case DigitalInput::SYNC_IN_JACK_DETECT:
        return IsSyncInJackProbablyPlugged();
    default:
        return false;
    }
}

bool Hardware::GetTriggerIn() const
{
    return !gpio_get(DigitalInputsPins[DigitalInput::TRIG_IN]);
}

bool Hardware::GetResetIn() const
{
    return !gpio_get(DigitalInputsPins[DigitalInput::RESET_IN]);
}

bool Hardware::GetSyncIn() const
{
    return !gpio_get(DigitalInputsPins[DigitalInput::SYNC_IN]);
}

bool Hardware::IsAudioInJackProbablyPlugged() const
{
    return audio_in_jack_plugged_counter_ > 0;
}

bool Hardware::IsSyncInJackProbablyPlugged() const
{
    return !gpio_get(DigitalInputsPins[DigitalInput::SYNC_IN_JACK_DETECT]);
}

void Hardware::SetDigitalOut(const DigitalOutput output, const bool state)
{
    switch (output)
    {
    case DigitalOutput::GATE_OUT:
        SetGateOut(state);
        break;
    case DigitalOutput::SYNC_OUT:
        SetSyncOut(state);
        break;
    case DigitalOutput::PULSE_OUT:
        SetPulseOut(state);
        break;
    }
}

bool Hardware::GetRawButtonState(const Button button) const
{
    return !gpio_get(ButtonsPins[button]);
}

void Hardware::SetGateOut(const bool state)
{
    gpio_put(PIN_GATE_OUT, !state);
}

void Hardware::SetSyncOut(const bool state)
{
#if !DEBUG_USING_SYNC_OUT
    gpio_put(PIN_SYNC_OUT, !state);
#endif
}

void Hardware::SetPulseOut(const bool state)
{
    gpio_put(PIN_PULSE_OUT, !state);
}

void Hardware::ChangeLayer(const Layer layer)
{
    if (layer == layer_)
    {
        return;
    }
    layer_ = layer;
    FreezePots();
}

void Hardware::FreezePots()
{
    for (Pot p : EnumRange<Pot>())
    {
        pot_freeze_values_[p] = adc_values_[PotToAnalogInputMap[p]];
    }
}

bool Hardware::IsPotFrozen(const Pot pot) const
{
    return pot_freeze_values_[pot] != kPotUndefined;
}

Hardware::Layer Hardware::GetLayer() const
{
    return layer_;
}

bool Hardware::ReadPot(int32_t *value, const Pot pot, const Layer layer)
{
    if (layer != layer_)
    {
        return false;
    }

    int32_t adc_value = adc_values_[PotToAnalogInputMap[pot]];
    int32_t freeze_value = pot_freeze_values_[pot];

    // if not frozen, just update
    if (freeze_value == kPotUndefined)
    {
        value[0] = adc_value;
        return true;
    }

    // if knob moved => unfreeze
    if ((diff(freeze_value, adc_value) > kPotUnfreezeThreshold))
    {
        pot_freeze_values_[pot] = kPotUndefined;
        value[0] = adc_value;
        return true;
    }
    return false;
}

int32_t Hardware::GetRawPotValue(const Pot pot) const
{
    return adc_values_[PotToAnalogInputMap[pot]];
}

Hardware::FeedValue Hardware::GetFeedValue(const AnalogInput input) const
{
    if (input != AnalogInput::FEED_1 &&
        input != AnalogInput::FEED_2 &&
        input != AnalogInput::FEED_3)
    {
        return FeedValue::UNCONNECTED;
    }
    int32_t feed = GetAnalogValue(input);
    if (feed < Hardware::kFeedLowThreshold)
    {
        return FeedValue::LOW;
    }
    else if (feed > Hardware::kFeedHighThreshold)
    {
        return FeedValue::HIGH;
    };
    return FeedValue::UNCONNECTED;
}

void Hardware::SetCalibrationsEnabled(const bool enabled)
{
    if (!enabled)
    {
        calibration_source_ = CalibrationSource::NONE;
        return;
    }
    if (custom_calibrations_[Calibration::PITCH1_1V] == 0 &&
        custom_calibrations_[Calibration::PITCH1_4V] == 0 &&
        custom_calibrations_[Calibration::PITCH2_1V] == 0 &&
        custom_calibrations_[Calibration::PITCH2_4V] == 0)
    {
        calibration_source_ = CalibrationSource::REFERENCE;
    }
    else
    {
        calibration_source_ = CalibrationSource::CUSTOM;
    }
}

void Hardware::SetCustomCalibrations(const CalibrationsType &calibrations)
{
    calibration_source_ = CalibrationSource::CUSTOM;
    for (Calibration c : EnumRange<Calibration>())
    {
        custom_calibrations_[c] = calibrations[c];
    }
    UpdateCalibrationMaps();
}