#pragma once

#include "Engine/AnimationEditor.hpp"
#include "Engine/Dash.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Skeleton.hpp"
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

    // Update player logic per frame
    void Update(double dt) override;

    // Draw player graphics
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    // Draw debug UI
    void DrawImGui() override;

    [[nodiscard]] const Math::vec2& GetPosition() const
    {
        return GameObject::GetPosition();
    };

    [[nodiscard]] GameObjectTypes Type() override
    {
        return GameObjectTypes::Player;
    }

    [[nodiscard]] std::string TypeName() override
    {
        return "Player";
    }

    [[nodiscard]] Shield* GetShield() const
    {
        return shieldComponent;
    }

    [[nodiscard]] Skeleton* GetSkeleton() const
    {
        return skeleton;
    }

    // Check collidable objects
    bool CanCollideWith(GameObjectTypes other_object_type) override;

    // Resolve physical collision
    void ResolveCollision(CS230::GameObject* other_object) override;

    // Reset player to initial state
    void ResetState();

    // Update respawn coordinate
    void SetSavePoint(Math::vec2 new_spawn_point);

    // Handle laser damage
    void ApplyLaserDamage(double damageAmount);

    CS230::DashComponent  dashComponent;
    bool                  isJumping            = false;
    std::optional<size_t> currentPlatformIndex = std::nullopt;
    double                velocityY            = 0.0;
    bool                  faceRight            = true;

    CS230::GameObject* interactionTarget = nullptr;
    bool               isInteracting     = false;

private:
    // Process player input
    void HandleInput(double dt);

    // Build skeletal hierarchy
    void BuildSkeleton();

    // Update procedural animation
    void UpdateProceduralAnimation(double dt);

    // Update player health status
    void UpdateHealthState(double dt);

    // Walk animation state
    double walkPhase = 0.0;

    // IK Target Positions
    Math::vec2 handTargetL;
    Math::vec2 handTargetR;
    Math::vec2 footTargetL;
    Math::vec2 footTargetR;

    // Body Proportions (Adjusted to fit inside 80px height)
    // Upper Body Total: 10 + 10 + 3 + 9 = 32px (< 40px)
    const double spineLenTop   = 9.0;  // Head
    const double spineLenUpper = 10.0; // Chest
    const double spineLenLower = 10.0; // Spine
    const double neckLen       = 3.0;

    const double shoulderSpan = 18.0; // Narrower shoulders
    const double hipSpan      = 12.0;

    const double upperArmLen = 14.0;
    const double foreArmLen  = 14.0;

    // Lower Body Total: 18 + 18 = 36px (< 40px)
    const double thighLen = 18.0;
    const double shinLen  = 18.0;

    // Half height of collision box (80.0 / 2.0)
    const double collisionHalfHeight = 40.0;

    Shield*         shieldComponent = nullptr;
    Skeleton*       skeleton        = nullptr;
    AnimationEditor animEditor;

    Bone* bHips       = nullptr;
    Bone* bSpineLower = nullptr;
    Bone* bSpineUpper = nullptr;
    Bone* bNeck       = nullptr;
    Bone* bHead       = nullptr;
    Bone* bNose       = nullptr;
    Bone* bLThigh     = nullptr;
    Bone* bLCalf      = nullptr;
    Bone* bRThigh     = nullptr;
    Bone* bRCalf      = nullptr;
    Bone* bLClavicle  = nullptr;
    Bone* bLArmUp     = nullptr;
    Bone* bLArmLow    = nullptr;
    Bone* bRClavicle  = nullptr;
    Bone* bRArmUp     = nullptr;
    Bone* bRArmLow    = nullptr;

    const double gravity      = 1500.0;
    const double jumpStrength = 700.0;
    const double baseSpeed    = 300.0;

    double       currentSpeedMultiplier = 1.0;
    const double shieldSlowdownRate     = 4.0;
    const double minShieldSpeedMult     = 0.3;

    Math::vec2 startPosition;
    Math::vec2 previousPosition;

    double       jumpBufferTimer = 0.0;
    double       coyoteTimer     = 0.0;
    const double jumpBufferTime  = 0.1;
    const double coyoteTime      = 0.1;

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
    // const double recoverRate       = 1.0;
    bool         tookDamageThisFrame  = false;

    double       invincibilityTimer    = 0.0;
    const double invincibilityDuration = 1.0;
};