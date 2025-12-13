#include "Star.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapElement.h"
#include "Engine/Physics/Reflection.hpp"
#include "Engine/ShowCollision.hpp"
#include "Gate.hpp"
#include "Mirror.hpp"
#include "Player.hpp"
#include "PushableMirror.hpp"
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "YellowLaser.hpp"
#include <imgui.h>

Star::Star(Math::vec2 position, Player* targetPlayer, const std::vector<TargetStar*>& destStars, StarType type)
    : CS230::GameObject(position), player(targetPlayer), targets(destStars), starType(type), currentState(State::Idle), timer(0.0)
{
    if (starType == StarType::Red)
        color = 0xFF0000FF;
    else
        color = 0xFFFF00FF;
}

void Star::Update(double dt)
{
    if (player == nullptr)
        return;

    Math::vec2 myPos     = GetPosition();
    Math::vec2 playerPos = player->GetPosition();
    double     distance  = (playerPos - myPos).Length();

    switch (currentState)
    {
        case State::Idle:
            if (starType == StarType::Yellow)
            {
                if (distance > chaseRadius)
                {
                    currentState = State::Idle;
                    timer        = 0.0;
                    Engine::GetLogger().LogEvent("Player escaped Yellow Star warning range!");
                    return;
                }
            }

            if (distance <= detectionRadius)
            {
                currentState = State::Warning;
                timer        = warningDuration;
                Engine::GetLogger().LogEvent("Star detected player! Warning started.");
            }
            break;

        case State::Warning:
            if (distance > chaseRadius)
            {
                currentState = State::Idle;
                timer        = 0.0;

                if (starType == StarType::Red)
                {
                    if (player->GetShield())
                    {
                        player->GetShield()->SetParryWindowActive(false);
                    }
                }
                return;
            }

            if (starType == StarType::Red)
            {
                Shield* shield = player->GetShield();
                if (shield)
                {
                    if (timer <= parryWindowTime)
                        shield->SetParryWindowActive(true);
                    else
                        shield->SetParryWindowActive(false);
                }
            }

            timer -= dt;
            if (timer <= 0.0)
            {
                Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();

                if (starType == StarType::Yellow)
                {
                    YellowLaser* laser = new YellowLaser(GetPosition(), dir, player, targets);
                    Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>()->Add(laser);
                }
                else
                {
                    if (player->GetShield())
                        player->GetShield()->SetParryWindowActive(false);

                    RedLaser* laser = new RedLaser(GetPosition(), dir, player, targets);
                    Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>()->Add(laser);
                }

                Engine::GetLogger().LogEvent("Star fired Laser!");
                currentState = State::Cooldown;
                timer        = cooldownDuration;
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
    CS230::GameObject::Update(dt);
}

void Star::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
{
    auto& renderer = Engine::GetRenderer2D();

    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix({ 40.0, 40.0 });
    renderer.DrawCircle(transform, color);

    auto showCollision = Engine::GetGameStateManager().GetGSComponent<CS230::ShowCollision>();
    bool isDebug       = (showCollision && showCollision->Enabled());

    if (isDebug)
    {
        Math::TransformationMatrix detTransform = Math::TranslationMatrix(GetPosition()) * Math::ScaleMatrix({ detectionRadius * 2.0, detectionRadius * 2.0 });
        renderer.DrawCircle(detTransform, CS200::CLEAR, 0xFF0000FF, 1.5);

        Math::TransformationMatrix chaseTransform = Math::TranslationMatrix(GetPosition()) * Math::ScaleMatrix({ chaseRadius * 2.0, chaseRadius * 2.0 });
        renderer.DrawCircle(chaseTransform, CS200::CLEAR, 0xFFFFFFFF, 1.5);
    }
    else
    {
        Math::TransformationMatrix rangeTransform = Math::TranslationMatrix(GetPosition()) * Math::ScaleMatrix({ detectionRadius * 2.0, detectionRadius * 2.0 });
        renderer.DrawCircle(rangeTransform, CS200::CLEAR, 0x808080FF, 1.0);
    }

    if (currentState == State::Warning && player != nullptr)
    {
        CS200::RGBA lineColor;
        if (starType == StarType::Red)
        {
            lineColor = (timer <= 0.5) ? 0x00FFFF80 : 0xFF000080;
        }
        else
        {
            lineColor = 0xFFFF0080;
        }

        std::vector<Physics::LineSegment> allSegments;

        Shield* shield = player->GetShield();
        if (shield && shield->IsGuardUp())
        {
            auto segments = shield->GetSegments();
            for (const auto& seg : segments)
            {
                allSegments.push_back({ seg.first, seg.second, true });
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
                    auto mirror = static_cast<PushableMirror*>(obj);
                    auto segs   = mirror->GetSegments();
                    for (const auto& s : segs)
                        allSegments.push_back({ s.first, s.second, true });
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

        Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();

        auto path = Physics::CalculateLaserPath(GetPosition(), dir, allSegments, 2, 15000.0);

        for (const auto& seg : path)
        {
            renderer.DrawLine(seg.first, seg.second, lineColor, 2.0);
        }
    }
}

void Star::DrawImGui()
{
    ImGui::PushID(this);
    if (ImGui::TreeNode("Star (Enemy)"))
    {
        const char* stateName = "Unknown";
        switch (currentState)
        {
            case State::Idle: stateName = "Idle"; break;
            case State::Warning: stateName = "Warning"; break;
            case State::Cooldown: stateName = "Cooldown"; break;
        }
        ImGui::Text("Current State: %s", stateName);
        ImGui::Text("State Timer: %.2f", timer);

        ImGui::Text("Type: %s", starType == StarType::Yellow ? "Yellow" : "Red");

        Math::vec2 pos  = GetPosition();
        float      p[2] = { static_cast<float>(pos.x), static_cast<float>(pos.y) };
        if (ImGui::DragFloat2("Position", p))
        {
            SetPosition({ p[0], p[1] });
        }

        ImGui::TreePop();
    }
    ImGui::PopID();
}