#include "BossStar.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapElement.h"
#include "Engine/Physics/Reflection.hpp"
#include "Game/Gate.hpp"
#include "Game/Mirror.hpp"
#include "Player.hpp"
#include "PushableMirror.hpp" // [추가] 이 헤더가 빠져서 오류가 발생했습니다.
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "YellowLaser.hpp"

BossStar::BossStar(Math::vec2 position, Player* player, const std::vector<TargetStar*>& targets)
    : CS230::GameObject(position), player(player), targets(targets), currentState(State::Idle), nextLaserType(NextLaser::Red), timer(0.0)
{
}

void BossStar::Update(double dt)
{
    if (player == nullptr)
        return;

    double distance = (player->GetPosition() - GetPosition()).Length();

    switch (currentState)
    {
        case State::Idle:
            if (distance <= detectionRadius)
            {
                currentState = State::Warning;
                timer        = warningDuration;
                Engine::GetLogger().LogEvent("Boss Detected Player! Charging Laser...");
            }
            break;

        case State::Warning:
            // 플레이어가 너무 멀어지면 취소 (선택 사항)
            if (distance > detectionRadius * 1.5)
            {
                currentState = State::Idle;
                timer        = 0.0;
                return;
            }

            timer -= dt;
            if (timer <= 0.0)
            {
                Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();
                auto       gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

                if (nextLaserType == NextLaser::Red)
                {
                    // 레드 레이저 발사 -> 다음은 옐로우
                    gom->Add(new RedLaser(GetPosition(), dir, player, targets));
                    nextLaserType = NextLaser::Yellow;
                    Engine::GetLogger().LogEvent("Boss Fired Red Laser!");
                }
                else
                {
                    // 옐로우 레이저 발사 -> 다음은 레드
                    gom->Add(new YellowLaser(GetPosition(), dir, player, targets));
                    nextLaserType = NextLaser::Red;
                    Engine::GetLogger().LogEvent("Boss Fired Yellow Laser!");
                }

                currentState = State::Cooldown;
                timer        = cooldownDuration;
            }
            break;

        case State::Cooldown:
            timer -= dt;
            if (timer <= 0.0)
            {
                currentState = State::Idle; // 다시 대기 상태로
            }
            break;
    }

    CS230::GameObject::Update(dt);
}

void BossStar::Draw(const Math::TransformationMatrix& camera_matrix)
{
    auto& renderer = Engine::GetRenderer2D();

    // 보스 외형 (크고, 다음 레이저 색상으로 빛남)
    CS200::RGBA bossColor;
    if (nextLaserType == NextLaser::Red)
        bossColor = 0xFF0000FF; // Red
    else
        bossColor = 0xFFFF00FF; // Yellow

    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix({ size, size });
    renderer.DrawCircle(transform, bossColor);

    // 경고선 그리기 (Warning 상태일 때만)
    if (currentState == State::Warning && player != nullptr)
    {
        // 경고선 색상 (깜빡임 효과)
        CS200::RGBA lineColor = bossColor;
        if (static_cast<int>(timer * 10) % 2 == 0)
            lineColor = CS200::WHITE;                // 깜빡깜빡
        lineColor = (lineColor & 0xFFFFFF00) | 0x80; // 반투명

        // 예상 경로 계산 (Reflection 로직 재사용)
        std::vector<Physics::LineSegment> allSegments;

        // (1) 쉴드
        Shield* shield = player->GetShield();
        if (shield && shield->IsGuardUp())
        {
            auto segs = shield->GetSegments();
            for (auto& s : segs)
                allSegments.push_back({ s.first, s.second, true });
        }
        // (2) 맵 오브젝트
        auto gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();
        for (auto obj : gom->GetObjects())
        {
            if (obj->Type() == GameObjectTypes::Mirror)
            {
                allSegments.push_back(static_cast<Mirror*>(obj)->GetReflectiveSegment());
            }
            else if (obj->Type() == GameObjectTypes::PushableMirror)
            {
                // [수정된 부분] PushableMirror 인식
                auto mirror = static_cast<PushableMirror*>(obj);
                auto segs   = mirror->GetSegments();
                for (auto& s : segs)
                    allSegments.push_back({ s.first, s.second, true });
            }
            else if (obj->Type() == GameObjectTypes::Floor)
            {
                auto segs = static_cast<CS230::MapElement*>(obj)->GetWallSegments();
                allSegments.insert(allSegments.end(), segs.begin(), segs.end());
            }
            else if (obj->Type() == GameObjectTypes::Gate)
            {
                auto gate = static_cast<Gate*>(obj);
                if (!gate->IsOpen())
                {
                    Math::vec2 p = gate->GetPosition();
                    allSegments.push_back(
                        {
                            { p.x - 50, p.y },
                            { p.x + 50, p.y },
                            false
                    });
                }
            }
        }

        Math::vec2 dir  = (player->GetPosition() - GetPosition()).Normalize();
        auto       path = Physics::CalculateLaserPath(GetPosition(), dir, allSegments, 2, 2000.0);

        for (const auto& seg : path)
        {
            renderer.DrawLine(seg.first, seg.second, lineColor, 3.0);
        }
    }

    CS230::GameObject::Draw(camera_matrix);
}