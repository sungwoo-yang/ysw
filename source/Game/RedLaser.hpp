// RedLaser.hpp
#pragma once
#include "Laser.hpp"
#include "Engine/GameObjectTypes.hpp"

class RedLaser : public Laser
{
public:
    RedLaser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player);
    void Update(double dt) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Laser;
    }

    std::string TypeName() override
    {
        return "RedLaser";
    }

private:
    bool   isCalculated   = false;
    double visualLifeTime = 0.2;
};