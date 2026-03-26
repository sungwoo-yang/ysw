// RedLaser.cpp
#include "RedLaser.hpp"
#include "ENgine/Engine.hpp"
#include "Engine/Logger.hpp"
#include "Player.hpp"
#include "Shield.hpp"

RedLaser::RedLaser(Math::vec2 startPos, Math::vec2 dir, Player* player, const std::vector<TargetStar*>& targets) : Laser(startPos, dir, player, targets)
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

        // 1. 패링 성공 여부 체크
        if (player != nullptr)
        {
            Shield* shield = player->GetShield();
            if (shield && shield->ConsumeParryState()) // 타이머에 맞게 Guard를 올렸다면
            {
                shield->HandleHit(true); // 쨍! 하는 패링 피드백
                Engine::GetLogger().LogEvent("Perfect Parry Success!");
                // (부모 클래스의 CalculatePath에서 자동으로 방패 반사면이 추가되어 반사됨)
            }
        }

        // 2. 단 한 번만 물리 연산 수행 (최대 5번 바운스)
        CalculatePath(5, 15000.0);
        CheckTargetIntersections(15.0);

        // 3. 플레이어 피격 데미지 판정 (패링 실패시)
        for (size_t i = 0; i < pathPoints.size() - 1; ++i)
        {
            // (간략화된 충돌 거리 체크 로직)
            if ((player->GetPosition() - pathPoints[i]).Length() < 40.0)
            {
                player->ApplyLaserDamage(4.0); // 한 방에 큰 데미지
                Engine::GetLogger().LogEvent("Player Hit by Red Laser!");
                break;
            }
        }
    }
}