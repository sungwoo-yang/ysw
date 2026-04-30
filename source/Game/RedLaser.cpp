// RedLaser.cpp
#include "RedLaser.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"
#include "Engine/Random.hpp"
#include "Player.hpp"
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
        Math::vec2 hitPos = startPos + (direction * laserLength);

        for (int i = 0; i < 6; ++i)
        {
            ParryRect rect;
            rect.pos = hitPos;

            double angle = util::random(-PI / 2.0, PI / 2.0);

            Math::vec2 baseDir = -direction;
            Math::vec2 rotDir  = { baseDir.x * std::cos(angle) - baseDir.y * std::sin(angle), baseDir.x * std::sin(angle) + baseDir.y * std::cos(angle) };

            rect.vel  = rotDir * util::random(300.0, 600.0);
            rect.life = util::random(0.15, 0.35);

            parryRects.push_back(rect);
        }
        hasEmittedParryParticle = true;
    }

    for (auto& rect : parryRects)
    {
        if (rect.life > 0.0)
        {
            rect.pos += rect.vel * dt;
            rect.life -= dt;
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

void RedLaser::Draw(const Math::TransformationMatrix& camera_matrix)
{
    Laser::Draw(camera_matrix);

    auto& renderer = Engine::GetRenderer2D();

    for (const auto& rect : parryRects)
    {
        if (rect.life > 0.0)
        {
            renderer.DrawLine(rect.pos - Math::vec2{ 4.0, 0.0 }, rect.pos + Math::vec2{ 4.0, 0.0 }, 0xFF0000FF, 8.0);
        }
    }
}

void RedLaser::SetParried(bool parried)
{
    isParried = parried;
}