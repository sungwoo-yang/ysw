// YellowLaser.cpp
#include "YellowLaser.hpp"
#include "Player.hpp"
#include "Shield.hpp"
#include <cmath>

YellowLaser::YellowLaser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player) : Laser(in_startPos, dir, in_player)
{
    color = 0xFFFF00FF;
}

void YellowLaser::Update([[maybe_unused]]double dt)
{
    if (!isActive) return;
    
    // 하얀색 레이저처럼 반사되며 쭉 뻗어나감 (최대 5번 반사)
    CalculatePath(5, 3000.0);
    CheckTargetIntersections(15.0);

    if (player == nullptr) return;

    // 플레이어 피격 판정
    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        if (DistToSegmentSquared(player->GetPosition(), pathPoints[i], pathPoints[i + 1]) < 30.0 * 30.0)
        {
            Shield* shield = player->GetShield();
            
            // 방패로 막지 못했을 경우 약한 데미지
            if (!(shield && shield->IsGuardUp() && i == 0))
            {
                player->ApplyLaserDamage(1.0);
            }
            break; // 한 번 맞으면 중복 데미지 방지
        }
    }
}