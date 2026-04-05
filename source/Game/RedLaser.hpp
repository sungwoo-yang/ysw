// RedLaser.hpp
#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Laser.hpp"

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

    void SetParried(bool parried);

    bool IsBlockedByShield() const override
    {
        return isParried;
    }

private:
    bool isParried               = false;
    bool hasEmittedParryParticle = false;
};