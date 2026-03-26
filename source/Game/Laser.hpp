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
    Laser(Math::vec2 startPos, Math::vec2 dir, Player* player, const std::vector<TargetStar*>& targets);
    virtual ~Laser() = default;

    void Update(double dt) override = 0; // Instant냐 Continuous냐에 따라 다름
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

protected:
    Math::vec2               startPos;
    Math::vec2               direction;
    Player*                  player;
    std::vector<TargetStar*> targets;
    CS200::RGBA              color;

    // 계산된 레이저의 정점들 (반사 지점들)
    std::vector<Math::vec2> pathPoints;

    void CalculatePath(int maxBounces, double maxLength); // 공통 물리 로직
    void CheckTargetIntersections(double hitRadius);      // 공통 타겟 충돌 로직
};