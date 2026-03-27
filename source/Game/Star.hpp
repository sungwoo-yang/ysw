#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include <vector>

class Player;
class TargetStar;

class Star : public CS230::GameObject
{
public:
    Star(Math::vec2 pos, Player* player);
    virtual ~Star() = default;

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    virtual void        OnWarningComplete()       = 0;
    virtual CS200::RGBA GetTelegraphColor() const = 0;
protected:
    enum class State
    {
        Idle,
        Warning,
        Cooldown
    };
    State  currentState;
    double timer;

    Player*                  player;

    double detectionRadius  = 600.0;
    double chaseRadius      = 900.0;
    double warningDuration  = 2.0;
    double cooldownDuration = 3.0;

    void HandleBasicAI(double dt);
    void DrawTrajectory();
};