#include "DemoReflection.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "CS200/RenderingAPI.hpp"
#include "DemoAstar.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Physics/Reflection.hpp"
#include "Engine/Window.hpp"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <limits>
#include <numbers>
#include <string>

namespace
{

    inline double dot(const Math::vec2& a, const Math::vec2& b)
    {
        return a.x * b.x + a.y * b.y;
    }

    inline Math::vec2 perpendicular(const Math::vec2& v)
    {
        return Math::vec2{ -v.y, v.x };
    }

    bool LineCircleIntersection(Math::vec2 p1, Math::vec2 p2, Math::vec2 center, double radius, Math::vec2& intersection)
    {
        Math::vec2 d = p2 - p1;
        Math::vec2 f = p1 - center;
        double     a = dot(d, d);
        double     b = 2.0 * dot(f, d);
        double     c = dot(f, f) - radius * radius;

        double discriminant = b * b - 4.0 * a * c;
        if (discriminant < 0)
        {
            return false;
        }
        else
        {
            discriminant       = std::sqrt(discriminant);
            double t1          = (-b - discriminant) / (2.0 * a);
            double t2          = (-b + discriminant) / (2.0 * a);
            bool   intersected = false;

            if (t1 >= -std::numeric_limits<double>::epsilon() && t1 <= 1.0 + std::numeric_limits<double>::epsilon())
            {
                intersection = p1 + d * t1;
                intersected  = true;
            }
            if (t2 >= -std::numeric_limits<double>::epsilon() && t2 <= 1.0 + std::numeric_limits<double>::epsilon())
            {
                if (!intersected)
                {
                    intersection = p1 + d * t2;
                }
                intersected = true;
            }
            return intersected;
        }
    }

    CS200::RGBA LerpColor(CS200::RGBA start, CS200::RGBA end, double t)
    {
        t                      = std::clamp(t, 0.0, 1.0);
        auto                 s = CS200::unpack_color(start);
        auto                 e = CS200::unpack_color(end);
        std::array<float, 4> result;
        for (int i = 0; i < 4; ++i)
        {
            result[i] = s[i] + static_cast<float>(t) * (e[i] - s[i]);
        }
        return CS200::pack_color(result);
    }

    std::array<float, 4> operator-(const std::array<float, 4>& a, const std::array<float, 4>& b)
    {
        return { a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3] };
    }

    std::array<float, 4> operator*(double scalar, const std::array<float, 4>& arr)
    {
        const auto s = static_cast<float>(scalar);
        return { s * arr[0], s * arr[1], s * arr[2], s * arr[3] };
    }

    std::array<float, 4>& operator+=(std::array<float, 4>& a, const std::array<float, 4>& b)
    {
        a[0] += b[0];
        a[1] += b[1];
        a[2] += b[2];
        a[3] += b[3];
        return a;
    }

    template <typename FLOAT = double>
    void ease_color_to_target(std::array<float, 4>& current, const std::array<float, 4>& target, FLOAT delta_time, FLOAT weight = 1.0)
    {
        const auto easing = std::min(delta_time * weight, static_cast<FLOAT>(1.0));
        current += easing * (target - current);
    }

}

void DemoLaserReflection::Load()
{
    CS200::RenderingAPI::SetClearColor(COLOR_BLACK);
    const auto windowSize = Engine::GetWindow().GetSize();
    characterPos          = { windowSize.x / 2.0, windowSize.y / 2.0 };
    laserOrigin           = { 0.0, static_cast<double>(windowSize.y) };
    shieldAngle           = PI / 2.0;
    isLaserOn             = false;
    laserTimer            = 0.0;
    laserColor            = COLOR_RED;
    showingWarningLaser   = false;
    warningLaserColor     = COLOR_WARNING;
    isParrying            = false;
    parryWindowActive     = false;
    parryTimer            = 0.0;
    shieldColor           = COLOR_CYAN;
    currentShieldColor    = CS200::unpack_color(COLOR_CYAN);
    targetShieldColor     = CS200::unpack_color(COLOR_CYAN);
    shieldHitTimer        = shieldColorRecoveryTime;
    isShieldFrozen        = false;
    shieldFrozenTimer     = 0.0;

    targets.clear();

    targets.push_back({});
    targets.back().position = { windowSize.x - 50.0, windowSize.y - 50.0 };

    targets.push_back({});
    targets.back().position = { 50.0, 50.0 };

    UpdateShield();
}

