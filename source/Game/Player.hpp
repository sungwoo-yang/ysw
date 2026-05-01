#pragma once

// #include "Engine/AnimationEditor.hpp"
#include "Engine/Dash.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class Shield;

namespace CS200
{
    class IRenderer2D;
}

class Player : public CS230::GameObject
{
public:
    Player(Math::vec2 in_start_pos);
    ~Player() override = default;

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;
    void DrawImGui() override;

    const Math::vec2& GetPosition() const
    {
        return GameObject::GetPosition();
    };

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Player;
    }

    std::string TypeName() override
    {
        return "Player";
    }

    Shield* GetShield() const
    {
        return shieldComponent;
    }

    bool CanCollideWith(GameObjectTypes other_object_type) override;
    void ResolveCollision(CS230::GameObject* other_object) override;

    void ResetState();
    void SetSavePoint(Math::vec2 new_spawn_point);

    // Handle laser damage and trigger invincibility frames
    void ApplyLaserDamage(double damageAmount);

    // Core movement and interaction states
    CS230::DashComponent  dashComponent;
    bool                  isJumping            = false;
    std::optional<size_t> currentPlatformIndex = std::nullopt;
    double                velocityY            = 0.0;
    bool                  faceRight            = true;

    CS230::GameObject* interactionTarget = nullptr;
    bool               isInteracting     = false;

private:
    // Process player input for movement, jumping, and actions
    void HandleInput(double dt);

    // Update player health status and handle recovery over time
    void UpdateHealthState(double dt);

    // System components
    Shield* shieldComponent = nullptr;

    // Movement physics constants
    const double gravity      = 1500.0;
    const double jumpStrength = 700.0;
    const double baseSpeed    = 300.0;

    // Shield movement modifiers
    double       currentSpeedMultiplier = 1.0;
    const double shieldSlowdownRate     = 4.0;
    const double minShieldSpeedMult     = 0.3;

    // State tracking for collision resolution
    Math::vec2 startPosition;
    Math::vec2 previousPosition;

    // Platforming assist timers (Game feel improvements)
    double       jumpBufferTimer = 0.0;
    double       coyoteTimer     = 0.0;
    const double coyoteTime      = 0.1;

    // Health and damage system
    enum class HealthState
    {
        Full,      // 5
        Healthy,   // 4
        Hurt,      // 3
        Critical,  // 2
        NearDeath, // 1
        Dead       // 0
    };
    HealthState healthState = HealthState::Full;
    double      playerHp    = 5.0;
    double      maxPlayerHp = 5.0;

    double       recoverDelayTimer    = 0.0;
    const double recoverDelayDuration = 5.0;
    bool         tookDamageThisFrame  = false;

    // I-frames (Invincibility frames) after taking damage
    double       invincibilityTimer    = 0.0;
    const double invincibilityDuration = 1.0;

    // Landing Sounds
    bool wasJumpingLastFrame = false;
};