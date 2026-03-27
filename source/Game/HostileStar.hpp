#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Star.hpp"

class Laser;

class HostileStar : public Star
{
public:
    enum class StarType
    {
        Yellow,
        Red
    };

    HostileStar(Math::vec2 pos, Player* player, StarType type);

    void        Update(double dt) override;
    void        OnWarningComplete() override;
    CS200::RGBA GetTelegraphColor() const override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Star;
    }

    std::string TypeName() override
    {
        return "HostileStar";
    }

    CS200::RGBA GetBodyColor() const override
    {
        return 0xFFFF00FF;
    }

    void SetStarType(StarType newType);

private:
    StarType     currentStarType;
    const double parryWindowTime = 0.2;

    Laser*     activeLaser           = nullptr;
    double     firingTimer           = 0.0;
    double     currentFiringDuration = 0.0;
    Math::vec2 laserDirection;
    double     rotationSpeed = 1.5;
};