void DemoLaserReflection::Unload()
{
}

gsl::czstring DemoLaserReflection::GetName() const
{
    return "Demo: Laser Reflection Parry";
}

void DemoLaserReflection::Update()
{
    const auto&  input = Engine::GetInput();
    const double dt    = Engine::GetWindowEnvironment().DeltaTime;

    laserTimer += dt;
    double cycleTime = fmod(laserTimer, laserCycleTime);

    bool prevLaserState = isLaserOn;

    showingWarningLaser = !isLaserOn && (cycleTime >= laserCycleTime - warningLaserLeadTime);
    isLaserOn           = (cycleTime < laserOnDuration);

    parryWindowActive = false;
    if (showingWarningLaser)
    {
        double timeIntoWarning = cycleTime - (laserCycleTime - warningLaserLeadTime);

        if (timeIntoWarning >= parryWindowStartTimeOffset)
        {
            parryWindowActive = true;
            warningLaserColor = COLOR_PARRY_WARNING;
            parryTimer += dt;
            if (input.KeyJustPressed(CS230::Input::Keys::Space))
            {
                isParrying = true;
                Engine::GetLogger().LogEvent("Parry Input Success!");
            }
        }
        else
        {
            warningLaserColor = COLOR_WARNING;
            parryTimer        = 0.0;
        }
    }
    else
    {
        parryTimer        = 0.0;
        warningLaserColor = COLOR_WARNING;
    }

    if (isShieldFrozen)
    {
        shieldFrozenTimer += dt;
        if (shieldFrozenTimer >= shieldFreezeDuration)
        {
            isShieldFrozen    = false;
            shieldFrozenTimer = 0.0;
            Engine::GetLogger().LogEvent("Shield Unfrozen.");
        }
    }

    if (isLaserOn)
    {
        if (prevLaserState == false)
        {
            if (isParrying)
            {
                laserColor = COLOR_YELLOW;
                Engine::GetLogger().LogEvent("Laser turned YELLOW (Parry)");
                isShieldFrozen    = true;
                shieldFrozenTimer = 0.0;
                Engine::GetLogger().LogEvent("Shield Frozen!");
            }
            else
            {
                laserColor = COLOR_RED;
                Engine::GetLogger().LogEvent("Laser turned RED (Parry Failed)");
                if (CheckShieldCollision())
                {
                    targetShieldColor = CS200::unpack_color(COLOR_RED);
                    shieldHitTimer    = 0.0;
                    Engine::GetLogger().LogEvent("Shield hit by RED laser (Initial)!");
                }
            }
            isParrying = false;
        }
        else if (laserColor == COLOR_RED)
        {
            if (CheckShieldCollision())
            {
                if (shieldHitTimer >= shieldColorRecoveryTime || !(targetShieldColor[0] > 0.9f && targetShieldColor[1] < 0.1f && targetShieldColor[2] < 0.1f))
                {
                    targetShieldColor = CS200::unpack_color(COLOR_RED);
                    shieldHitTimer    = 0.0;
                }
            }
        }
    }
    else
    {
        if (prevLaserState == true)
        {
            laserColor = COLOR_RED;
            Engine::GetLogger().LogEvent("Laser turned OFF");
        }

        if (showingWarningLaser)
        {
            std::vector<std::pair<Math::vec2, Math::vec2>> reflectionSegments;
            reflectionSegments.push_back({ shieldStart, shieldEnd });

            const auto windowSize = Engine::GetWindow().GetSize();
            Math::vec2 center     = { windowSize.x / 2.0, windowSize.y / 2.0 };
            Math::vec2 initialDir = (center - laserOrigin).Normalize();
            if (initialDir.Length() < std::numeric_limits<double>::epsilon())
            {
                initialDir = { 1.0, -1.0 };
                initialDir = initialDir.Normalize();
            }

            warningLaserPath = Physics::CalculateLaserPath(laserOrigin, initialDir, reflectionSegments);
            laserPath.clear();
        }
        else
        {
            warningLaserPath.clear();
        }
    }

    UpdateShieldColor(dt);

    if (!isShieldFrozen)
    {
        const double moveSpeed = 200.0;
        Math::vec2   moveDir{ 0.0, 0.0 };
        if (input.KeyDown(CS230::Input::Keys::W))
            moveDir.y += 1.0;
        if (input.KeyDown(CS230::Input::Keys::S))
            moveDir.y -= 1.0;
        if (input.KeyDown(CS230::Input::Keys::A))
            moveDir.x -= 1.0;
        if (input.KeyDown(CS230::Input::Keys::D))
            moveDir.x += 1.0;

        if (moveDir.Length() > std::numeric_limits<double>::epsilon())
        {
            Math::vec2 nextPos    = characterPos + moveDir.Normalize() * moveSpeed * dt;
            const auto windowSize = Engine::GetWindow().GetSize();
            nextPos.x             = std::clamp(nextPos.x, 0.0, static_cast<double>(windowSize.x));
            nextPos.y             = std::clamp(nextPos.y, 0.0, static_cast<double>(windowSize.y));
            characterPos          = nextPos;
        }

        const double rotateSpeed = PI / 2.0;
        if (input.KeyDown(CS230::Input::Keys::Left))
            shieldAngle += rotateSpeed * dt;
        if (input.KeyDown(CS230::Input::Keys::Right))
            shieldAngle -= rotateSpeed * dt;
        shieldAngle = fmod(shieldAngle, 2.0 * PI);
        if (shieldAngle < 0)
            shieldAngle += 2.0 * PI;
    }

    UpdateShield();
    if (isLaserOn)
    {
        CalculateLaser(laserColor == COLOR_YELLOW);
        if (laserColor == COLOR_YELLOW)
        {
            for (TargetState& target : targets)
            {
                if (!target.hitByParriedLaser && CheckCollision(target))
                {
                    target.color             = COLOR_GREEN;
                    target.hitByParriedLaser = true;
                    Engine::GetLogger().LogEvent("Target hit by parried laser!");
                }
            }
        }
    }
    else if (!showingWarningLaser)
    {
        laserPath.clear();
    }
}

