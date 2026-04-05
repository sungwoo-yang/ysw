// RedHitParticle.hpp
#pragma once
#include "Engine/Particle.hpp"

class RedHitParticle : public CS230::Particle
{
public:
    static constexpr int    MaxCount = 100;
    static constexpr double MaxLife  = 1;

    RedHitParticle() : Particle("Assets/images/Red.spt")
    {
    }

    std::string TypeName() override
    {
        return "RedHitParticle";
    }
};