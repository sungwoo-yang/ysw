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

    // 패링에 성공했다면 5번 튕기고, 아니라면 반사 없이 직진(0번)
    int bounces = isParried ? 5 : 0;
    CalculatePath(bounces, 3000.0);
    CheckTargetIntersections(15.0);

    if (player == nullptr)
        return;

    // 플레이어 피격 판정
    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        if (DistToSegmentSquared(player->GetPosition(), pathPoints[i], pathPoints[i + 1]) < 40.0 * 40.0)
        {
            Shield* shield = player->GetShield();
            
            // 패링에 성공했고, 발사 직후 첫 번째 선분(i==0)을 방패로 막고 있다면 데미지 무시
            if (isParried && shield && shield->IsGuardUp() && i == 0)
            {
                continue; 
            }

            // 그 외에는 자비 없는 큰 데미지 적용
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