#include "LfoMod.hpp"
#include "Kastle2.hpp"

namespace kastle2
{

void LfoMod::SetDestination(Destination destination)
{
    if (destination < Destination::DEFAULT || destination >= Destination::COUNT)
    {
        return;
    }
    destination_ = FixDestination(destination);
}

void LfoMod::SetDestination(FancyPot *pot)
{
    if (pot == nullptr)
    {
        return;
    }
    for (auto destination : EnumRange<Destination>())
    {
        if (destination == Destination::DEFAULT)
        {
            continue; // Skip the DEFAULT destination
        }
        const auto &target = kTargets[destination];
        if (target.pot == pot->GetPot() && target.layer == pot->GetLayer())
        {
            destination_ = destination;
            return;
        }
    }
}

LfoMod::Destination LfoMod::GetDestination() const
{
    return destination_;
}

bool LfoMod::IsDestination(FancyPot *pot) const
{
    if (pot == nullptr)
    {
        return 0;
    }
    const Target &dest = kTargets[FixDestination(destination_)];
    return (dest.pot == pot->GetPot() && dest.layer == pot->GetLayer());
}

const LfoMod::Target &LfoMod::GetTarget() const
{
    return kTargets[FixDestination(destination_)];
}

int32_t LfoMod::GetModValue(Destination destination) const
{
    return (destination == destination_) ? ModVal() : 0;
}

int32_t LfoMod::GetModValue(FancyPot *pot) const
{
    if (pot == nullptr)
    {
        return 0;
    }
    const Target &dest = kTargets[FixDestination(destination_)];
    return (dest.pot == pot->GetPot() && dest.layer == pot->GetLayer()) ? ModVal() : 0;
}

void LfoMod::UpdateValues(int32_t pot, int32_t mod)
{
    pot_ = pot;
    mod_ = map(mod, -Kastle2::hw.GetSafeCvMaxValue(), Kastle2::hw.GetSafeCvMaxValue(), -POT_RANGE, POT_RANGE, MapClamp::TRUE);
}

int32_t LfoMod::AdjustWithMod(FancyPot *pot, int32_t value) const
{
    return constrain(value + GetModValue(pot), POT_MIN, POT_MAX);
}

int32_t LfoMod::AdjustedPotValue(FancyPot *pot) const
{
    return AdjustWithMod(pot, pot->GetValue());
}

int32_t LfoMod::ModVal() const
{
    return apply_pot_mod_attenuvert(mod_, pot_);
}

bool LfoMod::DestinationChanged()
{
    bool changed = (destination_ != previous_destination_);
    previous_destination_ = destination_;
    return changed;
}

LfoMod::Destination LfoMod::FixDestination(Destination destination)
{
    const auto mapped_destination = (destination == Destination::DEFAULT) ? Destination::NORMAL_3 : destination;
    return mapped_destination;
}

} // namespace kastle2
