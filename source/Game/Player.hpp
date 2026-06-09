#pragma once

#include "Engine/Dash.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Polygon.h"
#include "Engine/Rect.hpp"
#include "Engine/Vec2.hpp"
#include "Game/OriAnim.hpp"
#include "Game/TutorialScript.hpp"

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

    void       ResetState();
    void       SetSavePoint(Math::vec2 new_spawn_point);
    Math::vec2 GetSavePoint() const { return startPosition; }

    void ApplyLaserDamage(double damageAmount);
    void ApplyBashImpulse(Math::vec2 impulse);
    void ApplyWaterRush(Math::vec2 dir, Math::vec2 wallPos);
    void SetVelocityX(double vx);
    void TryJump();
    bool IsInWaterRush() const { return waterRushTimer > 0.0; }

    bool   IsDead() const;
    double GetHP() const;
    bool   IsDeveloperMode() const { return developerMode; }

    CS230::DashComponent  dashComponent;
    OriAnimation          oriAnim;
    bool                  dashEnabled      = false;
    bool                  wallClimbEnabled = false;
    bool                  bashEnabled      = true;
    bool                  inputLocked      = false;
    double                bashLockTimer    = 0.0;
    double                waterDepth       = 0.0;    // px submerged below water surface
    bool                  isJumping            = false;
    std::optional<size_t> currentPlatformIndex = std::nullopt;
    double                velocityY            = 0.0;
    bool                  faceRight            = true;

    CS230::GameObject* interactionTarget = nullptr;
    bool               isInteracting     = false;

    // Auto-move (cutscene controlled movement)
    bool       autoMoveActive  = false;
    Math::vec2 autoMoveTarget  = {};

    void SetAutoMove(Math::vec2 target);
    void SetAutoMove(const std::vector<WaypointStep>& wps);
    void ClearAutoMove();

private:
    void HandleInput(double dt);
    void HandleAutoMove(double dt);
    void DrawShieldCooldown(CS200::IRenderer2D& renderer) const;

    void UpdateHealthState(double dt);
    void RecordWallContact(int direction);

    void LandOnSurface(double surface_y);
    void HitCeiling(double ceiling_y);
    void HitWall(double wall_x, int direction);

    bool ResolveFloorSurfaceSnap(const Polygon& floor_poly, const Math::rect& my_box, double prev_bottom);
    bool ResolveFloorCeilingCollision(const Polygon& floor_poly, const Math::rect& my_box, double prev_top);
    bool ResolveFloorVerticalWallCollision(const Polygon& floor_poly, const Math::rect& my_box, double prev_left, double prev_right);
    bool ResolveFloorDiagonalWallCollision(const Polygon& floor_poly, const Math::rect& my_box, double prev_left, double prev_right, double prev_bottom, double prev_top);
    bool ResolveFloorVertexSideCollision(const Polygon& floor_poly, const Math::rect& my_box, double prev_left, double prev_right, double prev_bottom, double prev_top);
    bool ResolveAABBFallback(const Math::rect& my_box, const Math::rect& other_box, double prev_bottom, double prev_top, double prev_left, double prev_right);

    const double collisionHalfHeight = 40.0;

    Shield* shieldComponent = nullptr;

    double       gravity                      = 2200.0;
    double       jumpStrength                 = 1100.0;
    const double jumpBufferTime               = 0.12;
    double       jumpReleaseGravityMultiplier = 5.5;

    const double wallStickDuration           = 0.28;
    const double wallStickFallSpeed          = 30.0;
    const double wallSlideSpeed              = 150.0;
    const double wallJumpHorizontalStrength  = 750.0;
    const double wallJumpVerticalStrength    = 1100.0;
    double       wallJumpControlLockDuration = 0.15;
    double       autoMoveStuckTimer = 0.0;
    Math::vec2   autoMovePrevPos    = {};

    std::vector<WaypointStep> autoWaypoints;
    int    autoWaypointIdx = 0;
    bool   jumpLaunched    = false;

    double maxRunSpeed        = 800.0;
    double playerAcceleration = 5000.0;
    double playerFriction     = 4000.0;

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

    // ---- Death dissolve effect ----
    struct DeathParticle {
        Math::vec2 pos;
        Math::vec2 vel;
        double     lifetime;
        double     maxLifetime;
        double     size;       // long-axis length
        uint32_t   rgb;        // 0xRRGGBB
        double     initAngle;
        double     angularVel;
    };

    void InitDeathParticles();
    void DrawDeathEffect() const;

    bool                       deathEffectActive = false;
    std::vector<DeathParticle> deathParticles;

public:
    void UpdateDeathEffect(double dt);
    bool IsDeathEffectDone() const { return !deathEffectActive && healthState == HealthState::Dead; }

private:
    bool wasJumpingLastFrame = false;

    double     waterRushTimer  = 0.0;
    Math::vec2 waterRushDir    = { 0.0, 0.0 };
    Math::vec2 waterRushOrigin = { 0.0, 0.0 };
};
