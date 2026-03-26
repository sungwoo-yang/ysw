#include "Laser.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/MapElement.h"
#include "Gate.hpp"
#include "Mirror.hpp"
#include "Player.hpp"
#include "PushableMirror.hpp"
#include "Shield.hpp"
#include "TargetStar.hpp"
#include <algorithm>

namespace
{
    // 선분과 점 사이의 최단 거리의 제곱을 구하는 유틸리티 함수
    double DistToSegmentSquared(Math::vec2 p, Math::vec2 v, Math::vec2 w)
    {
        double l2 = (w - v).Dot(w - v);
        if (l2 == 0.0)
            return (p - v).Dot(p - v);

        double t              = ((p - v).Dot(w - v)) / l2;
        t                     = std::max(0.0, std::min(1.0, t));
        Math::vec2 projection = v + (w - v) * t;

        return (p - projection).Dot(p - projection);
    }
}

Laser::Laser(Math::vec2 startPos, Math::vec2 dir, Player* in_player, const std::vector<TargetStar*>& in_targets)
    : CS230::GameObject(startPos), startPos(startPos), direction(dir.Normalize()), player(in_player), targets(in_targets), color(CS200::WHITE)
{
}

void Laser::CalculatePath(int maxBounces, double maxLength)
{
    std::vector<Physics::LineSegment> allSegments;

    if (player != nullptr)
    {
        Shield* shield = player->GetShield();
        if (shield && shield->IsGuardUp())
        {
            auto segs = shield->GetSegments();
            for (const auto& s : segs)
                allSegments.push_back({ s.first, s.second, true });
        }
    }

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
                auto segs   = mirror->GetSegments();
                for (const auto& s : segs)
                    allSegments.push_back({ s.first, s.second, true });
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
                    allSegments.push_back(
                        {
                            { p.x - 50, p.y },
                            { p.x + 50, p.y },
                            false
                    });
                }
            }
        }
    }

    auto calculatedPath = Physics::CalculateLaserPath(startPos, direction, allSegments, maxBounces, maxLength);

    // 계산된 세그먼트들을 그리기 편하도록 정점(Point) 리스트로 평탄화합니다.
    pathPoints.clear();
    if (!calculatedPath.empty())
    {
        pathPoints.push_back(calculatedPath[0].first);
        for (const auto& segment : calculatedPath)
        {
            pathPoints.push_back(segment.second);
        }
    }
    else
    {
        // 충돌이 아예 없으면 앞으로 일직선
        pathPoints.push_back(startPos);
        pathPoints.push_back(startPos + direction * maxLength);
    }
}

void Laser::CheckTargetIntersections(double hitRadius)
{
    // 계산된 레이저 경로들을 순회하며 퍼즐 타겟이 맞았는지 검사합니다.
    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        Math::vec2 p1 = pathPoints[i];
        Math::vec2 p2 = pathPoints[i + 1];

        for (TargetStar* target : targets)
        {
            if (target && !target->IsHit())
            {
                double r = target->GetRadius();
                if (DistToSegmentSquared(target->GetPosition(), p1, p2) <= (r + hitRadius) * (r + hitRadius))
                {
                    target->OnHit(); // 닿아있는 동안 활성화 신호를 보냄
                }
            }
        }
    }
}

void Laser::Draw(const Math::TransformationMatrix& camera_matrix)
{
    auto& renderer = Engine::GetRenderer2D();

    if (pathPoints.size() < 2)
        return;

    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        renderer.DrawLine(pathPoints[i], pathPoints[i + 1], CS200::WHITE, 4.0);
        renderer.DrawLine(pathPoints[i], pathPoints[i + 1], color, 12.0);
    }

    CS230::GameObject::Draw(camera_matrix);
}