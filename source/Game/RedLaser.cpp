// RedLaser.cpp
#include "RedLaser.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"
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

    int bounces = isParried ? 5 : 0;
    CalculatePath(bounces, 3000.0);
    CheckTargetIntersections(15.0);

    if (player == nullptr)
        return;

    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        if (DistToSegmentSquared(player->GetPosition(), pathPoints[i], pathPoints[i + 1]) < 40.0 * 40.0)
        {
            Shield* shield = player->GetShield();

                if (isParried && shield && shield->IsGuardUp() && i == 0)
            {
                auto segs = shield->GetSegments();
                if (!segs.empty())
                {
                    Math::vec2 p1      = segs[0].first;
                    Math::vec2 p2      = segs[0].second;
                    Math::vec2 wallVec = p2 - p1;
                    Math::vec2 normal  = Math::vec2{ -wallVec.y, wallVec.x }.Normalize();

                    Math::vec2 shieldCenter = { (p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5 };
                    if ((shieldCenter - player->GetPosition()).Dot(normal) < 0)
                        normal = -normal;

                    if (direction.Dot(normal) < 0)
                        continue;
                }
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