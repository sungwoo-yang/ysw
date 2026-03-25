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

    Skeleton* GetSkeleton() const
    {
        return skeleton;
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

    // Build skeletal hierarchy for procedural animation
    void BuildSkeleton();

    // Calculate IK (Inverse Kinematics) for limbs based on movement
    void UpdateProceduralAnimation(double dt);

    // Update player health status and handle recovery over time
    void UpdateHealthState(double dt);

    // Walk animation phase accumulator
    double walkPhase = 0.0;

    // IK Target Positions for procedural animation
    Math::vec2 handTargetL;
    Math::vec2 handTargetR;
    Math::vec2 footTargetL;
    Math::vec2 footTargetR;

    // Body Proportions (Adjusted to fit safely inside the collision box)
    const double spineLenTop   = 9.0;  // Head
    const double spineLenUpper = 10.0; // Chest
    const double spineLenLower = 10.0; // Spine
    const double neckLen       = 3.0;

    // const double shoulderSpan = 18.0;
    // const double hipSpan      = 12.0;

    const double upperArmLen = 14.0;
    const double foreArmLen  = 14.0;

    const double thighLen = 18.0;
    const double shinLen  = 18.0;

    // Half height of collision box for grounding calculations
    const double collisionHalfHeight = 40.0;

    // System components
    Shield*         shieldComponent = nullptr;
    Skeleton*       skeleton        = nullptr;
    AnimationEditor animEditor;

    // Cached bone pointers for fast IK updates
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
    // const double jumpBufferTime  = 0.1;
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
};