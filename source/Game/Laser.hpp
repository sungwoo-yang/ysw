#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/Physics/Reflection.hpp"
#include <vector>

class Player;
class TargetStar;

class Laser : public CS230::GameObject
{
public:
    Laser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player);
    virtual ~Laser() = default;

    void Update(double dt) override = 0;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    void SetIsActive(bool active);
    bool IsActive() const;

protected:
    Math::vec2               startPos;
    Math::vec2               direction;
    Player*                  player;
    std::vector<TargetStar*> targets;
    CS200::RGBA              color;
    bool                     isActive = true;

    std::vector<Math::vec2> pathPoints;

    void CalculatePath(int maxBounces, double maxLength);
    void CheckTargetIntersections(double hitRadius);

    static double DistToSegmentSquared(Math::vec2 p, Math::vec2 a, Math::vec2 b);
};