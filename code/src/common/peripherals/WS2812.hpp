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

/*
 * This implementation incorporates code derived from:
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Original C++ library by ForsakenNGS: https://github.com/ForsakenNGS/Pico_WS2812
 */

#pragma once

#include <memory>
#include "pico/types.h"
#include "hardware/pio.h"

namespace kastle2
{

/**
 * @class WS2812
 * @ingroup peripherals
 * @brief Smart LED driver for WS2812, WS2812B etc.
 * @note Don't access directly, use Kastle2::hw instead.
 * @author Vaclav Mach (Bastl Instruments) based on ForsakenNGS C++ implementation
 * @date 2024-06-05
 */
class WS2812
{
public:
    /**
     * @brief Order of the data bytes in the color data.
     */
    enum class DataByte
    {
        NONE = 0,
        RED = 1,
        GREEN = 2,
        BLUE = 3
    };

    /**
     * @brief Predefined order of the data bytes in the color data.
     */
    enum class DataFormat
    {
        RGB = 0,
        GRB = 1
    };

    /**
     * @brief Constructor for the WS2812 class.
     * @param pin GPIO pin number.
     * @param length Number of LEDs in the chain.
     * @param pio PIO instance.
     * @param sm PIO state machine number.
     */
    WS2812(size_t pin, size_t length, PIO pio, size_t sm);

    /**
     * @brief Constructor for the WS2812 class.
     * @param pin GPIO pin number.
     * @param length Number of LEDs in the chain.
     * @param pio PIO instance.
     * @param sm PIO state machine number.
     * @param format Color data format.
     */
    WS2812(size_t pin, size_t length, PIO pio, size_t sm, DataFormat format);

    /**
     * @brief Constructor for the WS2812 class.
     * @param pin GPIO pin number.
     * @param length Number of LEDs in the chain.
     * @param pio PIO instance.
     * @param sm PIO state machine number.
     * @param b1 Color which should be data byte 1.
     * @param b2 Color which should be data byte 2.
     * @param b3 Color which should be data byte 3.
     */
    WS2812(size_t pin, size_t length, PIO pio, size_t sm, DataByte b1, DataByte b2, DataByte b3);

    /**
     * @brief Packs the RGB color data into a single 32-bit integer.
     * @param red Red color component.
     * @param green Green color component.
     * @param blue Blue color component.
     * @return Packed 32-bit color data.
     */
    static uint32_t RGB(uint8_t red, uint8_t green, uint8_t blue)
    {
        return (uint32_t)(red) << 16 | (uint32_t)(green) << 8 | (uint32_t)(blue);
    };

    /**
     * @brief Unpacks the color data from a single 32-bit integer.
     * @param color Packed 32-bit color data.
     * @return Unpacked red color component.
     */
    static uint32_t R(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    };

    /**
     * @brief Unpacks the color data from a single 32-bit integer.
     * @param color Packed 32-bit color data.
     * @return Unpacked green color component.
     */
    static uint32_t G(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    };

    /**
     * @brief Unpacks the color data from a single 32-bit integer.
     * @param color Packed 32-bit color data.
     * @return Unpacked blue color component.
     */
    static uint32_t B(uint32_t color)
    {
        return color & 0xFF;
    };

    /**
     * @brief Sets the color of a single LED in the chain.
     * @param index Index of the LED in the chain.
     * @param color Color to set. Be careful, it's LSB FIRST, so red is #0000FF
     */
    void SetPixelColor(size_t index, uint32_t color);

    /**
     * @brief Sets the color of a single LED in the chain.
     * @param index Index of the LED in the chain.
     * @param red Red color component.
     * @param green Green color component.
     * @param blue Blue color component.
     */
    void SetPixelColor(size_t index, uint8_t red, uint8_t green, uint8_t blue);

    /**
     * @brief Fills the whole LED chain with a single color.
     * @param color Color to fill the chain with.
     */
    void Fill(uint32_t color);

    /**
     * @brief Fills the LED chain with a single color.
     * @param color Color to fill the chain with.
     * @param first Index of the first LED to fill.
     */
    void Fill(uint32_t color, size_t first);

    /**
     * @brief Fills the LED chain with a single color.
     * @param color Color to fill the chain with.
     * @param first Index of the first LED to fill.
     * @param count Number of LEDs to fill.
     */
    void Fill(uint32_t color, size_t first, size_t count);

