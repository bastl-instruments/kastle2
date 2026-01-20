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
#include "common/peripherals/NAU88C22.hpp"

namespace kastle2
{

/**
 * @class Codec
 * @ingroup core
 * @brief Abstraction layer for the audio codec, currently using NAU88C22.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-06-04
 * @note This class itself doesn't handle I2S transfers, only codec register configuration.
 */
class Codec
{
public:
    /**
     * @brief Supported codec types
     */
    enum class Type
    {
        NAU88C22
    };

    /**
     * @brief Initializes the codec.
     * @param i2c_inst_t I2C instance
     * @return True if the initialization was successful
     */
    inline bool Init(i2c_inst_t *i2c_inst)
    {
        // Possible autodetection of codec type can be added here in the future...

        bool inited = nau88c22_.Init(i2c_inst);
        if (inited)
        {
            type_ = Type::NAU88C22;
            return true;
        }
        return false;
    }

    /**
     * @brief Sets the output volume.
     * @param val Value in range 0-63
     */
    inline void SetHpVolume(uint32_t val)
    {
        switch (type_)
        {
        case Type::NAU88C22:
            nau88c22_.SetHpVolume(val);
            break;
        }
    }

    /**
     * @brief Sets the Input gain.
     * @param val Value in range 0-63
     */
    inline void SetInputGain(uint32_t val)
    {
        switch (type_)
        {
        case Type::NAU88C22:
            nau88c22_.SetInputGain(val);
            break;
        }
    }

    /**
     * @brief Sets the hardware "3D effect" of the codec.
     * @param val Value in range 0-15
     */
    inline void Set3DEffect(uint32_t val)
    {
        switch (type_)
        {
        case Type::NAU88C22:
            nau88c22_.Set3dEffect(val);
            break;
        }
    }

    /**
     * @brief When updating volume, zero cross helps to avoid clicks. Disabled by default.
     * @param enabled True to enable, false to disable
     * @warning Be careful this one! It can be problematic when volume is too low!
     */
    inline void SetZeroCrossUpdate(bool enabled)
    {
        switch (type_)
        {
        case Type::NAU88C22:
            nau88c22_.SetZeroCrossUpdate(enabled);
            break;
        }
    }

    /**
     * @brief Finishes writing to the codec registers. This is called after all the settings are set.
     */
    inline void Update()
    {
        switch (type_)
        {
        case Type::NAU88C22:
            nau88c22_.Update();
            break;
        }
    }

private:
    NAU88C22 nau88c22_;
    Type type_ = Type::NAU88C22;
};
}
