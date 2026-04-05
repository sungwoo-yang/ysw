// RedHitParticle.hpp
#pragma once
#include "Engine/Particle.hpp"

class RedHitParticle : public CS230::Particle
{
public:
    static constexpr int    MaxCount = 100;
    static constexpr double MaxLife  = 0.5;

    RedHitParticle() : Particle("Assets/images/Hit.spt")
    {
    }

    std::string TypeName() override
    {
        return "RedHitParticle";
    }
};