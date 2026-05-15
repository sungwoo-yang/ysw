#pragma once

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

    void ApplyLaserDamage(double damageAmount);

    bool IsDead() const;
    double GetHP() const;

    CS230::DashComponent  dashComponent;
    bool                  isJumping            = false;
    std::optional<size_t> currentPlatformIndex = std::nullopt;
    double                velocityY            = 0.0;
    bool                  faceRight            = true;

    CS230::GameObject* interactionTarget = nullptr;
    bool               isInteracting     = false;

private:
    void HandleInput(double dt);
    void DrawShieldCooldown(CS200::IRenderer2D& renderer) const;

    void UpdateHealthState(double dt);
    void RecordWallContact(int direction);

    const double collisionHalfHeight = 40.0;

    Shield* shieldComponent = nullptr;

    double       gravity      = 1600.0;
    double       jumpStrength = 800.0;
    const double jumpBufferTime = 0.12;
    double       jumpReleaseGravityMultiplier = 4.5;

    const double wallStickDuration           = 0.28;
    const double wallStickFallSpeed          = 30.0;
    const double wallSlideSpeed              = 150.0;
    const double wallJumpHorizontalStrength  = 620.0;
    const double wallJumpVerticalStrength    = 800.0;
    double       wallJumpControlLockDuration = 0.15;

    double       maxRunSpeed        = 550.0;
    double       playerAcceleration = 3000.0;
    double       playerFriction     = 2110.0;
    
    double       currentSpeedMultiplier = 1.0;
    const double shieldSlowdownRate     = 4.0;
    const double minShieldSpeedMult     = 0.3;

    Math::vec2 startPosition;
    Math::vec2 previousPosition;

    double       jumpBufferTimer = 0.0;
    double       coyoteTimer     = 0.0;
    const double coyoteTime      = 0.1;

    bool   isTouchingWall           = false;
    bool   isWallSliding            = false;
    int    wallDirection            = 0;
    double wallContactTimer         = 0.0;
    double wallJumpControlLockTimer = 0.0;
    bool   developerMode            = false;

    enum class HealthState
    {
        Full,      
        Healthy,   
        Hurt,      
        Critical,  
        NearDeath, 
        Dead       
    };
    HealthState healthState = HealthState::Full;
    double      playerHp    = 5.0;
    double      maxPlayerHp = 5.0;

    double       recoverDelayTimer    = 0.0;
    const double recoverDelayDuration = 5.0;
    bool         tookDamageThisFrame  = false;

    double       invincibilityTimer    = 0.0;
    const double invincibilityDuration = 1.0;

    bool wasJumpingLastFrame = false;
};
