#pragma once

#include "Engine/Dash.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Skeleton.hpp"
#include "Engine/Vec2.hpp" 
#include "Engine/AnimationEditor.hpp"

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
    Player(Math::vec2 start_pos);
    ~Player() override = default;

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;
    void DrawImGui() override;

    const Math::vec2& GetPosition() const { return GameObject::GetPosition(); };

    GameObjectTypes Type() override { return GameObjectTypes::Player; }
    std::string TypeName() override { return "Player"; }

    bool CanCollideWith(GameObjectTypes other_object_type) override;
    void ResolveCollision(CS230::GameObject* other_object) override;

    void ResetState();
    void SetSavePoint(Math::vec2 new_spawn_point);

    Shield* GetShield() const { return shieldComponent; }
    Skeleton* GetSkeleton() const { return skeleton; }

    CS230::DashComponent dashComponent;
    bool                 isJumping            = false;
    std::optional<size_t> currentPlatformIndex = std::nullopt;
    double               velocityY            = 0.0;
    bool                 faceRight            = true;

    CS230::GameObject* interactionTarget = nullptr;
    bool               isInteracting     = false;

private:
    void HandleInput(double dt);
    
    // Initialize Skeleton
    void BuildSkeleton(); 
    
    // Procedural Animation Update
    void UpdateProceduralAnimation(double dt);
    
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
    
    const double shoulderSpan  = 18.0; // Narrower shoulders
    const double hipSpan       = 12.0;
    
    const double upperArmLen   = 14.0;
    const double foreArmLen    = 14.0;
    
    // Lower Body Total: 18 + 18 = 36px (< 40px)
    const double thighLen      = 18.0;
    const double shinLen       = 18.0;
    
    // Half height of collision box (80.0 / 2.0)
    const double collisionHalfHeight = 40.0;

    Shield* shieldComponent;
    Skeleton* skeleton = nullptr;
    AnimationEditor animEditor;

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
};