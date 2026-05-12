#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Star.hpp"

class Laser;

class BossStar : public Star
{
public:
    BossStar(Math::vec2 pos, Player* player);

    void        Update(double dt) override;
    void        OnWarningComplete() override;
    CS200::RGBA GetTelegraphColor() const override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Boss;
    }

    std::string TypeName() override
    {
        return "BossStar";
    }

    CS200::RGBA GetBodyColor() const override
    {
        return 0xFF00FFFF;
    }

    int GetMaxBounces() const override;

private:
    int          attackStep;
    const double parryWindowTime = 0.5;

    Laser*     activeLaser           = nullptr;
    double     firingTimer           = 0.0;
    double     currentFiringDuration = 0.0;
    Math::vec2 laserDirection;
    double     rotationSpeed = 0.5;
};