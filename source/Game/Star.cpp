#include "Star.hpp"
#include "Player.hpp"
#include "TargetStar.hpp"
#include "Shield.hpp"
#include "Mirror.hpp"
#include "PushableMirror.hpp"
#include "Gate.hpp"
#include "Engine/MapElement.h"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/GameObjectManager.hpp"
#include "CS200/IRenderer2D.hpp"

Star::Star(Math::vec2 pos, Player* in_player, const std::vector<TargetStar*>& in_targets)
    : CS230::GameObject(pos), currentState(State::Idle), timer(0.0), player(in_player), targets(in_targets)
{
}

void Star::Update(double dt)
{
    // 기본적으로 상태 머신 AI를 실행합니다.
    HandleBasicAI(dt);
    CS230::GameObject::Update(dt);
}

void Star::HandleBasicAI(double dt)
{
    if (player == nullptr) return;

    double distance = (player->GetPosition() - GetPosition()).Length();

    switch (currentState)
    {
        case State::Idle:
            if (distance <= detectionRadius)
            {
                currentState = State::Warning;
                timer = warningDuration;
            }
            break;

        case State::Warning:
            if (distance > chaseRadius)
            {
                currentState = State::Idle;
                timer = 0.0;
                return;
            }

            timer -= dt;
            if (timer <= 0.0)
            {
                // 자식 클래스(HostileStar, BossStar 등)에서 재정의한 실제 발사 함수 호출
                OnWarningComplete(); 
                
                currentState = State::Cooldown;
                timer = cooldownDuration;
            }
            break;

        case State::Cooldown:
            timer -= dt;
            if (timer <= 0.0)
            {
                currentState = State::Idle;
            }
            break;
    }
}

void Star::Draw(const Math::TransformationMatrix& camera_matrix)
{
    auto& renderer = Engine::GetRenderer2D();

    // 별 자체를 그립니다. 자식 클래스에서 정의한 색상(GetTelegraphColor)을 사용합니다.
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix({ 40.0, 40.0 });
    renderer.DrawCircle(transform, GetTelegraphColor());

    // Warning 상태일 때만 레이저가 튈 경로를 미리 보여줍니다.
    if (currentState == State::Warning)
    {
        DrawTrajectory();
    }

    CS230::GameObject::Draw(camera_matrix);
}

void Star::DrawTrajectory()
{
    if (player == nullptr) return;

    std::vector<Physics::LineSegment> allSegments;
    
    // 1. 방패 세그먼트 수집
    Shield* shield = player->GetShield();
    if (shield && shield->IsGuardUp())
    {
        auto segs = shield->GetSegments();
        for (const auto& s : segs) allSegments.push_back({ s.first, s.second, true });
    }

    // 2. 맵의 반사 객체 수집
    auto gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();
    if (gom != nullptr)
    {
        for (auto obj : gom->GetObjects())
        {
            if (obj->Type() == GameObjectTypes::Mirror)
            {
                allSegments.push_back(static_cast<Mirror*>(obj)->GetReflectiveSegment());
            }
            else if (obj->Type() == GameObjectTypes::PushableMirror)
            {
                auto mirror = static_cast<PushableMirror*>(obj);
                auto segs = mirror->GetSegments();
                for (const auto& s : segs) allSegments.push_back({ s.first, s.second, true });
            }
            else if (obj->Type() == GameObjectTypes::Floor)
            {
                auto wall = static_cast<CS230::MapElement*>(obj);
                auto segs = wall->GetWallSegments();
                allSegments.insert(allSegments.end(), segs.begin(), segs.end());
            }
            else if (obj->Type() == GameObjectTypes::Gate)
            {
                auto gate = static_cast<Gate*>(obj);
                if (!gate->IsOpen())
                {
                    Math::vec2 p = gate->GetPosition();
                    allSegments.push_back({{ p.x - 50, p.y }, { p.x + 50, p.y }, false});
                }
            }
        }
    }

    // 3. 레이저 궤적 연산
    Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();
    auto path = Physics::CalculateLaserPath(GetPosition(), dir, allSegments, 2, 15000.0);

    // 4. 예고선 렌더링 (살짝 투명하게 처리)
    auto& renderer = Engine::GetRenderer2D();
    CS200::RGBA lineColor = GetTelegraphColor();
    lineColor = (lineColor & 0xFFFFFF00) | 0x80;

    for (const auto& seg : path)
    {
        renderer.DrawLine(seg.first, seg.second, lineColor, 2.0);
    }
}