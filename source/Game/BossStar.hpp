#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include <vector>

class Player;
class TargetStar;

class BossStar : public CS230::GameObject
{
public:
    BossStar(Math::vec2 position, Player* player, const std::vector<TargetStar*>& targets);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;
    void TakeDamage(double damage);

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Boss;
    }

    std::string TypeName() override
    {
        return "BossStar";
    }

private:
    enum class State
    {
        Idle,
        Warning,
        Cooldown
    };

    enum class NextLaser
    {
        Red,
        Yellow
    };

    State     currentState;
    NextLaser nextLaserType;

    Player*                  player;
    std::vector<TargetStar*> targets;

    double timer;

    const double detectionRadius  = 800.0;
    const double warningDuration  = 1.5;
    const double cooldownDuration = 5.0;
    const double size             = 80.0;
};