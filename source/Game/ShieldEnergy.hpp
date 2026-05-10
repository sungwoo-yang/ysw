#pragma once

namespace Boss
{
    class ShieldEnergy
    {
    public:
        ShieldEnergy() = default;

        void Reset();

        void AddEnergy(float amount);
        bool ConsumeEnergy(float amount);

        void SetEnergy(float value);

        float GetEnergy() const;
        float GetMaxEnergy() const;
        float GetRatio() const;

        bool IsFull() const;
        bool CanSpend(float amount) const;

    private:
        float energy = 0.0f;
    };
}