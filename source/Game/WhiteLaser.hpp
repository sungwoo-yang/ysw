#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Laser.hpp"

class WhiteLaser : public Laser
{
public:
    WhiteLaser(Math::vec2 in_startPos, Math::vec2 in_dir, Player* in_player);

    void Update(double dt) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Laser;
    }

    std::string TypeName() override
    {
        return "WhiteLaser";
    }

private:
    const int    maxBounces = 5;
    const double maxLength  = 3000.0;
    const double hitRadius  = 15.0;
};