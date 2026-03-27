#include "HostileStar.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Player.hpp"
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "YellowLaser.hpp"

HostileStar::HostileStar(Math::vec2 pos, Player* in_player, StarType type) : Star(pos, in_player), currentStarType(type)
{
    warningDuration = 4.0;

    if (currentStarType == StarType::Red)
    {
        cooldownDuration = 4.0;
    }
    else
    {
        cooldownDuration = 9.0;
    }
}

void HostileStar::Update(double dt)
{
    // 레이저를 발사 중인 상태 (Firing State)
    if (activeLaser != nullptr)
    {
        firingTimer += dt;
        if (firingTimer >= currentFiringDuration)
        {
            activeLaser->Destroy();
            activeLaser = nullptr;
        }
        else
        {
            // 노란색일 때만 유도(회전) 로직 실행
            if (currentStarType == StarType::Yellow)
            {
                Math::vec2 targetDir    = (player->GetPosition() - GetPosition()).Normalize();
                double     currentAngle = std::atan2(laserDirection.y, laserDirection.x);
                double     targetAngle  = std::atan2(targetDir.y, targetDir.x);

                double diff = targetAngle - currentAngle;
                while (diff <= -PI)
                    diff += 2 * PI;
                while (diff > PI)
                    diff -= 2 * PI;

                double maxRotate = rotationSpeed * dt;
                currentAngle += (std::abs(diff) < maxRotate) ? diff : ((diff > 0) ? maxRotate : -maxRotate);

                laserDirection = Math::vec2{ std::cos(currentAngle), std::sin(currentAngle) };
                activeLaser->SetDirection(laserDirection);
            }
            activeLaser->SetStartPos(GetPosition());
        }
    }
    else
    {
        // 레이저 발사 중이 아닐 때만 다음 경고/쿨타임 타이머 진행
        HandleBasicAI(dt);
    }

    // 패링 윈도우 로직
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

    if (currentStarType == StarType::Red)
    {
        bool    parrySuccess = false;
        Shield* shield       = player->GetShield();

        // 발사 순간에 플레이어의 패링 상태를 체크
        if (shield && shield->ConsumeParryState())
        {
            shield->HandleHit(true);
            Engine::GetLogger().LogEvent("Hostile Red Laser Perfect Parry!");
            parrySuccess = true;
        }

        if (shield)
            shield->SetParryWindowActive(false);

        RedLaser* rLaser = new RedLaser(GetPosition(), dir, player);

        // 패링 성공 시 레이저 데미지 비활성화 (시각 효과만 남김)
        if (parrySuccess)
        {
            rLaser->SetParried(true);
        }

        gom->Add(rLaser);
        activeLaser           = rLaser;
        firingTimer           = 0.0;
        currentFiringDuration = 0.2; // 빨간 레이저는 짧은 버스트
        laserDirection        = dir;
    }
    else
    {
        YellowLaser* yLaser = new YellowLaser(GetPosition(), dir, player);
        gom->Add(yLaser);

        activeLaser           = yLaser;
        firingTimer           = 0.0;
        currentFiringDuration = 5.0; // 노란 레이저는 5초 유지
        laserDirection        = dir;
        rotationSpeed         = 1.5;
    }
}

CS200::RGBA HostileStar::GetTelegraphColor() const
{
    if (currentStarType == StarType::Red && currentState == State::Warning && timer <= parryWindowTime)
    {
        return 0x00FFFFFF; // 청록색(Cyan)
    }
    return (currentStarType == StarType::Red) ? 0xFF0000FF : 0xFFFF00FF;
}

void HostileStar::SetStarType(StarType newType)
{
    currentStarType = newType;

    if (currentStarType == StarType::Red)
    {
        cooldownDuration = 4.0;
    }
    else
    {
        cooldownDuration = 9.0;
    }
}