void DemoLaserReflection::UpdateShieldColor(double dt)
{
    bool isTargetRed = (targetShieldColor[0] > 0.9f && targetShieldColor[1] < 0.1f && targetShieldColor[2] < 0.1f);
    if (isTargetRed)
    {
        shieldHitTimer += dt;
        if (shieldHitTimer >= shieldColorRecoveryTime)
        {
            targetShieldColor = CS200::unpack_color(COLOR_CYAN);
        }
    }
    else
    {
        shieldHitTimer = shieldColorRecoveryTime;
        if (!(targetShieldColor[0] < 0.1f && targetShieldColor[1] > 0.9f && targetShieldColor[2] > 0.9f))
        {
            targetShieldColor = CS200::unpack_color(COLOR_CYAN);
        }
    }
    ease_color_to_target(currentShieldColor, targetShieldColor, dt, 5.0);
    shieldColor = CS200::pack_color(currentShieldColor);
}

void DemoLaserReflection::UpdateShield()
{
    double dx   = (shieldLength / 2.0) * std::cos(shieldAngle);
    double dy   = (shieldLength / 2.0) * std::sin(shieldAngle);
    shieldStart = characterPos + Math::vec2{ dx, dy };
    shieldEnd   = characterPos - Math::vec2{ dx, dy };
}

