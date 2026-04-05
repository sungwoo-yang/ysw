// YellowLaser.hpp
#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Laser.hpp"

class YellowLaser : public Laser
{
public:
    YellowLaser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player);
    void Update([[maybe_unused]] double dt) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Laser;
    }

    std::string TypeName() override
    {
        return "YellowLaser";
    }

    void StartExtending()
    {
        currentLength = 0.0;
    }

    void SetExtendSpeed(double speed)
    {
        extendSpeed = speed;
    }

private:
    double currentLength  = 3000.0;
    double extendSpeed    = 1000.0;
    double maxLaserLength = 3000.0;
};