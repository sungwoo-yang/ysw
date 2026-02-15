#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

class Player;
class TargetStar;

// Defines the behavior and attack pattern of the Star enemy
enum class StarType
{
    Yellow, // Fires continuous or tracking lasers
    Red     // Fires high-damage lasers that require parrying
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
    // State machine defining the attack lifecycle
    enum class State
    {
        Idle,       // Waiting for player to enter range
        Warning,    // Charging attack and telegraphing path
        Cooldown,   // Resting after attack
    };

    State  currentState;
    Player* player;
    std::vector<TargetStar*> targets;
    double timer;
    StarType starType;

    // Attack configuration constants
    const double detectionRadius  = 500.0;
    const double chaseRadius      = detectionRadius * 1.5   ;
    const double warningDuration  = 3.0;
    const double cooldownDuration = 5.0;
    const double parryWindowTime  = 0.5;

    CS200::RGBA color = 0xFF0000FF;
};