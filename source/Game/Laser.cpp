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
#include <cmath>

namespace
{
    CS200::RGBA WithAlpha(CS200::RGBA baseColor, double alpha)
    {
        const auto alphaByte = static_cast<CS200::RGBA>(std::clamp(alpha, 0.0, 1.0) * 255.0);
        return (baseColor & 0xFFFFFF00u) | alphaByte;
    }

    CS200::RGBA MixWithWhite(CS200::RGBA baseColor, double whiteAmount, double alpha)
    {
        const double amount = std::clamp(whiteAmount, 0.0, 1.0);

        const auto mixChannel = [amount](CS200::RGBA channel)
        {
            const double value = static_cast<double>(channel);
            return static_cast<CS200::RGBA>(std::clamp(value + ((255.0 - value) * amount), 0.0, 255.0));
        };

        const CS200::RGBA r = mixChannel((baseColor & 0xFF000000u) >> 24);
        const CS200::RGBA g = mixChannel((baseColor & 0x00FF0000u) >> 16);
        const CS200::RGBA b = mixChannel((baseColor & 0x0000FF00u) >> 8);
        const CS200::RGBA a = static_cast<CS200::RGBA>(std::clamp(alpha, 0.0, 1.0) * 255.0);

        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    void DrawGlowCircle(CS200::IRenderer2D& renderer, Math::vec2 center, CS200::RGBA glowColor, double diameter, double alpha)
    {
        if (diameter <= 0.0 || alpha <= 0.0)
        {
            return;
        }

        const auto transform = Math::TranslationMatrix(center) * Math::ScaleMatrix({ diameter, diameter });
        renderer.DrawCircle(transform, WithAlpha(glowColor, alpha), CS200::CLEAR, 0.0);
    }

    void DrawLaserFlare(CS200::IRenderer2D& renderer, Math::vec2 center, Math::vec2 direction, CS200::RGBA glowColor, double pulse)
    {
        const double strength = std::clamp(pulse, 0.0, 1.0);
        DrawGlowCircle(renderer, center, glowColor, 36.0 * strength, 0.14 * strength);
        DrawGlowCircle(renderer, center, glowColor, 18.0 * strength, 0.30 * strength);
        DrawGlowCircle(renderer, center, CS200::WHITE, 6.0 * strength, 0.80 * strength);

        Math::vec2 tangent = direction.Normalize();
        if (tangent.LengthSquared() <= 0.01)
        {
            tangent = { 1.0, 0.0 };
        }
        const Math::vec2 normal = { -tangent.y, tangent.x };
        renderer.DrawLine(center - tangent * (14.0 * strength), center + tangent * (14.0 * strength), WithAlpha(CS200::WHITE, 0.35 * strength), 2.0);
        renderer.DrawLine(center - normal * (9.0 * strength), center + normal * (9.0 * strength), WithAlpha(glowColor, 0.30 * strength), 2.0);
    }
}

Laser::Laser(Math::vec2 in_startPos, Math::vec2 dir, Player* in_player) : CS230::GameObject(in_startPos), startPos(in_startPos), direction(dir.Normalize()), player(in_player), color(CS200::WHITE)
{
}

void Laser::CalculatePath(int maxBounces, double maxLength)
{
    std::vector<Physics::LineSegment> allSegments;

    if (player != nullptr)
    {
        Shield* shield = player->GetShield();

        if (shield && shield->IsGuardUp() && IsBlockedByShield())
        {
            auto segs = shield->GetSegments();

            if (!segs.empty())
            {
                Math::vec2 p1      = segs[0].first;
                Math::vec2 p2      = segs[0].second;
                Math::vec2 wallVec = p2 - p1;
                Math::vec2 normal  = Math::vec2{ -wallVec.y, wallVec.x }.Normalize();

                Math::vec2 shieldCenter = { (p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5 };
                if ((shieldCenter - player->GetPosition()).Dot(normal) < 0)
                {
                    normal = -normal;
                }

                if (direction.Dot(normal) < 0)
                {
                    for (const auto& s : segs)
                        allSegments.push_back({ s.first, s.second, true });
                }
            }
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
    auto gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();
    if (gom == nullptr)
        return;

    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        Math::vec2 p1 = pathPoints[i];
        Math::vec2 p2 = pathPoints[i + 1];

        for (auto obj : gom->GetObjects())
        {
            if (obj->Type() == GameObjectTypes::Target)
            {
                TargetStar* target = static_cast<TargetStar*>(obj);
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
}

void Laser::Draw(const Math::TransformationMatrix& camera_matrix)
{
    if (!isActive)
        return;

    auto& renderer = Engine::GetRenderer2D();

    if (pathPoints.size() < 2)
        return;

    const double time = Engine::GetWindowEnvironment().ElapsedTime;

    for (size_t i = 0; i < pathPoints.size() - 1; ++i)
    {
        Math::vec2 start = pathPoints[i];
        Math::vec2 end   = pathPoints[i + 1];
        Math::vec2 beam  = end - start;

        if (beam.LengthSquared() <= 0.01)
        {
            continue;
        }

        Math::vec2 beamDir = beam.Normalize();
        Math::vec2 normal  = { -beamDir.y, beamDir.x };

        const double pulse       = 0.88 + (std::sin(time * 28.0 + static_cast<double>(i) * 1.73) * 0.12);
        const double shimmer     = std::sin(time * 45.0 + beam.Length() * 0.018 + static_cast<double>(i) * 2.11);
        const double heatWobble  = std::sin(time * 21.0 + start.x * 0.013 + start.y * 0.009);
        const double sideOffset  = 1.8 + (heatWobble * 1.2);
        const double coreWidth   = 2.8 + (pulse * 1.2);
        const double beamWidth   = 9.0 + (pulse * 2.5);
        const double glowWidth   = 28.0 + (pulse * 8.0);
        const auto   warmColor   = MixWithWhite(color, 0.28, 1.0);
        const auto   brightColor = MixWithWhite(color, 0.68, 1.0);

        renderer.DrawLine(start, end, WithAlpha(color, 0.10 * pulse), glowWidth);
        renderer.DrawLine(start, end, WithAlpha(color, 0.18 * pulse), 20.0 + (pulse * 4.0));
        renderer.DrawLine(start, end, WithAlpha(warmColor, 0.55 * pulse), beamWidth);
        renderer.DrawLine(start + normal * sideOffset, end + normal * sideOffset, WithAlpha(brightColor, 0.38 + (shimmer * 0.08)), 2.2);
        renderer.DrawLine(start - normal * (sideOffset * 0.6), end - normal * (sideOffset * 0.6), WithAlpha(color, 0.24 - (shimmer * 0.05)), 1.6);
        renderer.DrawLine(start, end, WithAlpha(CS200::WHITE, 0.88), coreWidth);
        renderer.DrawLine(start, end, WithAlpha(CS200::WHITE, 0.62), 1.2);
    }

    DrawLaserFlare(renderer, pathPoints.front(), direction, color, 0.95);

    for (size_t i = 1; i < pathPoints.size(); ++i)
    {
        const bool isLastPoint = i == pathPoints.size() - 1;
        const double pulse     = 0.65 + (std::sin(time * 24.0 + static_cast<double>(i) * 1.91) * 0.12);

        if (!isLastPoint)
        {
            const Math::vec2 incoming = (pathPoints[i] - pathPoints[i - 1]).Normalize();
            const Math::vec2 outgoing = (pathPoints[i + 1] - pathPoints[i]).Normalize();
            DrawLaserFlare(renderer, pathPoints[i], (incoming + outgoing).Normalize(), color, pulse);
        }
        else
        {
            DrawGlowCircle(renderer, pathPoints[i], color, 16.0, 0.10 * pulse);
            DrawGlowCircle(renderer, pathPoints[i], CS200::WHITE, 4.0, 0.45 * pulse);
        }
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
