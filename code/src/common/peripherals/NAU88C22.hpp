/*
MIT License

Copyright (c) 2023 Vaclav Mach (Bastl Instruments)

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
#include "common/EnumTools.hpp"

namespace kastle2
{

/**
 * @class NAU88C22
 * @ingroup peripherals
 * @brief Audio codec driver.
 * @note Don't access directly, use Kastle2::codec instead.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2023-11-28
 */
class NAU88C22
{

public:
    /// @brief Datasheet shows "0x34", but the last bit is R/W
    static const uint8_t I2C_ADDRESS = 0x1A;

    // EQ Band
    enum class EqBand
    {
        BAND_1,
        BAND_2,
        BAND_3,
        BAND_4,
        BAND_5,
        COUNT
    };

    // EQ Cutoff
    // See datasheet page 34
    enum class EqCutoff
    {
        CUTOFF_1,
        CUTOFF_2,
        CUTOFF_3,
        CUTOFF_4,
        COUNT
    };

    // EQ Width
    enum class EqWidth
    {
        NARROW,
        WIDE,
        COUNT
    };

    // EQ Path
    enum class EqPath
    {
        INPUT,
        OUTPUT,
        COUNT
    };

    enum class SpecialRegister
    {
        HP_VOLUME,
        INPUT_GAIN,
        REG_3D,
        COUNT
    };

    /**
     * @brief Initializes the codec.
     * @param i2c_inst I2C instance
     */
    bool Init(i2c_inst_t *i2c_inst);

    /**
     * @brief Writes a value to the codec's register.
     * @param addr Register address
     * @param value Value to write (usually 9 bits)
     */
    void WriteRegister(uint8_t addr, uint16_t value);

    /**
     * @brief Reads a value from the codec's register.
     * @param addr Register address
     * @return Value from the register
     */
    uint16_t ReadRegister(uint8_t addr);

    /**
     * @brief Sets the HP volume.
     * @param volume Max is 63, 0x3f (+6dB)
     */
    void SetHpVolume(uint8_t volume);

    /**
     * @brief Sets the input gain
     * @param volume Max is 63, 0x3f (+35.25dB), min is 0x00f (-12dB)
     */
    void SetInputGain(uint8_t gain);

    /**
     * @brief Sets the "3D effect". Most likely a mono mix with phase shifting. And more white noise :-)
     * @param volume Max is 15, 0xf (100%), min is 0 (0%)
     */
    void Set3dEffect(uint8_t effect);

    /**
     * @brief Sets the Equalizer.
     * @param band Band
     * @param cutoff See datasheet page 34
     * @param gain Gain (-12 to 12)
     * @param width Bandwidth (works only for bands 2-4)
     */
    void SetEqBand(EqBand band, EqCutoff cutoff, int8_t gain, EqWidth width);

    /**
     * @brief Sets where should the Equalizer work
     * @param path Input or output
     */
    void SetEqPath(EqPath path);

    /**
     * @brief Finishes writing to the special registers (HP volume, input gain, 3D effect)
     *        This is used to avoid writing to the codec too often
     */
    void Update();

    /**
     * @brief When updating volume, zero cross helps to avoid clicks. But can be problematic when volume is too low. Disabled by default.
     * @param enabled True to enable, false to disable
     */
    void SetZeroCrossUpdate(bool enabled);