    /**
     * @brief Sends the data to the LEDs. You need to call this after setting the pixel colors.
     */
    void Show();

    /**
     * @brief Applies the brightness to the color.
     * @param color Color (eg. 0xFF00FF) to apply the brightness to.
     * @param brightness Brightness level (0-255).
     * @return The color with applied brightness.
     */
    static uint32_t ApplyBrightness(uint32_t color, uint8_t brightness);

    /**
     * @brief Mixes two colors (saturating the result)
     * @param color1 The first color
     * @param color2 The second color
     */
    static uint32_t MixColors(uint32_t color1, uint32_t color2);

    /**
     * @brief Mixes two colors with a given ratio
     * @param color1 The first color
     * @param color2 The second color
     * @param ratio The fraction to mix the colors, between 0 and 255.
     * @return The mixed color
     */
    static uint32_t CrossfadeColors(uint32_t color1, uint32_t color2, uint8_t ratio);

    /**
     * @brief Commonly used colors
     */
    static constexpr uint32_t RED = 0xFF0000;
    static constexpr uint32_t GREEN = 0x00FF00;
    static constexpr uint32_t BLUE = 0x0000FF;
    static constexpr uint32_t WHITE = 0x558899;
    static constexpr uint32_t BLACK = 0x000000;
    static constexpr uint32_t NONE = 0x000000;
    static constexpr uint32_t YELLOW = 0x66AA11;
    static constexpr uint32_t CYAN = 0x00FFFF;
    static constexpr uint32_t MAGENTA = 0xFF00FF;
    static constexpr uint32_t ORANGE = 0xFFA500;
    static constexpr uint32_t PINK = 0xFF50C0;
    static constexpr uint32_t PURPLE = 0x800080;
    static constexpr uint32_t BROWN = 0xA52A2A;
    static constexpr uint32_t GRAY = 0x808080;
    static constexpr uint32_t LIGHT_RED = 0xFF8888;
    static constexpr uint32_t LIGHT_GREEN = 0x55FF55;
    static constexpr uint32_t LIGHT_BLUE = 0x6666FF;
    static constexpr uint32_t DARK_RED = 0x880000;
    static constexpr uint32_t DARK_GREEN = 0x008800;
    static constexpr uint32_t DARK_BLUE = 0x000088;
    static constexpr uint32_t NAVY = 0x000080;
    static constexpr uint32_t FUCHSIA = 0xFF00FF;
    static constexpr uint32_t AQUAMARINE = 0x7FFFD4;
    static constexpr uint32_t CHARTREUSE = 0x7FFF00;
    static constexpr uint32_t CORAL = 0xFF7F50;
    static constexpr uint32_t CRIMSON = 0xDC143C;
    static constexpr uint32_t INDIGO = 0x4B0082;
    static constexpr uint32_t KHAKI = 0x254000;
    static constexpr uint32_t LAVENDER = 0xE6E6FA;
    static constexpr uint32_t LIME = 0x00FF00;
    static constexpr uint32_t MAROON = 0x800000;
    static constexpr uint32_t OLIVE = 0x808000;
    static constexpr uint32_t PEACH = 0xA5BBFF;
    static constexpr uint32_t TEAL = 0x008080;
    static constexpr uint32_t TURQUOISE = 0x40E0D0;
    static constexpr uint32_t VIOLET = 0xEE42EE;
    static constexpr uint32_t RASPBERRY = 0x770022;
    static constexpr uint32_t LIGHT_PINK = 0xDD55CC;
    static constexpr uint32_t MEDIUM_CYAN = 0x007777;
    static constexpr uint32_t MEDIUM_MAGENTA = 0x770077;
    static constexpr uint32_t WARM_WHITE = 0x7f7f4f;
    static constexpr uint32_t COLD_WHITE = 0x4f6f7f;

private:
    size_t pin_;
    size_t length_;
    PIO pio_;
    size_t sm_;
    DataByte bytes_[4];
    std::unique_ptr<uint32_t[]> data_;

    void Initialize(size_t pin, size_t length, PIO pio, size_t sm, DataByte b1, DataByte b2, DataByte b3);
    uint32_t ConvertData(uint32_t rgb);

    static uint8_t GetColorByte(uint32_t color, uint32_t index);
    static uint8_t SaturateByte(uint32_t color);
};
}