void DemoLaserReflection::CalculateLaser(bool parrySuccess)
{
    std::vector<std::pair<Math::vec2, Math::vec2>> reflectionSegments;

    if (parrySuccess)
    {
        reflectionSegments.push_back({ shieldStart, shieldEnd });
    }

    const auto windowSize = Engine::GetWindow().GetSize();
    Math::vec2 center     = { windowSize.x / 2.0, windowSize.y / 2.0 };
    Math::vec2 initialDir = (center - laserOrigin).Normalize();

    if (initialDir.Length() < std::numeric_limits<double>::epsilon())
    {
        initialDir = { 1.0, -1.0 };
        initialDir = initialDir.Normalize();
    }

    laserPath = Physics::CalculateLaserPath(laserOrigin, initialDir, reflectionSegments);
}

bool DemoLaserReflection::CheckShieldCollision() const
{
    const auto windowSize = Engine::GetWindow().GetSize();
    Math::vec2 center     = { windowSize.x / 2.0, windowSize.y / 2.0 };
    Math::vec2 initialDir = (center - laserOrigin).Normalize();
    if (initialDir.Length() < std::numeric_limits<double>::epsilon())
    {
        initialDir = { 1.0, -1.0 };
        initialDir = initialDir.Normalize();
    }

    Math::vec2 intersectionPoint;
    double     t;

    return Physics::RaySegmentIntersection(laserOrigin, initialDir, shieldStart, shieldEnd, intersectionPoint, t);
}

bool DemoLaserReflection::CheckCollision(const TargetState& target) const
{
    Math::vec2 intersectionPoint;
    for (const auto& segment : laserPath)
    {
        if (LineCircleIntersection(segment.first, segment.second, target.position, target.radius, intersectionPoint))
        {
            return true;
        }
    }
    return false;
}

void DemoLaserReflection::Draw() const
{
    CS200::RenderingAPI::Clear();
    auto&      renderer   = Engine::GetRenderer2D();
    const auto windowSize = Engine::GetWindow().GetSize();

    renderer.BeginScene(CS200::build_ndc_matrix(windowSize));

    renderer.DrawCircle(Math::TranslationMatrix(characterPos) * Math::ScaleMatrix(10.0), COLOR_WHITE);
    renderer.DrawLine(shieldStart, shieldEnd, shieldColor, 3.0);
    for (const auto& target : targets)
    {
        renderer.DrawCircle(Math::TranslationMatrix(target.position) * Math::ScaleMatrix(target.radius), target.color);
    }

    if (showingWarningLaser)
    {
        for (const auto& segment : warningLaserPath)
        {
            if (std::isfinite(segment.first.x) && std::isfinite(segment.first.y) && std::isfinite(segment.second.x) && std::isfinite(segment.second.y))
            {
                renderer.DrawLine(segment.first, segment.second, warningLaserColor, warningLaserWidth);
            }
        }
    }

    if (isLaserOn)
    {
        for (const auto& segment : laserPath)
        {
            if (std::isfinite(segment.first.x) && std::isfinite(segment.first.y) && std::isfinite(segment.second.x) && std::isfinite(segment.second.y))
            {
                renderer.DrawLine(segment.first, segment.second, laserColor, mainLaserWidth);
            }
        }
    }

    renderer.EndScene();
}

