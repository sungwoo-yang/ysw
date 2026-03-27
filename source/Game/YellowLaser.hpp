// YellowLaser.hpp
#pragma once
#include "Laser.hpp"
#include "Engine/GameObjectTypes.hpp"

class YellowLaser : public Laser
{
public:
    YellowLaser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player);
    void Update([[maybe_unused]]double dt) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Laser;
    }

    std::string TypeName() override
    {
        return "YellowLaser";
    }
};