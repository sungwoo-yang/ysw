// RedLaser.cpp
#include "RedLaser.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"
#include "Player.hpp"
#include "RedHitParticle.hpp"
#include "Shield.hpp"

RedLaser::RedLaser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player) : Laser(in_startPos, dir, in_player)
{
    color = 0xFF0000FF;
}

void RedLaser::Update([[maybe_unused]] double dt)
{
    if (!isActive)
        return;
    double laserLength = 3000.0;

    if (isParried && player != nullptr)
    {
        Math::vec2 toPlayer = player->GetPosition() - startPos;
        double     dist     = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);

        laserLength = dist - 60.0;
        if (laserLength < 0.0)
            laserLength = 0.0;
    }

    int bounces = 0;
    CalculatePath(bounces, laserLength);
    CheckTargetIntersections(15.0);

    if (isParried && !hasEmittedParryParticle && pathPoints.size() >= 2)
    {
        if (pathPoints.size() >= 2)
        {
            Math::vec2 hitPos = startPos + (direction * laserLength);

            auto pManager = Engine::GetGameStateManager().GetGSComponent<CS230::ParticleManager<RedHitParticle>>();

            if (pManager != nullptr)
            {
                pManager->Emit(30, hitPos, { 0.0, 0.0 }, -direction * 400.0, PI * 2.0);
            }
            hasEmittedParryParticle = true;
        }
    }

    if (player == nullptr)
        return;

    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        if (DistToSegmentSquared(player->GetPosition(), pathPoints[i], pathPoints[i + 1]) < 40.0 * 40.0)
        {
            if (isParried && i == 0)
            {
                continue;
            }

            player->ApplyLaserDamage(4.0);
            Engine::GetLogger().LogEvent("Player Hit by Fatal Red Laser!");
            break;
        }
    }
}

void RedLaser::SetParried(bool parried)
{
    isParried = parried;
}