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
    // Utility to find the shortest squared distance from a point to a line segment
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

YellowLaser::YellowLaser(Math::vec2 in_startPos, Math::vec2 in_direction, Player* in_player, const std::vector<TargetStar*>& in_targets)
    : CS230::GameObject(in_startPos), startPos(in_startPos), currentDir(in_direction.Normalize()), player(in_player), targets(in_targets)
{
}

void YellowLaser::Update(double dt)
{
    // Check if the player has successfully outrun the laser's range
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

    // Handle laser lifespan expiration
    timer += dt;
    if (timer >= laserDuration)
    {
        Destroy();
        return;
    }

    // Dynamic tracking and collision updates
    if (player != nullptr)
    {
        UpdateDirection(dt);
    }

    CalculateLaserPath();
    CheckCollisions();
}

void YellowLaser::UpdateDirection(double dt)
{
    // Calculate vector pointing directly at the player
    Math::vec2 targetDir = (player->GetPosition() - startPos).Normalize();

    double currentAngle = std::atan2(currentDir.y, currentDir.x);
    double targetAngle  = std::atan2(targetDir.y, targetDir.x);

    // Normalize angle difference to [-PI, PI] to determine the shortest rotation path
    double diff = targetAngle - currentAngle;
    while (diff <= -PI)
        diff += 2 * PI;
    while (diff > PI)
        diff -= 2 * PI;

    // Apply rotation clamped by the maximum allowed rotation speed
    double maxRotate = rotationSpeed * dt;
    if (std::abs(diff) < maxRotate)
    {
        currentDir = targetDir; // Snap perfectly to target if within step range
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

    // 1. Gather dynamic blocking geometry (Player's Shield)
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

    // 2. Gather static and pushable scene geometry
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
                if (!gate->IsOpen()) // Closed gates block the laser
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

    // 3. Perform physics raycasting to determine the multi-bounce trajectory
    auto calculatedPath = Physics::CalculateLaserPath(startPos, currentDir, allSegments, 5, maxLaserLength);

    // 4. Flatten the path segments into a list of continuous vertices for rendering
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
        // Fail-safe: Shoot straight forward if no collisions are detected at all
        pathPoints.push_back(startPos);
        pathPoints.push_back(startPos + currentDir * maxLaserLength);
    }
}

void YellowLaser::CheckCollisions()
{
    // Iterate over each straight segment of the bounced laser
    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        Math::vec2 p1 = pathPoints[i];
        Math::vec2 p2 = pathPoints[i + 1];

        // 1. Check intersection with the player
        if (player != nullptr)
        {
            double distSq = DistToSegmentSquared(player->GetPosition(), p1, p2);

            // Damage radius expands the hitbox slightly for forgiveness
            if (distSq < (20.0 + damageRadius) * (20.0 + damageRadius))
            {
                bool    safe   = false;
                Shield* shield = player->GetShield();

                // If the player is guarding and the hit occurs on the primary segment (before any bounce), it counts as blocked
                if (shield && shield->IsGuardUp() && i == 0)
                {
                    safe = true;
                }

                if (!safe)
                {
                    // Apply continuous tick damage to the player
                    player->ApplyLaserDamage(1.0);
                    Engine::GetLogger().LogEvent("Player hit by continuous Laser!");
                    return; // Prevent multiple damage ticks from different segments in the same frame
                }
            }
        }

        // 2. Check intersection with puzzle targets
        for (TargetStar* target : targets)
        {
            if (target && !target->IsHit())
            {
                double r = target->GetRadius();
                if (DistToSegmentSquared(target->GetPosition(), p1, p2) <= (r + damageRadius) * (r + damageRadius))
                {
                    target->OnHit(); // Continuously charges the target while intersecting
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

    // Draw the continuous beam along all calculated reflection points
    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        // Draw a thick yellow outer glow followed by a thin white inner core
        renderer.DrawLine(pathPoints[i], pathPoints[i + 1], CS200::WHITE, 4.0);
        renderer.DrawLine(pathPoints[i], pathPoints[i + 1], laserColor, 12.0);
    }
}