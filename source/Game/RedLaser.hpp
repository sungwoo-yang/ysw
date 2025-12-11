#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"
#include <vector>

class Player;
class TargetStar;

class RedLaser : public CS230::GameObject
{
public:
    RedLaser(Math::vec2 startPos, Math::vec2 direction, Player* player, const std::vector<TargetStar*>& targets);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Laser;
    }

    std::string TypeName() override
    {
        return "RedLaser";
    }

private:
    Math::vec2               start;
    Math::vec2               dir;
    Player*                  player;
    std::vector<TargetStar*> targets;

    struct BeamSegment
    {
        Math::vec2  p1, p2;
        CS200::RGBA color;
    };

    std::vector<BeamSegment> beams;

    bool   isCalculated = false;
    double lifeTime     = 0.2;
};