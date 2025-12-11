#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

class Player;
class TargetStar;

enum class StarType
{
    Yellow,
    Red
};

class Star : public CS230::GameObject
{
public:
    Star(Math::vec2 position, Player* targetPlayer, const std::vector<TargetStar*>& destStars, StarType type);
    void Update(double dt) override;
    void Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix) override;
    void DrawImGui() override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Star;
    }

    std::string TypeName() override
    {
        return "Star";
    }

private:
    enum class State
    {
        Idle,
        Warning,
        Cooldown,
    };

    Player*                  player;
    std::vector<TargetStar*> targets;

    State  currentState;
    double timer;

    StarType starType;

    const double detectionRadius  = 500.0;
    const double chaseRadius      = detectionRadius * 1.5   ;
    const double warningDuration  = 3.0;
    const double cooldownDuration = 5.0;
    const double parryWindowTime  = 0.5;

    CS200::RGBA color = 0xFF0000FF;
};