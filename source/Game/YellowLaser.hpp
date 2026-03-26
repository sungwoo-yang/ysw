// YellowLaser.hpp
#pragma once
#include "Laser.hpp"

class YellowLaser : public Laser
{
public:
    YellowLaser(Math::vec2 startPos, Math::vec2 dir, Player* player, const std::vector<TargetStar*>& targets);
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
    const double maxLifeTime   = 4.0; // 4초 동안 발사
    const double rotationSpeed = 1.5;
};