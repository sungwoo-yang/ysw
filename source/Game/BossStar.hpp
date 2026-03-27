#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Star.hpp"

class BossStar : public Star
{
public:
    BossStar(Math::vec2 pos, Player* player);

    void Update(double dt) override;

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

private:
    int          attackStep;
    const double parryWindowTime = 0.5;
};