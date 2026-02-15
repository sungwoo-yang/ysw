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
#include "PushableMirror.hpp"
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "YellowLaser.hpp"

BossStar::BossStar(Math::vec2 in_position, Player* in_player, const std::vector<TargetStar*>& in_targets)
    : CS230::GameObject(in_position), currentState(State::Idle), nextLaserType(NextLaser::Yellow), player(in_player), targets(in_targets), timer(0.0)
{
}

void BossStar::Update(double dt)
{
    if (player == nullptr)
        return;

    double distanceSq = (player->GetPosition() - GetPosition()).LengthSquared();

    switch (currentState)
    {
        case State::Idle:
            // Switch to Warning if player enters detection range
            if (distanceSq <= detectionRadius)
            {
                currentState = State::Warning;
                timer        = warningDuration;
                Engine::GetLogger().LogEvent("Boss Detected Player! Charging Laser...");
            }
            break;

        case State::Warning:
            // Cancel charge if player escapes
            if (distanceSq > detectionRadius * 2.25)
            {
                currentState = State::Idle;
                timer        = 0.0;
                return;
            }

            timer -= dt;
            if (timer <= 0.0)
            {
                // Timer finished: Fire Laser
                Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();
                auto       gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

                // Alternate laser type after each fire
                if (nextLaserType == NextLaser::Red)
                {
                    gom->Add(new RedLaser(GetPosition(), dir, player, targets));
                    nextLaserType = NextLaser::Yellow;
                    Engine::GetLogger().LogEvent("Boss Fired Red Laser!");
                }
                else
                {
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
                if (distanceSq <= detectionRadius)
                {
                    currentState = State::Warning;
                    timer        = warningDuration;
                }
                else
                {
                    currentState = State::Idle;
                }
            }
            break;
    }

    CS230::GameObject::Update(dt);
}

void BossStar::Draw(const Math::TransformationMatrix& camera_matrix)
{
    auto& renderer = Engine::GetRenderer2D();

    CS200::RGBA bossColor;
    if (nextLaserType == NextLaser::Red)
        bossColor = 0xFF0000FF;
    else
        bossColor = 0xFFFF00FF;

    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix({ size, size });
    renderer.DrawCircle(transform, bossColor);

    // Render Trajectory Warning: Calculates laser path including reflections
    if (currentState == State::Warning && player != nullptr)
    {
        CS200::RGBA lineColor = bossColor;
        if (static_cast<int>(timer * 10) % 2 == 0)
            lineColor = CS200::WHITE;
        lineColor = (lineColor & 0xFFFFFF00) | 0x80;

        std::vector<Physics::LineSegment> allSegments;

        // Collect all reflective surfaces (Player Shield, Mirrors, Walls, Closed Gates)
        allSegments.reserve(64);

        Shield* shield = player->GetShield();
        if (shield && shield->IsGuardUp())
        {
            auto segs = shield->GetSegments();
            for (auto& s : segs)
                allSegments.push_back({ s.first, s.second, true });
        }

        auto gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();
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