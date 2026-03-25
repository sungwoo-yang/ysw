#pragma once

#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"
#include <deque>
#include <vector>

class Player;
class TargetStar;

class YellowLaser : public CS230::GameObject
{
public:
    YellowLaser(Math::vec2 in_startPos, Math::vec2 in_direction, Player* in_player, const std::vector<TargetStar*>& in_targets);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Laser;
    }

    std::string TypeName() override
    {
        return "YellowLaser";
    }

private:
    // Smoothly rotates the laser towards the player's current position
    void UpdateDirection(double dt);

    // Recalculates bounce trajectories dynamically each frame
    void CalculateLaserPath();

    // Processes damage and target activation based on the current path
    void CheckCollisions();

    Math::vec2               startPos;
    Math::vec2               currentDir;
    Player*                  player;
    std::vector<TargetStar*> targets;
    std::vector<Math::vec2>  pathPoints;    // Stores the vertices of the reflected laser beam

    // Laser configuration parameters
    const double maxLaserLength = 2500.0;
    const double laserDuration  = 4.0;
    const double rotationSpeed  = 1.5;  // Angular velocity for tracking the player
    const double damageRadius   = 10.0; // Thickness of the hit detection

    const double detectionRange = 500.0;
    const double chaseRange     = detectionRange * 1.5;

    double      timer      = 0.0;
    CS200::RGBA laserColor = CS200::YELLOW;
};