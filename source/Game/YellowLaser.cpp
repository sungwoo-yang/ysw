#include "YellowLaser.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"
#include "Engine/Physics/Reflection.hpp"
#include "Player.hpp"
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
    pathPoints.clear();
    pathPoints.push_back(startPos);

    Math::vec2 rayOrigin         = startPos;
    Math::vec2 rayDir            = currentDir;
    double     remainingDistance = maxLaserLength;

    int       bounces    = 0;
    const int maxBounces = 5;

    while (bounces < maxBounces && remainingDistance > 0)
    {
        Math::vec2 closestIntersection;
        double     closestT  = remainingDistance;
        bool       hitShield = false;
        Math::vec2 normal    = { 0, 0 };

        if (player != nullptr)
        {
            Shield* shield = player->GetShield();

            if (shield && shield->IsGuardUp())
            {
                auto segments = shield->GetSegments();
                for (const auto& seg : segments)
                {
                    Math::vec2 intersection;
                    double     t;
                    if (Physics::RaySegmentIntersection(rayOrigin, rayDir, seg.first, seg.second, intersection, t))
                    {
                        if (t > 0.1 && t < closestT)
                        {
                            closestT            = t;
                            closestIntersection = intersection;
                            hitShield           = true;

                            Math::vec2 wallVec = seg.second - seg.first;
                            normal             = Math::vec2{ -wallVec.y, wallVec.x }.Normalize();
                            if (Math::dot(rayDir, normal) > 0)
                            {
                                normal = -normal;
                            }
                        }
                    }
                }
            }
        }

        if (hitShield)
        {
            pathPoints.push_back(closestIntersection);
            rayOrigin = closestIntersection + normal * 0.1;
            rayDir    = Physics::CalculateReflection(rayDir, normal);
            remainingDistance -= closestT;
            bounces++;
        }
        else
        {
            pathPoints.push_back(rayOrigin + rayDir * remainingDistance);
            break;
        }
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