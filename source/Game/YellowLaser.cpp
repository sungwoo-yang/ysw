// YellowLaser.cpp
#include "YellowLaser.hpp"
#include "Player.hpp"
#include "Shield.hpp"
#include <cmath>

YellowLaser::YellowLaser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player) : Laser(in_startPos, dir, in_player)
{
    color         = 0xFFFF00FF;
    currentLength = maxLaserLength;
}

void YellowLaser::Update([[maybe_unused]] double dt)
{
    if (!isActive)
        return;

    if (currentLength < maxLaserLength)
    {
        currentLength += extendSpeed * dt;
        if (currentLength > maxLaserLength)
        {
            currentLength = maxLaserLength;
        }
    }
    CalculatePath(5, currentLength);
    CheckTargetIntersections(15.0);

    if (player == nullptr)
        return;

    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        if (DistToSegmentSquared(player->GetPosition(), pathPoints[i], pathPoints[i + 1]) < 30.0 * 30.0)
        {
            Shield* shield = player->GetShield();

            if (!(shield && shield->IsGuardUp() && i == 0))
            {
                player->ApplyLaserDamage(1.0);
            }
            break;
        }
    }
}