void DemoLaserReflection::DrawImGui()
{
    ImGui::Begin("Laser Reflection Info");

    ImGui::Text("Character Position: (%.1f, %.1f)", characterPos.x, characterPos.y);
    ImGui::Text("Shield Angle (Degrees): %.1f", shieldAngle * 180.0 / PI);
    ImGui::Text("Shield Start: (%.1f, %.1f)", shieldStart.x, shieldStart.y);
    ImGui::Text("Shield End: (%.1f, %.1f)", shieldEnd.x, shieldEnd.y);
    ImVec4 currentShieldColorVec4 = ImVec4(currentShieldColor[0], currentShieldColor[1], currentShieldColor[2], currentShieldColor[3]);
    ImGui::ColorEdit4("Current Shield Color", (float*)&currentShieldColorVec4, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
    ImGui::Text("Shield Frozen: %s (%.1f / %.1f s)", isShieldFrozen ? "YES" : "NO", isShieldFrozen ? shieldFrozenTimer : 0.0f, shieldFreezeDuration);

    ImGui::Separator();

    ImGui::Text("Laser State: %s", isLaserOn ? "ON" : (showingWarningLaser ? "WARNING" : "OFF"));
    ImGui::Text("Laser Origin: (%.1f, %.1f)", laserOrigin.x, laserOrigin.y);
    ImGui::Text("Laser Timer: %.2f / %.2f", fmod(laserTimer, laserCycleTime), laserCycleTime);
    const char* colorName = (laserColor == COLOR_RED) ? "RED" : (laserColor == COLOR_YELLOW ? "YELLOW" : "UNKNOWN");
    ImGui::Text("Laser Color: %s", colorName);

    const char* warnColorName = (warningLaserColor == COLOR_WARNING) ? "RED" : ((warningLaserColor == COLOR_PARRY_WARNING) ? "CYAN" : "UNKNOWN");
    ImGui::Text("Warning Laser Color: %s", showingWarningLaser ? warnColorName : "N/A");
    ImGui::Text("Parry Window: %s", parryWindowActive ? "ACTIVE" : "INACTIVE");
    ImGui::Text("Parrying: %s", isParrying ? "YES (Input success)" : "NO");

    ImGui::SeparatorText("Targets Info");
    for (size_t i = 0; i < targets.size(); ++i)
    {
        const auto& target = targets[i];
        ImGui::PushID(static_cast<int>(i));
        const char* targetColorName = (target.color == COLOR_RED) ? "RED" : (target.color == COLOR_GREEN ? "GREEN" : "UNKNOWN");
        ImGui::Text("Target %zu Position: (%.1f, %.1f)", i + 1, target.position.x, target.position.y);
        ImGui::Text("Target %zu Color: %s", i + 1, targetColorName);
        ImGui::Text("Target %zu Hit: %s", i + 1, target.hitByParriedLaser ? "YES" : "NO");
        ImGui::PopID();
        if (i < targets.size() - 1)
            ImGui::Separator();
    }
    if (ImGui::Button("Reset Targets"))
    {
        for (TargetState& target : targets)
        {
            target.color             = COLOR_RED;
            target.hitByParriedLaser = false;
        }
        Engine::GetLogger().LogEvent("All Targets Reset!");
    }

    ImGui::Separator();
    ImGui::Text("Laser Path Segments: %zu", laserPath.size());
    if (ImGui::TreeNode("Path Details"))
    {
        for (size_t i = 0; i < laserPath.size(); ++i)
        {
            ImGui::Text("Seg %zu: (%.1f, %.1f) -> (%.1f, %.1f)", i, laserPath[i].first.x, laserPath[i].first.y, laserPath[i].second.x, laserPath[i].second.y);
        }
        ImGui::TreePop();
    }
    ImGui::Text("Warning Laser Path Segments: %zu", warningLaserPath.size());
    if (ImGui::TreeNode("Warning Path Details"))
    {
        for (size_t i = 0; i < warningLaserPath.size(); ++i)
        {
            ImGui::Text("Seg %zu: (%.1f, %.1f) -> (%.1f, %.1f)", i, warningLaserPath[i].first.x, warningLaserPath[i].first.y, warningLaserPath[i].second.x, warningLaserPath[i].second.y);
        }
        ImGui::TreePop();
    }

    if (ImGui::Button("Switch to Demo Astar"))
    {
        Engine::GetGameStateManager().PopState();
        Engine::GetGameStateManager().PushState<DemoAstar>();
    }

    ImGui::End();
}