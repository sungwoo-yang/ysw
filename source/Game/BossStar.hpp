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
    BossStar(Math::vec2 in_position, Player* in_player, const std::vector<TargetStar*>& in_targets);

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
    // Boss states
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

    static constexpr double detectionRadius  = 800.0;
    static constexpr double detectionRadiusSq = detectionRadius * detectionRadius;
    static constexpr double warningDuration  = 1.5;
    static constexpr double cooldownDuration = 5.0;
    static constexpr double size             = 80.0;
};