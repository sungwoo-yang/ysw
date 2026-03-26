#include "BossStar.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Player.hpp"
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "YellowLaser.hpp"

BossStar::BossStar(Math::vec2 pos, Player* in_player, const std::vector<TargetStar*>& in_targets) : Star(pos, in_player, in_targets), attackStep(0) // 첫 공격은 Red(0)부터 시작
{
    // 보스전의 템포에 맞게 조정 (기존 BossStar 밸런스 참고)
    detectionRadius  = 800.0;
    chaseRadius      = 1600.0; // 보스방에서는 플레이어를 끝까지 물고 늘어짐
    warningDuration  = 1.5;
    cooldownDuration = 3.0; // 패턴 사이의 간격
}

void BossStar::Update(double dt)
{
    // 1. 부모의 기본 상태 머신(대기->경고->발사 대기) 실행
    HandleBasicAI(dt);

    // 2. 현재 쏠 레이저가 '빨간색(패링용)'이고, Warning 상태일 때 패링 윈도우 개방
    if (attackStep < 2 && currentState == State::Warning)
    {
        Shield* shield = player->GetShield();
        if (shield != nullptr)
        {
            if (timer <= parryWindowTime)
                shield->SetParryWindowActive(true);
            else
                shield->SetParryWindowActive(false);
        }
    }

    CS230::GameObject::Update(dt);
}

void BossStar::OnWarningComplete()
{
    Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();
    auto       gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

    if (attackStep < 2)
    {
        // Step 0, 1: 빨간색 레이저 발사
        if (player->GetShield())
            player->GetShield()->SetParryWindowActive(false);
        gom->Add(new RedLaser(GetPosition(), dir, player, targets));
        Engine::GetLogger().LogEvent("Boss Fired Red Laser! (Parry Required)");
    }
    else
    {
        // Step 2: 노란색 레이저 발사
        gom->Add(new YellowLaser(GetPosition(), dir, player, targets));
        Engine::GetLogger().LogEvent("Boss Fired Yellow Laser! (Evade)");
    }

    // 다음 패턴으로 넘어가기 (0 -> 1 -> 2 -> 0 반복)
    attackStep = (attackStep + 1) % 3;
}

CS200::RGBA BossStar::GetTelegraphColor() const
{
    // 현재 attackStep에 맞춰 예고선 색상 변경
    if (attackStep < 2)
        return 0xFF0000FF; // Red
    else
        return 0xFFFF00FF; // Yellow
}