#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/Component.hpp"
#include "Engine/Vec2.hpp"
#include <array>
#include <utility>
#include <vector>

namespace CS230
{
    class GameObject;
}

namespace CS200
{
    class IRenderer2D;
}

namespace Math
{
    class TransformationMatrix;
}

constexpr CS200::RGBA COLOR_CYAN  = 0x00FFFFFF;
constexpr CS200::RGBA COLOR_RED   = 0xFF0000FF;
constexpr CS200::RGBA COLOR_WHITE = 0xFFFFFFFF;

class Shield : public CS230::Component
{
public:
    Shield(CS230::GameObject* owner);

    void Update(double dt) override;
    void HandleInput(double dt);
    void Draw(CS200::IRenderer2D& renderer, const Math::TransformationMatrix& camera_matrix) const;
    void DrawImGui();
    void UpdatePosition();
    void HandleHit(bool parrySuccess);

    void SetParryWindowActive(bool active)
    {
        parryWindowActive = active;
    }

    bool IsParryWindowActive() const
    {
        return parryWindowActive;
    }

    void TryParry();
    bool ConsumeParryState();

    bool IsGuardUp() const;

    std::vector<std::pair<Math::vec2, Math::vec2>> GetSegments() const;

    bool         isShieldFrozen       = false;
    double       shieldFrozenTimer    = 0.0;
    const double shieldFreezeDuration = 3.0;

private:
    void UpdateShieldColor(double dt);

    CS230::GameObject* owner;
    double             shieldAngle  = PI / 2.0;
    double             shieldLength = 120.0;
    double             orbitRadius  = 80.0;

    Math::vec2 shieldCenter;
    Math::vec2 shieldStart;
    Math::vec2 shieldEnd;

    Math::vec2 prevShieldStart;
    Math::vec2 prevShieldEnd;

    CS200::RGBA          shieldColor = COLOR_CYAN;
    std::array<float, 4> currentShieldColor{};
    std::array<float, 4> targetShieldColor{};

    double       shieldHitTimer          = 0.0;
    const double shieldColorRecoveryTime = 1.0;

    bool   isGuarding = false;
    double cooldownTimer = 0.0;
    const double shieldCooldown = 1.0;

    bool isParrying        = false;
    bool parryWindowActive = false;
};