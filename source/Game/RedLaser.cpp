#include "RedLaser.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapElement.h"
#include "Engine/Physics/Reflection.hpp"
#include "Gate.hpp"
#include "Mirror.hpp"
#include "Player.hpp"
#include "Shield.hpp"
#include "TargetStar.hpp"
#include <algorithm>

static double DistToSegmentSquared(Math::vec2 p, Math::vec2 v, Math::vec2 w)
{
    double l2 = (w - v).Dot(w - v);
    if (l2 == 0.0)
        return (p - v).Dot(p - v);
    double t              = ((p - v).Dot(w - v)) / l2;
    t                     = std::max(0.0, std::min(1.0, t));
    Math::vec2 projection = v + (w - v) * t;
    return (p - projection).Dot(p - projection);
}

RedLaser::RedLaser(Math::vec2 startPos, Math::vec2 direction, Player* player, const std::vector<TargetStar*>& targets)
    : CS230::GameObject(startPos), start(startPos), dir(direction.Normalize()), player(player), targets(targets)
{
}

void RedLaser::Update(double dt)
{
    lifeTime -= dt;
    if (lifeTime <= 0.0)
    {
        Destroy();
        return;
    }

    if (!isCalculated)
    {
        isCalculated = true;

        std::vector<Physics::LineSegment> allSegments;

        if (player != nullptr)
        {
            Shield* shield = player->GetShield();
            if (shield && shield->ConsumeParryState())
            {
                auto segments = shield->GetSegments();
                if (!segments.empty())
                {
                    Math::vec2 segStart = segments[0].first;
                    Math::vec2 segEnd   = segments[0].second;

                    Math::vec2 wallVec = segEnd - segStart;
                    Math::vec2 normal  = Math::vec2{ -wallVec.y, wallVec.x }.Normalize();

                    Math::vec2 playerToShield = ((segStart + segEnd) * 0.5) - player->GetPosition();
                    if (Math::dot(playerToShield, normal) < 0)
                    {
                        normal = -normal;
                    }

                    if (Math::dot(dir, normal) < 0)
                    {
                        allSegments.push_back({ segStart, segEnd, true });

                        shield->HandleHit(true);
                        Engine::GetLogger().LogEvent("Perfect Parry Success!");
                    }
                    else
                    {
                        Engine::GetLogger().LogEvent("Parry Failed: Wrong Direction!");
                    }
                }
            }
        }

        auto& objects = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>()->GetObjects();
        for (auto obj : objects)
        {
            if (obj->Type() == GameObjectTypes::Mirror)
            {
                allSegments.push_back(static_cast<Mirror*>(obj)->GetReflectiveSegment());
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

        auto path = Physics::CalculateLaserPath(start, dir, allSegments, 5);

        bool hitSomething = false;

        for (const auto& seg : path)
        {
            Math::vec2  p1        = seg.first;
            Math::vec2  p2        = seg.second;
            CS200::RGBA beamColor = 0xFF0000FF;

            for (TargetStar* target : targets)
            {
                if (target == nullptr)
                    continue;
                double r2 = target->GetRadius() * target->GetRadius();

                if (DistToSegmentSquared(target->GetPosition(), p1, p2) <= r2)
                {
                    target->OnHit();
                    beams.push_back({ p1, target->GetPosition(), beamColor });
                    hitSomething = true;
                    Engine::GetLogger().LogEvent("Red Laser Hit Target!");
                    break;
                }
            }
            if (hitSomething)
                break;

            if (player != nullptr)
            {
                double playerR2 = 40.0 * 40.0;
                if (DistToSegmentSquared(player->GetPosition(), p1, p2) <= playerR2)
                {
                    player->ApplyLaserDamage(4.0);

                    beams.push_back({ p1, player->GetPosition(), beamColor });
                    hitSomething = true;
                    Engine::GetLogger().LogEvent("Player Hit by Red Laser!");
                    break;
                }
            }
            if (hitSomething)
                break;

            beams.push_back({ p1, p2, beamColor });
        }
    }
}

void RedLaser::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
{
    auto& renderer = Engine::GetRenderer2D();
    for (const auto& beam : beams)
    {
        renderer.DrawLine(beam.p1, beam.p2, beam.color, 15.0);
    }
}