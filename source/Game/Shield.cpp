#include "Game/Shield.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Window.hpp"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <stdexcept>

namespace
{
    template <typename FLOAT = double>
    void ease_color_to_target(std::array<float, 4>& current, const std::array<float, 4>& target, FLOAT delta_time, FLOAT weight = 1.0)
    {
        auto subtract = [](const std::array<float, 4>& a, const std::array<float, 4>& b) -> std::array<float, 4>
        {
            return { a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3] };
        };
        auto multiply = [](FLOAT scalar, const std::array<float, 4>& arr) -> std::array<float, 4>
        {
            const auto s = static_cast<float>(scalar);
            return { s * arr[0], s * arr[1], s * arr[2], s * arr[3] };
        };
        auto add_assign = [&](std::array<float, 4>& a, const std::array<float, 4>& b) -> std::array<float, 4>&
        {
            a[0] += b[0];
            a[1] += b[1];
            a[2] += b[2];
            a[3] += b[3];
            return a;
        };

        const auto easing = std::min(delta_time * weight, static_cast<FLOAT>(1.0));
        add_assign(current, multiply(easing, subtract(target, current)));
    }
}

Shield::Shield(CS230::GameObject* owner) : owner(owner), shieldHitTimer(shieldColorRecoveryTime)
{
    if (owner == nullptr)
    {
        throw std::runtime_error("Shield component must have a valid owner.");
    }

    currentShieldColor = CS200::unpack_color(CS200::CYAN);
    targetShieldColor  = CS200::unpack_color(CS200::CYAN);

    UpdatePosition();
}

void Shield::HandleInput([[maybe_unused]] double dt)
{
    if (isShieldFrozen)
        return;

    auto& input = Engine::GetInput();

    Math::vec2  mouseScreenPos = input.GetMousePosition();
    Math::ivec2 winSize        = Engine::GetWindow().GetSize();

    Math::vec2 mouseGLPos = { mouseScreenPos.x, static_cast<double>(winSize.y) - mouseScreenPos.y };

    auto       camera           = Engine::GetGameStateManager().GetGSComponent<CS230::Camera>();
    Math::vec2 cameraBottomLeft = camera ? camera->GetPosition() : Math::vec2{ 0, 0 };

    Math::vec2 mouseWorldPos = cameraBottomLeft + mouseGLPos;

    Math::vec2 dir = mouseWorldPos - owner->GetPosition();
    shieldAngle    = std::atan2(dir.y, dir.x);

    bool rightClick = input.MouseButtonDown(CS230::Input::MouseButton::Right);

    if (rightClick)
    {
        if (cooldownTimer <= 0.0 && !isShieldFrozen)
        {
            isGuarding = true;
        }
    }
    else
    {
        if (isGuarding)
        {
            isGuarding    = false;
            cooldownTimer = shieldCooldown;
            Engine::GetLogger().LogDebug("Shield Lowered. Cooldown started.");
        }
    }

    if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Right))
    {
        if (parryWindowActive)
        {
            TryParry();
        }
    }
}

void Shield::TryParry()
{
    if (parryWindowActive && !isShieldFrozen)
    {
        isParrying = true;
        Engine::GetLogger().LogEvent("Parry Input Success!");
    }
}

bool Shield::ConsumeParryState()
{
    if (isParrying)
    {
        isParrying = false;
        return true;
    }
    return false;
}

bool Shield::IsGuardUp() const
{
    return isGuarding && !isShieldFrozen;
}

void Shield::Update(double dt)
{
    UpdatePosition();

    if (cooldownTimer > 0.0)
    {
        cooldownTimer -= dt;
        if (cooldownTimer < 0.0)
            cooldownTimer = 0.0;
    }

    if (isShieldFrozen)
    {
        shieldFrozenTimer += dt;
        if (shieldFrozenTimer >= shieldFreezeDuration)
        {
            isShieldFrozen    = false;
            shieldFrozenTimer = 0.0;
            isGuarding        = false;
            Engine::GetLogger().LogEvent("Shield Unfrozen.");
        }
    }

    if (isShieldFrozen)
    {
        isGuarding = false;
    }

    UpdateShieldColor(dt);

    if (!parryWindowActive)
    {
        isParrying = false;
    }
}

void Shield::UpdatePosition()
{
    prevShieldStart = shieldStart;
    prevShieldEnd   = shieldEnd;

    const Math::vec2 ownerPos = owner->GetPosition();

    double dirX = std::cos(shieldAngle);
    double dirY = std::sin(shieldAngle);

    shieldCenter = ownerPos + Math::vec2{ dirX * orbitRadius, dirY * orbitRadius };

    double tanX = -dirY;
    double tanY = dirX;

    Math::vec2 halfLenVec = Math::vec2{ tanX, tanY } * (shieldLength / 2.0);

    shieldStart = shieldCenter + halfLenVec;
    shieldEnd   = shieldCenter - halfLenVec;
}

