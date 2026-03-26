// YellowLaser.cpp
#include "YellowLaser.hpp"
#include "Player.hpp"
#include "Shield.hpp"
#include <cmath>

YellowLaser::YellowLaser(Math::vec2 startPos, Math::vec2 dir, Player* player, const std::vector<TargetStar*>& targets) : Laser(startPos, dir, player, targets)
{
    color = 0xFFFF00FF;
}

void YellowLaser::Update(double dt)
{
    lifeTime += dt;
    if (lifeTime >= maxLifeTime || player == nullptr)
    {
        Destroy();
        return;
    }

    // 1. 플레이어 방향으로 레이저 부드럽게 회전 (추격)
    Math::vec2 targetDir    = (player->GetPosition() - startPos).Normalize();
    double     currentAngle = std::atan2(direction.y, direction.x);
    double     targetAngle  = std::atan2(targetDir.y, targetDir.x);

    double diff = targetAngle - currentAngle;
    while (diff <= -PI)
        diff += 2 * PI;
    while (diff > PI)
        diff -= 2 * PI;

    double maxRotate = rotationSpeed * dt;
    currentAngle += (std::abs(diff) < maxRotate) ? diff : ((diff > 0) ? maxRotate : -maxRotate);
    direction = Math::vec2{ std::cos(currentAngle), std::sin(currentAngle) };

    // 2. 물리 궤적 계산
    CalculatePath(5, 2500.0);
    CheckTargetIntersections(15.0); // 퍼즐 타겟도 충전 가능

    // 3. 플레이어 데미지 판정
    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        Math::vec2 p1 = pathPoints[i];
        Math::vec2 p2 = pathPoints[i + 1];

        // (간략화된 충돌 거리 체크 로직)
        if ((player->GetPosition() - p1).Length() < 30.0) // 실제로는 DistToSegmentSquared 사용
        {
            Shield* shield = player->GetShield();
            // 첫 번째 세그먼트(i==0)에서 방패를 들고 있다면 안전
            if (!(shield && shield->IsGuardUp() && i == 0))
            {
                player->ApplyLaserDamage(1.0); // 지속 틱 데미지
            }
        }
    }
}