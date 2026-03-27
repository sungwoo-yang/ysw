#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Star.hpp"

class HostileStar : public Star
{
public:
    enum class StarType
    {
        Yellow,
        Red
    };

    HostileStar(Math::vec2 pos, Player* player, StarType type);

    void Update(double dt) override;

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

    void SetStarType(StarType newType);

private:
    StarType     currentStarType;
    const double parryWindowTime = 0.5;
};