void Shield::Draw(CS200::IRenderer2D& renderer, [[maybe_unused]] const Math::TransformationMatrix& camera_matrix) const
{
    if (isShieldFrozen || IsGuardUp())
    {
        renderer.DrawLine(shieldStart, shieldEnd, shieldColor, 3.0);
    }
}

void Shield::HandleHit(bool parrySuccess)
{
    if (parrySuccess)
    {
        isShieldFrozen    = false;
        shieldFrozenTimer = 0.0;
        isGuarding        = false;
        cooldownTimer     = 0.0;

        Engine::GetLogger().LogEvent("Perfect Parry! Shield Ready.");
    }
    else
    {
        targetShieldColor = CS200::unpack_color(CS200::RED);
        shieldHitTimer    = 0.0;
        Engine::GetLogger().LogEvent("Shield hit by RED laser!");
    }
}

std::vector<std::pair<Math::vec2, Math::vec2>> Shield::GetSegments() const
{
    std::vector<std::pair<Math::vec2, Math::vec2>> segments;

    segments.push_back({ shieldStart, shieldEnd });
    segments.push_back({ prevShieldStart, prevShieldEnd });

    Math::vec2 midStart = (shieldStart + prevShieldStart) * 0.5;
    Math::vec2 midEnd   = (shieldEnd + prevShieldEnd) * 0.5;
    segments.push_back({ midStart, midEnd });

    return segments;
}

void Shield::UpdateShieldColor(double dt)
{
    bool isTargetRed = (targetShieldColor[0] > 0.9f && targetShieldColor[1] < 0.1f && targetShieldColor[2] < 0.1f);

    if (isTargetRed)
    {
        shieldHitTimer += dt;
        if (shieldHitTimer >= shieldColorRecoveryTime)
        {
            targetShieldColor = CS200::unpack_color(CS200::CYAN);
        }
    }
    else
    {
        shieldHitTimer = shieldColorRecoveryTime;
        if (!(targetShieldColor[0] < 0.1f && targetShieldColor[1] > 0.9f && targetShieldColor[2] > 0.9f))
        {
            targetShieldColor = CS200::unpack_color(CS200::CYAN);
        }
    }
    ease_color_to_target(currentShieldColor, targetShieldColor, dt, 5.0);
    shieldColor = CS200::pack_color(currentShieldColor);
}

void Shield::DrawImGui()
{
    if (ImGui::TreeNode("Shield Component"))
    {
        ImGui::Text("State Info:");
        bool isGuardDown = Engine::GetInput().MouseButtonDown(CS230::Input::MouseButton::Right);
        ImGui::Checkbox("Is Guard Up", &isGuardDown);
        ImGui::Checkbox("Is Frozen", &isShieldFrozen);
        ImGui::Checkbox("Parry Window Active", &parryWindowActive);
        ImGui::Checkbox("Is Parrying", &isParrying);

        ImGui::Separator();
        ImGui::Text("Timers:");
        ImGui::Text("Cooldown: %.2f / %.2f", static_cast<float>(cooldownTimer), static_cast<float>(shieldCooldown));
        ImGui::Text("Frozen Timer: %.2f / %.2f", static_cast<float>(shieldFrozenTimer), static_cast<float>(shieldFreezeDuration));
        ImGui::Text("Hit Timer: %.2f / %.2f", static_cast<float>(shieldHitTimer), static_cast<float>(shieldColorRecoveryTime));

        ImGui::Separator();
        ImGui::Text("Transform:");
        float angleDeg = static_cast<float>(shieldAngle * 180.0 / PI);
        ImGui::Text("Angle: %.1f deg", angleDeg);

        float len = static_cast<float>(shieldLength);
        if (ImGui::DragFloat("Length", &len, 1.0f, 10.0f, 500.0f))
        {
            shieldLength = static_cast<double>(len);
        }

        float rad = static_cast<float>(orbitRadius);
        if (ImGui::DragFloat("Orbit Radius", &rad, 1.0f, 10.0f, 300.0f))
        {
            orbitRadius = static_cast<double>(rad);
        }

        ImGui::Separator();
        ImGui::Text("Color:");
        ImGui::ColorEdit4("Current Color", currentShieldColor.data());

        ImGui::TreePop();
    }
}