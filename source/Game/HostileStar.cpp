#include "HostileStar.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Player.hpp"
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "YellowLaser.hpp"

HostileStar::HostileStar(Math::vec2 pos, Player* in_player, const std::vector<TargetStar*>& in_targets, StarType type) : Star(pos, in_player, in_targets), currentStarType(type)
{
    // 별의 타입에 따라 기본 사거리와 쿨타임을 다르게 설정할 수 있습니다.
    if (currentStarType == StarType::Red)
    {
        warningDuration  = 1.5; // 빨간색은 경고가 짧고 기습적
        cooldownDuration = 3.0;
    }
    else
    {
        warningDuration  = 2.0; // 노란색은 경고 후 레이저 지속 시간이 김
        cooldownDuration = 5.0;
    }
}

void HostileStar::Update(double dt)
{
    // 부모의 상태 머신(거리 체크, 타이머 갱신)을 먼저 실행합니다.
    HandleBasicAI(dt);

    // 빨간색 별이고 Warning 상태일 때, 발사 직전에 플레이어 방패에 패링 기회를 줍니다.
    if (currentStarType == StarType::Red && currentState == State::Warning)
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

void HostileStar::OnWarningComplete()
{
    Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();
    auto       gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

    // 타입에 맞는 레이저를 생성하여 씬에 추가합니다.
    if (currentStarType == StarType::Red)
    {
        if (player->GetShield())
            player->GetShield()->SetParryWindowActive(false); // 발사 시 패링 윈도우 닫기
        gom->Add(new RedLaser(GetPosition(), dir, player, targets));
    }
    else
    {
        gom->Add(new YellowLaser(GetPosition(), dir, player, targets));
    }
}

CS200::RGBA HostileStar::GetTelegraphColor() const
{
    // 발사 직전 경고선의 색상을 결정합니다. (자식 레이저의 색과 맞춤)
    return (currentStarType == StarType::Red) ? 0xFF0000FF : 0xFFFF00FF;
}