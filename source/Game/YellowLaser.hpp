// YellowLaser.hpp
#pragma once
#include "Laser.hpp"
#include "Engine/GameObjectTypes.hpp"

class YellowLaser : public Laser
{
public:
    YellowLaser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player);
    void Update(double dt) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Laser;
    }

    std::string TypeName() override
    {
        return "YellowLaser";
    }

private:
    double       lifeTime      = 0.0;
    const double maxLifeTime   = 5.0;
    const double rotationSpeed = 1.5;
};