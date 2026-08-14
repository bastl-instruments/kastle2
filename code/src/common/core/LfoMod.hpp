/*
MIT License

Copyright (c) 2026 Marek Mach (Bastl Instruments)

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
#include <string>
#include "common/EnumTools.hpp"
#include "common/controls/FancyPot.hpp"
#include "common/core/Hardware.hpp"
#include "common/utils.hpp"

namespace kastle2
{

/**
 * @class LfoMod
 * @ingroup core
 * @brief A utility for easy routing/reassignment of the LFO Mod value to different parameters (pots).
 * @author Marek Mach (Bastl Instruments)
 * @date 2026-06-26
 */

class LfoMod
{
public:
    /**
     * @brief Represents the target for LFO modulation, consisting of a potentiometer and its layer, and the corresponding color.
     */
    struct Target
    {
        Hardware::Pot pot;
        Hardware::Layer layer;
        uint32_t color;
    };

    /**
     * @brief Represents the possible destinations for LFO modulation, including normal, shift, and mode layers.
     * @note The default is "0" due to the memory initialization, but it is remapped to NORMAL_3 in the code.
     */
    enum class Destination
    {
        DEFAULT = 0, ///< Default destination (remapped to NORMAL_3)
        NORMAL_1,    ///< Normal pot 1
        NORMAL_2,    ///< Normal pot 2
        NORMAL_3,    ///< LFO frequency
        NORMAL_4,    ///< Normal pot 4
        NORMAL_5,    ///< Normal pot 5
        NORMAL_6,    ///< Normal pot 6
        NORMAL_7,    ///< Normal pot 7
        SHIFT_1,     ///< Input gain
        SHIFT_2,     ///< Shift pot 2
        SHIFT_3,     ///< Rhythm
        SHIFT_4,     ///< Shift pot 4
        SHIFT_5,     ///< Output gain
        SHIFT_6,     ///< Shift pot 6
        SHIFT_7,     ///< Tempo
        MODE_1,      ///< Mode pot 1
        MODE_2,      ///< Mode pot 2
        MODE_3,      ///< Mode pot 3
        MODE_4,      ///< Mode pot 4
        MODE_5,      ///< Mode pot 5
        MODE_6,      ///< Mode pot 6
        MODE_7,      ///< Mode pot 7
        COUNT
    };

    /**
     * @brief Sets the destination target for LFO modulation by index.
     * @param destination The destination target in the targets array.
     */
    void SetDestination(Destination destination);

    /**
     * @brief Sets the destination target for LFO modulation by potentiometer and layer.
     * @param pot Pointer to the potentiometer to set as the destination.
     */
    void SetDestination(FancyPot *pot);

    /**
     * @brief Returns the current destination target for LFO modulation.
     * @return The Target structure representing the current destination target.
     */
    const Target &GetTarget() const;

    /**
     * @brief Returns the current destination for LFO modulation.
     * @return The current destination.
     */
    Destination GetDestination() const;

    /**
     * @brief Returns true if the possed pot is the current destination for LFO modulation.
     * @param pot Pointer to the potentiometer to check.
     * @return True if the pot is the current destination, false otherwise.
     */
    bool IsDestination(FancyPot *pot) const;

    /**
     * @brief Updates the potentiometer and modulation values for LFO modulation.
     * @param pot The potentiometer value (0-4095).
     * @param mod The modulation value (0-4095).
     */
    void UpdateValues(int32_t pot, int32_t mod);

    /**
     * @brief Returns the modulation value for a specific target index.
     * @param destination The destination target in the targets array.
     * @return The modulation value (-4095 to 4095) for the specified target, or 0 if not the current destination.
     */
    int32_t GetModValue(Destination destination) const;

    /**
     * @brief Returns the modulation value for a specific potentiometer and layer.
     * @param pot Pointer to the potentiometer to check.
     * @return The modulation value (-4095 to 4095) for the specified potentiometer and layer, or 0 if not the current destination.
     */
    int32_t GetModValue(FancyPot *pot) const;

    /**
     * @brief Adjusts a given value with the modulation value for a specific potentiometer.
     * @param pot Pointer to the potentiometer to check.
     * @param value The value to adjust (0-4095).
     * @return The adjusted value, constrained between POT_MIN and POT_MAX.
     */
    int32_t AdjustWithMod(FancyPot *pot, int32_t value) const;

    /**
     * @brief Returns the adjusted value for a specific potentiometer, including modulation.
     * @param pot Pointer to the potentiometer to check.
     * @return The adjusted value, constrained between POT_MIN and POT_MAX.
     */
    int32_t AdjustedPotValue(FancyPot *pot) const;

    /**
     * @brief Checks if the destination target for LFO modulation has changed.
     * @return True if the destination has changed, false otherwise.
     */
    bool DestinationChanged();

private:
    static constexpr EnumArray<Destination, LfoMod::Target> kTargets = {
        {Hardware::Pot::COUNT, Hardware::Layer::COUNT, WS2812::BLACK}, // default has to have something in it, even tho it's not used
        {Hardware::Pot::POT_1, Hardware::Layer::NORMAL, WS2812::CORAL},
        {Hardware::Pot::POT_2, Hardware::Layer::NORMAL, WS2812::YELLOW},
        {Hardware::Pot::POT_3, Hardware::Layer::NORMAL, WS2812::COLD_WHITE},
        {Hardware::Pot::POT_4, Hardware::Layer::NORMAL, WS2812::WARM_WHITE},
        {Hardware::Pot::POT_5, Hardware::Layer::NORMAL, WS2812::TEAL},
        {Hardware::Pot::POT_6, Hardware::Layer::NORMAL, WS2812::PURPLE},
        {Hardware::Pot::POT_7, Hardware::Layer::NORMAL, WS2812::BLUE},
        {Hardware::Pot::POT_1, Hardware::Layer::SHIFT, 0x00B0FF},
        {Hardware::Pot::POT_2, Hardware::Layer::SHIFT, 0x0080FF},
        {Hardware::Pot::POT_3, Hardware::Layer::SHIFT, 0x0040FF},
        {Hardware::Pot::POT_4, Hardware::Layer::SHIFT, 0x0000FF},
        {Hardware::Pot::POT_5, Hardware::Layer::SHIFT, 0x8000FF},
        {Hardware::Pot::POT_6, Hardware::Layer::SHIFT, 0xFF00FF},
        {Hardware::Pot::POT_7, Hardware::Layer::SHIFT, 0xFF00AF},
        {Hardware::Pot::POT_1, Hardware::Layer::MODE, 0xFFAF00},
        {Hardware::Pot::POT_2, Hardware::Layer::MODE, 0xFFFF00},
        {Hardware::Pot::POT_3, Hardware::Layer::MODE, 0x50FF00},
        {Hardware::Pot::POT_4, Hardware::Layer::MODE, 0x00FF00},
        {Hardware::Pot::POT_5, Hardware::Layer::MODE, 0x00FF30},
        {Hardware::Pot::POT_6, Hardware::Layer::MODE, 0x00FF60},
        {Hardware::Pot::POT_7, Hardware::Layer::MODE, 0x00FFA0}};

    Destination destination_ = Destination::DEFAULT;
    Destination previous_destination_ = Destination::DEFAULT;

    int32_t pot_ = 0;
    int32_t mod_ = 0;

    int32_t ModVal() const;
    static Destination FixDestination(Destination destination);
};

} // namespace kastle2
