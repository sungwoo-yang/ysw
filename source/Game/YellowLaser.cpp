// YellowLaser.cpp
#include "YellowLaser.hpp"
#include "Player.hpp"
#include "Shield.hpp"
#include <cmath>

YellowLaser::YellowLaser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player) : Laser(in_startPos, dir, in_player)
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

    CalculatePath(5, 2500.0);
    CheckTargetIntersections(15.0);

    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        Math::vec2 p1 = pathPoints[i];

        if ((player->GetPosition() - p1).Length() < 30.0)
        {
            Shield* shield = player->GetShield();

            if (!(shield && shield->IsGuardUp() && i == 0))
            {
                player->ApplyLaserDamage(1.0);
            }
        }
    }
}