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

void RedLaser::Update(double dt)
{
    visualLifeTime -= dt;
    if (visualLifeTime <= 0.0)
    {
        Destroy();
        return;
    }

    if (!isCalculated)
    {
        isCalculated = true;

        if (player != nullptr)
        {
            Shield* shield = player->GetShield();
            if (shield && shield->ConsumeParryState())
            {
                shield->HandleHit(true);
                Engine::GetLogger().LogEvent("Perfect Parry Success!");
            }
        }

        CalculatePath(5, 15000.0);
        CheckTargetIntersections(15.0);

        for (size_t i = 0; i < pathPoints.size() - 1; ++i)
        {
            if ((player->GetPosition() - pathPoints[i]).Length() < 40.0)
            {
                player->ApplyLaserDamage(4.0);
                Engine::GetLogger().LogEvent("Player Hit by Red Laser!");
                break;
            }
        }
    }
}