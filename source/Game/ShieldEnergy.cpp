#include "ShieldEnergy.hpp"

#include "BossConfig.hpp"

#include <algorithm>

namespace Boss
{
    void ShieldEnergy::Reset()
    {
        energy = 0.0f;
    }

    void ShieldEnergy::AddEnergy(float amount)
    {
        if (amount <= 0.0f)
        {
            return;
        }

        energy = std::clamp(energy + amount, 0.0f, Config::MaxShieldEnergy);
    }

    bool ShieldEnergy::ConsumeEnergy(float amount)
    {
        if (amount <= 0.0f)
        {
            return true;
        }

        if (!CanSpend(amount))
        {
            return false;
        }

        energy = std::clamp(energy - amount, 0.0f, Config::MaxShieldEnergy);
        return true;
    }

    void ShieldEnergy::SetEnergy(float value)
    {
        energy = std::clamp(value, 0.0f, Config::MaxShieldEnergy);
    }

    float ShieldEnergy::GetEnergy() const
    {
        return energy;
    }

    float ShieldEnergy::GetMaxEnergy() const
    {
        return Config::MaxShieldEnergy;
    }

    float ShieldEnergy::GetRatio() const
    {
        if constexpr (Config::MaxShieldEnergy <= 0.0f)
        {
            return 0.0f;
        }

        return energy / Config::MaxShieldEnergy;
    }

    bool ShieldEnergy::IsFull() const
    {
        return energy >= Config::RequiredChargeEnergy;
    }

    bool ShieldEnergy::CanSpend(float amount) const
    {
        return energy >= amount;
    }
}