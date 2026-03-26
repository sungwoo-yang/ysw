#include "PuzzleStar.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "WhiteLaser.hpp"
#include <cmath>

PuzzleStar::PuzzleStar(Math::vec2 pos, Player* in_player, const std::vector<TargetStar*>& in_targets, Pattern pattern, Math::vec2 initialDir)
    : Star(pos, in_player, in_targets), currentPattern(pattern), aimDirection(initialDir.Normalize()), myLaser(nullptr)
{
    // 생성과 동시에 레이저를 하나 만들어서 게임오브젝트 매니저에 등록합니다.
    myLaser = new WhiteLaser(GetPosition(), aimDirection, player, targets);
    Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>()->Add(myLaser);
}

PuzzleStar::~PuzzleStar()
{
    // 별이 파괴될 때 자신이 쏘던 레이저도 같이 지워줍니다.
    if (myLaser != nullptr)
    {
        myLaser->Destroy();
    }
}

void PuzzleStar::Update(double dt)
{
    // 부모의 HandleBasicAI(대기->경고->발사)를 완전히 무시하고 퍼즐 패턴만 돌립니다.

    switch (currentPattern)
    {
        case Pattern::Static:
            // 방향이 바뀌지 않으므로 위치만 계속 동기화해줍니다.
            if (myLaser)
                myLaser->SetStartPos(GetPosition());
            break;

        case Pattern::Rotating:
            {
                // 현재 방향을 회전시킵니다.
                double currentAngle = std::atan2(aimDirection.y, aimDirection.x);
                currentAngle += rotationSpeed * dt;

                aimDirection = Math::vec2{ std::cos(currentAngle), std::sin(currentAngle) };

                if (myLaser)
                {
                    myLaser->SetStartPos(GetPosition());
                    myLaser->SetDirection(aimDirection);
                }
                break;
            }

        case Pattern::Blink:
            {
                blinkTimer += dt;
                if (blinkTimer >= blinkInterval)
                {
                    blinkTimer = 0.0;
                    isLaserOn  = !isLaserOn;
                    SetLaserActive(isLaserOn);
                }

                if (myLaser)
                    myLaser->SetStartPos(GetPosition());
                break;
            }
    }

    CS230::GameObject::Update(dt);
}

void PuzzleStar::SetLaserActive(bool active)
{
    if (active && myLaser == nullptr)
    {
        // 레이저 켜기
        myLaser = new WhiteLaser(GetPosition(), aimDirection, player, targets);
        Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>()->Add(myLaser);
    }
    else if (!active && myLaser != nullptr)
    {
        // 레이저 끄기
        myLaser->Destroy();
        myLaser = nullptr;
    }
}