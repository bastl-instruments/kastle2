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
Based on ADSR envelope made by Nigel Redmon.
http://www.earlevel.com/main/2013/06/01/envelope-generators/
*/

#include "AdsrEnv.hpp"
#include <cmath>

using namespace kastle2;

void AdsrEnv::Init(float sample_rate)
{
    Reset();
    hold_value_ = 0;
    sample_rate_ = sample_rate;
    looping_ = false;
    decay_freeze_ = false;
    attack_coef_ = 0;
    decay_coef_ = 0;
    release_coef_ = 0;
    attack_base_ = 0;
    decay_base_ = 0;
    release_base_ = 0;
    sustain_level_ = 0;
    target_ratioA_log_ = 0.0f;
    target_ratioDR_log_ = 0.0f;
    target_ratioA_ = 0.0f;
    target_ratioDR_ = 0.0f;
    non_resetting_ = NonResetting::NONE;
    SetSustainLevel(0);
    SetReleaseTime(0);
    SetTargetRatioA(kRatioAttack);
    SetTargetRatioDR(kRatioDecay);
    SetAttackTime(0.1f);
    SetDecayTime(1.0f);
    SetDecayFreezeRiseTime(0.4f);
}

void AdsrEnv::Reset()
{
    state_ = State::IDLE;
    output_ = 0;
    hold_count_ = 0;
    next_trigger_ = false;
}

void AdsrEnv::SetAttackTime(float time)
{
    float rate = time * sample_rate_;
    float attack_coef = CalcCoef(rate, target_ratioA_log_);
    float attack_base = (1.0 + target_ratioA_) * (1.0 - attack_coef);
    attack_coef_ = float_to_q31(attack_coef);
    attack_base_ = float_to_q31(attack_base);
}

void AdsrEnv::SetDecayTime(float time)
{
    float rate = time * sample_rate_;
    float decay_coef = CalcCoef(rate, target_ratioDR_log_);
    float decay_base = (sustain_level_ - target_ratioDR_) * (1.0 - decay_coef);
    decay_coef_ = float_to_q31(decay_coef);
    decay_base_ = float_to_q31(decay_base);
}

void AdsrEnv::SetReleaseTime(float time)
{
    float rate = time * sample_rate_;
    float release_coef = CalcCoef(rate, target_ratioDR_log_);
    float release_base = -target_ratioDR_ * (1.0 - release_coef);
    release_coef_ = float_to_q31(release_coef);
    release_base_ = float_to_q31(release_base);
}

float AdsrEnv::CalcCoef(float rate, float targetRatioLog)
{
    return exp(targetRatioLog / rate);
}

void AdsrEnv::SetNonResetting(NonResetting option)
{
    non_resetting_ = option;
}

inline bool AdsrEnv::IsNonResettingAttack() const
{
    return non_resetting_ == NonResetting::ATTACK || non_resetting_ == NonResetting::ATTACK_AND_DECAY;
}

inline bool AdsrEnv::IsNonResettingDecay() const
{
    return non_resetting_ == NonResetting::DECAY || non_resetting_ == NonResetting::ATTACK_AND_DECAY;
}

void AdsrEnv::SetDecayFreezeRise(bool freeze)
{
    decay_freeze_ = freeze;
}

void AdsrEnv::SetDecayFreezeRiseTime(float time)
{
    float rate = time * sample_rate_;
    float decay_freeze_coef = CalcCoef(rate, target_ratioA_log_);
    float decay_freeze_base = (1.0 + target_ratioA_) * (1.0 - decay_freeze_coef);
    decay_freeze_coef_ = float_to_q31(decay_freeze_coef);
    decay_freeze_base_ = float_to_q31(decay_freeze_base);
}

void AdsrEnv::SetSustainLevel(q31_t level)
{
    sustain_level_ = level;
}

void AdsrEnv::SetTargetRatioA(float targetRatio)
{
    if (targetRatio < 0.000000001)
        targetRatio = 0.000000001; // -180 dB
    target_ratioA_ = targetRatio;
    target_ratioA_log_ = -log((1.0 + target_ratioA_) / target_ratioA_);
}

void AdsrEnv::SetTargetRatioDR(float targetRatio)
{
    if (targetRatio < 0.000000001)
        targetRatio = 0.000000001; // -180 dB
    target_ratioDR_ = targetRatio;
    target_ratioDR_log_ = -log((1.0 + target_ratioDR_) / target_ratioDR_);
}

FASTCODE q31_t AdsrEnv::Process()
{
    if (next_trigger_)
    {
        next_trigger_ = false;

        if (!(IsNonResettingAttack() && state_ == State::ATTACK) && !(IsNonResettingDecay() && state_ == State::DECAY))
        {
            output_ = 0;
        }
        state_ = State::ATTACK;
    }

    switch (state_)
    {
    case State::HOLD:
        hold_count_++;
        if (hold_count_ >= hold_value_)
        {
            hold_count_ = 0;
            if (looping_)
                state_ = State::RELEASE;
            else
                state_ = State::DECAY;
        }
        break;
    case State::IDLE:
        if (looping_)
        {
            state_ = State::ATTACK;
        }
        break;
    case State::ATTACK:
        output_ = MultAdd(attack_base_, output_, attack_coef_);

        if (output_ == Q31_MAX)
        {
            if (hold_value_ > 0)
            {
                state_ = State::HOLD;
                hold_count_ = 0;
            }
            else if (looping_)
            {
                state_ = State::RELEASE;
            }
            else
            {
                state_ = State::DECAY;
            }
        }
        break;
    case State::DECAY:
        if (!decay_freeze_) // stop everything if decay is frozen
        {
            output_ = MultAdd(decay_base_, output_, decay_coef_);
            if (output_ <= sustain_level_)
            {
                output_ = sustain_level_;
                state_ = State::SUSTAIN;
            }
        }
        else if (output_ > q31(0.02f) && output_ < Q31_MAX) // smoothly rise back to top if envelope is still active
        {
            output_ = MultAdd(decay_freeze_base_, output_, decay_freeze_coef_);
        }
        break;
    case State::SUSTAIN:
        break;
    case State::RELEASE:
        output_ = MultAdd(release_base_, output_, release_coef_);
        if (output_ <= 0)
        {
            output_ = 0;
            if (looping_)
            {
                state_ = State::ATTACK;
            }
            else
            {
                state_ = State::IDLE;
            }
        }
    }
    return output_;
}

FASTCODE void AdsrEnv::Trigger()
{
    next_trigger_ = true;
}

AdsrEnv::State AdsrEnv::GetState() const
{
    return state_;
}

q31_t AdsrEnv::MultAdd(q31_t a, q31_t b, q31_t c)
{
    // result = a + b * c;
    int64_t result31 = ((int64_t)b * (int64_t)c) >> 31;
    result31 += a;
    if (result31 > Q31_MAX)
    {
        result31 = Q31_MAX;
    }
    else if (result31 < 0)
    {
        result31 = 0;
    }
    return (q31_t)result31;
}