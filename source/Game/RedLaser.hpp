// RedLaser.hpp
#pragma once
#include "Laser.hpp"

class RedLaser : public Laser
{
public:
    RedLaser(Math::vec2 startPos, Math::vec2 dir, Player* player, const std::vector<TargetStar*>& targets);
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
    double visualLifeTime = 0.2; // 계산 후 화면에 0.2초만 그려짐
};