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

Laser::Laser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player) : CS230::GameObject(in_startPos), startPos(in_startPos), direction(dir.Normalize()), player(in_player), color(CS200::WHITE)
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
        pathPoints.push_back(startPos);
        pathPoints.push_back(startPos + direction * maxLength);
    }
}

void Laser::CheckTargetIntersections(double hitRadius)
{
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
                    target->OnHit();
                }
            }
        }
    }
}

void Laser::Draw(const Math::TransformationMatrix& camera_matrix)
{
    if (!isActive)
        return;

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

void Laser::SetStartPos(Math::vec2 newPos)
{
    startPos = newPos;
}

void Laser::SetDirection(Math::vec2 newDir)
{
    direction = newDir.Normalize();
}

void Laser::SetIsActive(bool active)
{
    isActive = active;

    if (!isActive)
    {
        pathPoints.clear();
    }
}

bool Laser::IsActive() const
{
    return isActive;
}

double Laser::DistToSegmentSquared(Math::vec2 p, Math::vec2 a, Math::vec2 b)
{
    double l2 = (b - a).LengthSquared();
    if (l2 == 0.0)
        return (p - a).LengthSquared();

    double     t          = std::max(0.0, std::min(1.0, (p - a).Dot(b - a) / l2));
    Math::vec2 projection = a + (b - a) * t;

    return (p - projection).LengthSquared();
}