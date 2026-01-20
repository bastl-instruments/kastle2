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

#include <cstdint>
#include "common/dsp/math/qmath.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class AdsrEnv
 * @ingroup dsp_control
 * @brief ADSR envelope generator which uses Q31 fixed point math in processing.
 * @details Needs the 32 bits, because with Q15 math the changes rounded to zero.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-04-06
 *
 * Based on ADSR envelope made by Nigel Redmon.
 * http://www.earlevel.com/main/2013/06/01/envelope-generators/
 */
class AdsrEnv
{
public:
    /**
     * @brief ADSR envelope states
     */
    enum class State
    {
        IDLE = 0,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE,
        HOLD
    };

    /**
     * @brief Non-resetting options for removing clicks and changing behavior
     */
    enum class NonResetting
    {
        NONE,            ///< Each trigger resets the envelope to zero and starts attack phase
        ATTACK,          ///< Non-resetting attack, will not reset the value to zero or restart the attack phase
        DECAY,           ///< Non-resetting decay, will just go to attack, without resetting the output to zero
        ATTACK_AND_DECAY ///< Non-resetting attack and decay
    };

    /**
     * @brief How much linear/curved is the attack
     */
    static constexpr float kRatioAttack = 0.01f;

    /**
     * @brief How much linear/curved is the decay
     */
    static constexpr float kRatioDecay = 0.005f;

    /**
     * @brief Reset the envelope to default values and idle state
     * @param sample_rate Sample rate of the audio processing
     */
    void Init(float sample_rate);

    /**
     * @brief Process the envelope generator and returns current value between 0-Q31_MAX
     * @return Current envelope value in 32 bits
     */
    FASTCODE q31_t Process(void);

    /**
     * @brief Get the current state of the envelope
     * @return Current state of the envelope
     */
    State GetState(void) const;

    /**
     * @brief Set the attack time in seconds
     * @param time Attack time in seconds
     */
    void SetAttackTime(float time);

    /**
     * @brief Set the hold time in seconds (zero by default)
     * @param time Hold time in seconds
     */
    void SetHoldTime(float time)
    {
        hold_value_ = time * sample_rate_;
    };

    /**
     * @brief Set the decay time in seconds
     * @param time Decay time in seconds
     */
    void SetDecayTime(float time);

    /**
     * @brief Set the release time in seconds
     * @param time Release time in seconds
     */
    void SetReleaseTime(float time);

    /**
     * @brief Set the sustain level in Q31 fixed point
     * @param level Sustain level between 0-Q31_MAX
     */
    void SetSustainLevel(q31_t level);

    /**
     * @brief Set the target ratio for attack curve
     * @param targetRatio Target ratio between 0.0-1.0
     */
    void SetTargetRatioA(float targetRatio);

    /**
     * @brief Set the target ratio for decay/release curve
     * @param targetRatio Target ratio between 0.0-1.0
     */
    void SetTargetRatioDR(float targetRatio);

    /**
     * @brief Set if triggering resets the output to the starting value
     * @param option Non-resetting option to use from the NonResetting enum
     */
    void SetNonResetting(NonResetting option);

    /**
     * @brief Returns true if attack is none-resetting
     * @return True if attack is none-resetting
     */
    inline bool IsNonResettingAttack() const;

    /**
     * @brief Returns true if decay is none-resetting
     * @return True if decay is none-resetting
     */
    inline bool IsNonResettingDecay() const;

    /**
     * @brief Set if the decay length is indefinite
     * @param freeze Whether the decay is indefinite
     * @details If true, the envelope will not decay to sustain level, but will rise back up to max and stay (until set to false)
     */
    void SetDecayFreezeRise(bool freeze);

    /**
     * @brief Set the decay freeze rise time in seconds
     * @param time Decay freeze rise time in seconds
     */
    void SetDecayFreezeRiseTime(float time);

    /**
     * @brief Enable or disable looping
     * @param looping Whether looping is enabled
     */
    void SetLooping(bool looping)
    {
        looping_ = looping;
    };

    /**
     * @brief Trigger the envelope generator
     */
    FASTCODE void Trigger();

    /**
     * @brief Reset the envelope to idle state and zero output
     */
    FASTCODE void Reset();
    
    /**
     * @brief Returns the current envelope value
     * @return Current envelope value between 0-Q31_MAX
     */
    q31_t GetOutput() const
    {
        return output_;
    };

    /**
     * @brief Whether the envelope is currently active
     */
    bool IsActive() const
    {
        return state_ == State::ATTACK ||
               state_ == State::DECAY ||
               state_ == State::HOLD ||
               next_trigger_;
    }

protected:
    float sample_rate_ = 0.0f;
    bool looping_ = false;
    State state_ = State::IDLE;
    long hold_count_ = 0;
    long hold_value_ = 0;

    // Curve settings
    float target_ratioA_ = 0.0f;
    float target_ratioDR_ = 0.0f;

    // Cached -log((1.0 + target_ratio) / target_ratio)
    float target_ratioA_log_ = 0.0f;
    float target_ratioDR_log_ = 0.0f;

    // Sustain settings
    q31_t sustain_level_ = 0;

    // Current envelope value
    q31_t output_ = 0;

    // Calculated data
    q31_t attack_coef_ = 0;
    q31_t decay_coef_ = 0;
    q31_t release_coef_ = 0;
    q31_t attack_base_ = 0;
    q31_t decay_base_ = 0;
    q31_t release_base_ = 0;
    q31_t decay_freeze_base_ = 0;
    q31_t decay_freeze_coef_ = 0;
    static inline float CalcCoef(float rate, float targetRatio);

    // Don't start attack from zero, removes "clicks"
    NonResetting non_resetting_ = NonResetting::NONE;

    // Trigger flag
    bool next_trigger_ = false;

    // Freeze in decay (as it currently is, or right after attack ends)
    bool decay_freeze_ = false;

    // Result = a + b * c;
    static inline q31_t MultAdd(q31_t a, q31_t b, q31_t c);
};
}
