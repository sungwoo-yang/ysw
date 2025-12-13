#include "YellowLaser.hpp"
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
#include "PushableMirror.hpp"
#include "Shield.hpp"
#include "TargetStar.hpp"
#include <algorithm>
#include <cmath>

namespace
{
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

YellowLaser::YellowLaser(Math::vec2 startPos, Math::vec2 direction, Player* player, const std::vector<TargetStar*>& targets)
    : CS230::GameObject(startPos), startPos(startPos), currentDir(direction.Normalize()), player(player), targets(targets), timer(0.0)
{
    CalculateLaserPath();
}

void YellowLaser::Update(double dt)
{
    if (player != nullptr)
    {
        double distanceToPlayer = (player->GetPosition() - startPos).Length();

        if (distanceToPlayer > chaseRange)
        {
            Engine::GetLogger().LogEvent("Player escaped laser chase range!");
            Destroy();
            return;
        }
    }

    timer += dt;
    if (timer >= laserDuration)
    {
        Destroy();
        return;
    }

    if (player != nullptr)
    {
        UpdateDirection(dt);
    }

    CalculateLaserPath();
    CheckCollisions();
}

void YellowLaser::UpdateDirection(double dt)
{
    Math::vec2 targetDir = (player->GetPosition() - startPos).Normalize();

    double currentAngle = std::atan2(currentDir.y, currentDir.x);
    double targetAngle  = std::atan2(targetDir.y, targetDir.x);

    double diff = targetAngle - currentAngle;
    while (diff <= -PI)
        diff += 2 * PI;
    while (diff > PI)
        diff -= 2 * PI;

    double maxRotate = rotationSpeed * dt;
    if (std::abs(diff) < maxRotate)
    {
        currentDir = targetDir;
    }
    else
    {
        currentAngle += (diff > 0) ? maxRotate : -maxRotate;
        currentDir = Math::vec2{ std::cos(currentAngle), std::sin(currentAngle) };
    }
}

void YellowLaser::CalculateLaserPath()
{
    std::vector<Physics::LineSegment> allSegments;

    if (player != nullptr)
    {
        Shield* shield = player->GetShield();
        if (shield && shield->IsGuardUp())
        {
            auto shieldSegments = shield->GetSegments();
            for (const auto& seg : shieldSegments)
            {
                allSegments.push_back({ seg.first, seg.second, true });
            }
        }
    }

    auto gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();
    if (gom)
    {
        for (auto obj : gom->GetObjects())
        {
            if (obj->Type() == GameObjectTypes::Mirror)
            {
                allSegments.push_back(static_cast<Mirror*>(obj)->GetReflectiveSegment());
            }
            else if (obj->Type() == GameObjectTypes::PushableMirror)
            {
                auto mirror         = static_cast<PushableMirror*>(obj);
                auto mirrorSegments = mirror->GetSegments();
                for (const auto& seg : mirrorSegments)
                {
                    allSegments.push_back({ seg.first, seg.second, true });
                }
            }
            else if (obj->Type() == GameObjectTypes::Floor)
            {
                auto wall     = static_cast<CS230::MapElement*>(obj);
                auto wallSegs = wall->GetWallSegments();
                allSegments.insert(allSegments.end(), wallSegs.begin(), wallSegs.end());
            }
            else if (obj->Type() == GameObjectTypes::Gate)
            {
                auto gate = static_cast<Gate*>(obj);
                if (!gate->IsOpen())
                {
                    Math::vec2 pos = gate->GetPosition();
                    allSegments.push_back(
                        {
                            { pos.x - 50, pos.y },
                            { pos.x + 50, pos.y },
                            false
                    });
                }
            }
        }
    }

    auto calculatedPath = Physics::CalculateLaserPath(startPos, currentDir, allSegments, 5, maxLaserLength);

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
        pathPoints.push_back(startPos + currentDir * maxLaserLength);
    }
}

void YellowLaser::CheckCollisions()
{
    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        Math::vec2 p1 = pathPoints[i];
        Math::vec2 p2 = pathPoints[i + 1];

        if (player != nullptr)
        {
            double distSq = DistToSegmentSquared(player->GetPosition(), p1, p2);
            if (distSq < (20.0 + damageRadius) * (20.0 + damageRadius))
            {
                bool    safe   = false;
                Shield* shield = player->GetShield();
                if (shield && shield->IsGuardUp() && i == 0)
                {
                    safe = true;
                }

                if (!safe)
                {
                    Engine::GetLogger().LogEvent("Player hit by continuous Laser!");
                    return;
                }
            }
        }

        for (TargetStar* target : targets)
        {
            if (target && !target->IsHit())
            {
                double r = target->GetRadius();
                if (DistToSegmentSquared(target->GetPosition(), p1, p2) <= (r + damageRadius) * (r + damageRadius))
                {
                    target->OnHit();
                    Engine::GetLogger().LogEvent("Target Star activated by Laser!");
                }
            }
        }
    }
}

void YellowLaser::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
{
    auto& renderer = Engine::GetRenderer2D();

    if (pathPoints.size() < 2)
        return;

    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        renderer.DrawLine(pathPoints[i], pathPoints[i + 1], CS200::WHITE, 4.0);
        renderer.DrawLine(pathPoints[i], pathPoints[i + 1], laserColor, 12.0);
    }
}