// RedLaser.hpp
#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Laser.hpp"
#include <vector>

struct ParryRect
{
    Math::vec2 pos;
    Math::vec2 vel;
    double     life;
};

class RedLaser : public Laser
{
public:
    RedLaser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player);
    void Update([[maybe_unused]] double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

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

    std::vector<ParryRect> parryRects;
};