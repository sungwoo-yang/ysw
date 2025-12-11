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
    YellowLaser(Math::vec2 startPos, Math::vec2 direction, Player* player, const std::vector<TargetStar*>& targets);

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
    void UpdateDirection(double dt);
    void CalculateLaserPath();
    void CheckCollisions();

    Math::vec2               startPos;
    Math::vec2               currentDir;
    Player*                  player;
    std::vector<TargetStar*> targets;
    std::vector<Math::vec2>  pathPoints;

    const double maxLaserLength = 2500.0;
    const double laserDuration  = 4.0;
    const double rotationSpeed  = 1.5;
    const double damageRadius   = 10.0;

    const double detectionRange = 500.0;
    const double chaseRange     = detectionRange * 1.5;

    double      timer      = 0.0;
    CS200::RGBA laserColor = CS200::YELLOW;
};