    // Registers
    static constexpr uint8_t RESET = 0x00;
    static constexpr uint8_t POWER_MANAGEMENT_1 = 0x01;
    static constexpr uint8_t POWER_MANAGEMENT_2 = 0x02;
    static constexpr uint8_t POWER_MANAGEMENT_3 = 0x03;
    static constexpr uint8_t AUDIO_INTERFACE = 0x04;
    static constexpr uint8_t COMPANDING_CONTROL = 0x05;
    static constexpr uint8_t CLOCK_CONTROL_1 = 0x06;
    static constexpr uint8_t CLOCK_CONTROL_2 = 0x07;
    static constexpr uint8_t GPIO_CONTROL = 0x08;
    static constexpr uint8_t JACK_DETECT_CONTROL_1 = 0x09;
    static constexpr uint8_t DAC_CONTROL = 0x0A;
    static constexpr uint8_t LEFT_DAC_DIGITAL_VOLUME = 0x0B;
    static constexpr uint8_t RIGHT_DAC_DIGITAL_VOLUME = 0x0C;
    static constexpr uint8_t JACK_DETECT_CONTROL_2 = 0x0D;
    static constexpr uint8_t ADC_CONTROL = 0x0E;
    static constexpr uint8_t LEFT_ADC_DIGITAL_VOLUME = 0x0F;
    static constexpr uint8_t RIGHT_ADC_DIGITAL_VOLUME = 0x10;
    static constexpr uint8_t EQ1 = 0x12;
    static constexpr uint8_t EQ2 = 0x13;
    static constexpr uint8_t EQ3 = 0x14;
    static constexpr uint8_t EQ4 = 0x15;
    static constexpr uint8_t EQ5 = 0x16;
    static constexpr uint8_t DAC_LIMITER_1 = 0x18;
    static constexpr uint8_t DAC_LIMITER_2 = 0x19;
    static constexpr uint8_t NOTCH_FILTER_1 = 0x1b;
    static constexpr uint8_t NOTCH_FILTER_2 = 0x1c;
    static constexpr uint8_t NOTCH_FILTER_3 = 0x1d;
    static constexpr uint8_t NOTCH_FILTER_4 = 0x1e;
    static constexpr uint8_t ALC_CONTROL_1 = 0x20;
    static constexpr uint8_t ALC_CONTROL_2 = 0x21;
    static constexpr uint8_t ALC_CONTROL_3 = 0x22;
    static constexpr uint8_t NOISE_GATE = 0x23;
    static constexpr uint8_t PLL_N = 0x24;
    static constexpr uint8_t PLL_K1 = 0x25;
    static constexpr uint8_t PLL_K2 = 0x26;
    static constexpr uint8_t PLL_K3 = 0x27;
    static constexpr uint8_t _3D_CONTROL = 0x29;
    static constexpr uint8_t BEEP_CONTROL = 0x2b;
    static constexpr uint8_t INPUT_CONTROL = 0x2c;
    static constexpr uint8_t LEFT_INP_PGA_CONTROL = 0x2d;
    static constexpr uint8_t RIGHT_INP_PGA_CONTROL = 0x2e;
    static constexpr uint8_t LEFT_ADC_BOOST_CONTROL = 0x2f;
    static constexpr uint8_t RIGHT_ADC_BOOST_CONTROL = 0x30;
    static constexpr uint8_t OUTPUT_CONTROL = 0x31;
    static constexpr uint8_t LEFT_MIXER_CONTROL = 0x32;
    static constexpr uint8_t RIGHT_MIXER_CONTROL = 0x33;
    static constexpr uint8_t LEFT_HP_VOLUME = 0x34;
    static constexpr uint8_t RIGHT_HP_VOLUME = 0x35;
    static constexpr uint8_t LOUT2_SPK_CONTROL = 0x36;
    static constexpr uint8_t ROUT2_SPK_CONTROL = 0x37;
    static constexpr uint8_t AUX2_MIXER_CONTROL = 0x38;
    static constexpr uint8_t AUX1_MIXER_CONTROL = 0x39;
    static constexpr uint8_t POWER_MANAGEMENT_4 = 0x3A;
    static constexpr uint8_t REVISION = 0x3E;
    static constexpr uint8_t DEVICE_ID = 0x3F;
    static constexpr uint8_t DAC_DITHER = 0x41;
    static constexpr uint8_t MISC_CONTROLS = 0x49;

    // Values
    static constexpr uint8_t DEFAULT_HP_VOLUME = 0x39;  // 0 dB
    static constexpr uint8_t MAX_HP_VOLUME = 0x3F;      // +6 dB
    static constexpr uint8_t DEFAULT_INPUT_GAIN = 0x10; // 0 dB
    static constexpr uint8_t MAX_INPUT_GAIN = 0x3F;     // +32.25 dB
    static constexpr uint8_t DEFAULT_3D_EFFECT = 0;     // none
    static constexpr uint8_t MAX_3D_EFFECT = 0xF;       // 100% 3D

    static constexpr uint16_t DEVICE_ID_SHOULD_BE = 0x01A;

private:
    i2c_inst_t *i2c_inst_;

    // Invalid recieved value
    static const uint16_t INVALID = 0xFFFF;

    void SendSpecialRegister(SpecialRegister reg, uint8_t value);

    EnumArray<SpecialRegister, uint8_t> special_registers_values_;
    EnumArray<SpecialRegister, bool> special_registers_set_;

    absolute_time_t next_time_finish_special_registers_;

    // Enable zero cross when updating volume (avoid clicks, default is false)
    bool zero_cross_enabled_ = false;
};
}
