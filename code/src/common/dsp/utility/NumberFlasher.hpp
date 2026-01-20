/*
MIT License

Copyright (c) 2025 Vaclav Mach (Bastl Instruments)

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

namespace kastle2
{

/**
 * @class NumberFlasher
 * @ingroup dsp_utility
 * @brief Helper for flashing a number on an LED.
 * @date 2025-06-04
 * @author Vaclav Mach (Bastl Instruments)
 *
 * This class handles the timing and state of the LED flashing.
 * Call FlashNumber() with a number to start the flashing animation.
 * Use GetLedState() to get the current state of the LED.
 */
class NumberFlasher
{
public:
    /**
     * @brief Structure to hold the timings for the LED flashing. It's in "ticks" (1 tick approx 1 ms).
     */
    struct Timings
    {
        uint32_t start = 500u;           ///< Blanks space before the first flash
        uint32_t per_flash = 60u;        ///< Ticks for each flash
        uint32_t between_flashes = 250u; ///< Ticks between flashes
        uint32_t end = 1000u;            ///< Blank space after the last flash
    };

    /**
     * @brief Clears the flasher state.
     */
    void Init()
    {
        animation_frame_ = 0;
        animation_length_ = 0;
        countdown_ = 0;
        state_ = false;
    }

    /**
     * @brief Starts the flashing animation for a given number.
     * @param number Must be greater than 0. Each digit will be flashed.
     *
     * The number is expected to be a positive integer greater than 0.
     */
    void FlashNumber(uint32_t number)
    {
        // No flashing for zero
        if (number == 0)
        {
            return;
        }
        animation_frame_ = 0;
        countdown_ = timings_.start;
        animation_length_ = number * 2; // START x|x|x END
    }

    /**
     * @brief Sets the timings for the LED flashing.
     * @param timings The timings to use for the flashing animation.
     */
    void SetTimings(Timings timings)
    {
        timings_ = timings;
    }

    /**
     * @brief Processes the animation state.
     *
     * This function should be called periodically to update the LED state.
     * It handles the countdown and updates the LED state based on the current frame.
     */
    void Process()
    {
        // No animation to process
        if (animation_length_ == 0)
        {
            return;
        }

        // If we are in the middle of a countdown, just decrement it
        if (countdown_ > 0)
        {
            countdown_--;
            return;
        }

        // If we are here, we need to process the next frame of the animation
        if (animation_frame_ < animation_length_)
        {

            if (animation_frame_ % 2 == 0) // Flash on
            {
                state_ = true;
                countdown_ = timings_.per_flash;
            }
            else
            {
                state_ = false;
                countdown_ = timings_.between_flashes;
            }
            animation_frame_++;

            if (animation_frame_ == animation_length_)
            {
                // Last frame, we need to end the animation
                state_ = false;
                countdown_ = timings_.end;
            }
        }
        else
        {
            // Reset the flasher
            animation_length_ = 0;
            state_ = false;
        }
    }

    /**
     * @brief Gets the current state of the LED.
     * @return True if the LED should be on, false otherwise.
     */
    bool GetLedState() const
    {
        // No animation to process
        if (animation_length_ == 0)
        {
            return false;
        }
        return state_;
    }

    /**
     * @brief Checks if the animation is active.
     * @return True if the animation is active, false otherwise.
     */
    bool IsActive() const
    {
        return animation_length_ > 0;
    }

private:
    uint32_t animation_frame_ = 0;
    uint32_t animation_length_ = 0;
    uint32_t countdown_ = 0;
    Timings timings_;
    bool state_ = false;
};

}
