#include "Player.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "PushableMirror.hpp"
#include "Shield.hpp"
#include "WorldTextManager.hpp"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <limits>
#include <string>
#include <vector>

namespace
{
    constexpr Math::irect PLAYER_COLLISION_BOX{
        { -20, -40 },
        {  20,  40 }
    };
    constexpr Math::vec2 PLAYER_COLLISION_SIZE{ 40.0, 80.0 };

    // Utility: Clamp
    template <typename T>
    T Clamp(T v, T lo, T hi)
    {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

    // Utility: Vector Lerp
    Math::vec2 LerpV(Math::vec2 a, Math::vec2 b, double t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    // IK Result Structure
    struct TwoBone
    {
        Math::vec2 mid;
        Math::vec2 end;
    };

    // Straight Limb IK Solver
    TwoBone SolveTwoBoneStraight(Math::vec2 start, double L1, double L2, Math::vec2 target, bool keepLength)
    {
        Math::vec2 diff = target - start;
        double     dist = diff.Length();
        Math::vec2 dir  = diff.Normalize();

        if (dist < 1e-6)
            dir = { 1.0, 0.0 };

        double totalLen = L1 + L2;

        double currentLen = dist;
        if (currentLen > totalLen)
        {
            currentLen = totalLen;
        }

        if (keepLength)
            currentLen = totalLen;

        Math::vec2 endPos = start + dir * currentLen;
        Math::vec2 midPos = start + dir * L1;

        return { midPos, endPos };
    }
}

Player::Player(Math::vec2 start_pos) : CS230::GameObject(start_pos), startPosition(start_pos), previousPosition(start_pos), shieldComponent(nullptr), isJumping(true), velocityY(0.0), faceRight(true)
{
    shieldComponent = new Shield(this);
    AddGOComponent(shieldComponent);

    skeleton = new Skeleton(this);
    AddGOComponent(skeleton);
    BuildSkeleton();
    animEditor.SetSkeleton(skeleton);
    AddGOComponent(new CS230::RectCollision(PLAYER_COLLISION_BOX, this));
    dashComponent.dashCooldown = 1.0;

    double floorY = start_pos.y - collisionHalfHeight;
    handTargetL   = start_pos;
    handTargetR   = start_pos;
    footTargetL   = { start_pos.x - 5.0, floorY };
    footTargetR   = { start_pos.x + 5.0, floorY };
}

void Player::BuildSkeleton()
{
    if (!skeleton)
        return;

    // Construct Hierarchy
    skeleton->AddBone("Hips", 0.0, { 0.0, 0.0 }, 0.0);

    skeleton->AddBone("SpineLower", spineLenLower, { 0.0, 0.0 }, 0.0, "Hips");
    skeleton->AddBone("SpineUpper", spineLenUpper, { 0.0, 0.0 }, 0.0, "SpineLower");
    skeleton->AddBone("Neck", neckLen, { 0.0, 0.0 }, 0.0, "SpineUpper");
    skeleton->AddBone("Head", spineLenTop, { 0.0, 0.0 }, 0.0, "Neck");

    // Add Nose to indicate facing direction
    skeleton->AddBone("Nose", 0.0, { 0.0, 0.0 }, 0.0, "Head");

    skeleton->AddBone("L_Thigh", thighLen, { 0.0, 0.0 }, 0.0, "Hips");
    skeleton->AddBone("L_Calf", shinLen, { 0.0, 0.0 }, 0.0, "L_Thigh");

    skeleton->AddBone("R_Thigh", thighLen, { 0.0, 0.0 }, 0.0, "Hips");
    skeleton->AddBone("R_Calf", shinLen, { 0.0, 0.0 }, 0.0, "R_Thigh");

    skeleton->AddBone("L_Clavicle", 0.0, { 0.0, 0.0 }, 0.0, "SpineUpper");
    skeleton->AddBone("L_Arm_Up", upperArmLen, { 0.0, 0.0 }, 0.0, "L_Clavicle");
    skeleton->AddBone("L_Arm_Low", foreArmLen, { 0.0, 0.0 }, 0.0, "L_Arm_Up");

    skeleton->AddBone("R_Clavicle", 0.0, { 0.0, 0.0 }, 0.0, "SpineUpper");
    skeleton->AddBone("R_Arm_Up", upperArmLen, { 0.0, 0.0 }, 0.0, "R_Clavicle");
    skeleton->AddBone("R_Arm_Low", foreArmLen, { 0.0, 0.0 }, 0.0, "R_Arm_Up");
}

void Player::Update(double dt)
{
    interactionTarget = nullptr;
    previousPosition  = GetPosition();

    if (jumpBufferTimer > 0.0)
        jumpBufferTimer -= dt;
    if (coyoteTimer > 0.0)
        coyoteTimer -= dt;

    HandleInput(dt);
    dashComponent.UpdateTimers(dt);

    bool applyGravity = true;
    if (dashComponent.IsDashing() && dashComponent.disableGravityOnDash)
        applyGravity = false;

    if (applyGravity)
        velocityY -= gravity * dt;

    double finalVelX = GetVelocity().x;
    if (dashComponent.IsDashing())
        finalVelX = dashComponent.GetDashVelocityX();
    SetVelocity({ finalVelX, velocityY });

    isJumping            = true;
    currentPlatformIndex = std::nullopt;

    CS230::GameObject::Update(dt);

    animEditor.Update(dt);
    if (animEditor.IsEditing())
    {
    }
    else
    {
        UpdateProceduralAnimation(dt);
    }
}

void Player::UpdateProceduralAnimation(double dt)
{
    if (!skeleton)
        return;

    // 1. Parameters
    int face = faceRight ? 1 : -1;

    float nearMulSpan = 1.18f;
    float farMulSpan  = 0.72f;
    float nearLenMul  = 1.00f;
    float farLenMul   = 0.92f;
    float nearYOffset = -2.0f;
    float farYOffset  = +4.0f;

    double currentShoulderSpan = 10.0;
    double currentHipSpan      = 8.0;

    double speedAbs   = std::abs(GetVelocity().x);
    bool   isGrounded = (std::abs(velocityY) < 10.0);
    bool   isMoving   = isGrounded && (speedAbs > 5.0);

    // 2. Walk Phase
    float phaseSpeedScale = 0.035f;
    if (isMoving)
    {
        walkPhase += (speedAbs * phaseSpeedScale * dt);
    }

    // 3. Bobbing
    float  hipBobAmp = 2.0f;
    double bob       = isMoving ? (-hipBobAmp * (0.5 * (1.0 - std::cos(2.0 * walkPhase)))) : 0.0;

    // 4. Calculate Key Positions
    Math::vec2 rootVisual = GetPosition() + Math::vec2{ 0.0, -3.0 };

    Math::vec2 spineBase = rootVisual + Math::vec2{ 0.0, bob };
    Math::vec2 pelvis    = spineBase;
    Math::vec2 spineMid  = spineBase + Math::vec2{ 0.0, spineLenLower };
    Math::vec2 chest     = spineMid + Math::vec2{ 0.0, spineLenUpper };
    Math::vec2 neck      = chest + Math::vec2{ 0.0, neckLen };
    Math::vec2 headBase  = neck;
    Math::vec2 headTop   = headBase + Math::vec2{ 0.0, spineLenTop };

    Math::vec2 nosePos = headTop + Math::vec2{ 5.0 * face, -3.0 };

    double nearSpanS = currentShoulderSpan * 0.5 * nearMulSpan;
    double farSpanS  = currentShoulderSpan * 0.5 * farMulSpan;
    double nearSpanH = currentHipSpan * 0.5 * nearMulSpan;
    double farSpanH  = currentHipSpan * 0.5 * farMulSpan;

    Math::vec2 shoulderNear = chest + Math::vec2{ -face * nearSpanS, nearYOffset };
    Math::vec2 shoulderFar  = chest + Math::vec2{ face * farSpanS, farYOffset };
    Math::vec2 hipNear      = pelvis + Math::vec2{ -face * nearSpanH, nearYOffset * 0.6 };
    Math::vec2 hipFar       = pelvis + Math::vec2{ face * farSpanH, farYOffset * 0.6 };

    Math::vec2 shoulderR = faceRight ? shoulderNear : shoulderFar;
    Math::vec2 shoulderL = faceRight ? shoulderFar : shoulderNear;
    Math::vec2 hipR      = faceRight ? hipNear : hipFar;
    Math::vec2 hipL      = faceRight ? hipFar : hipNear;

    double uArmR = upperArmLen * (faceRight ? nearLenMul : farLenMul);
    double fArmR = foreArmLen * (faceRight ? nearLenMul : farLenMul);
    double uArmL = upperArmLen * (faceRight ? farLenMul : nearLenMul);
    double fArmL = foreArmLen * (faceRight ? farLenMul : nearLenMul);

    double thighR_ = thighLen * (faceRight ? nearLenMul : farLenMul);
    double shinR_  = shinLen * (faceRight ? nearLenMul : farLenMul);
    double thighL_ = thighLen * (faceRight ? farLenMul : nearLenMul);
    double shinL_  = shinLen * (faceRight ? farLenMul : nearLenMul);

    // 5. Foot Targets
    double floorY = GetPosition().y - collisionHalfHeight;

    auto ClampLimbLength = [](Math::vec2 origin, Math::vec2 target, double maxLen) -> Math::vec2
    {
        Math::vec2 dir  = target - origin;
        double     dist = dir.Length();
        if (dist > maxLen)
        {
            return origin + dir.Normalize() * maxLen;
        }
        return target;
    };

    if (!isGrounded)
    {
        Math::vec2 airPoseL = hipL + Math::vec2{ -2.0 * face, -30.0 };
        Math::vec2 airPoseR = hipR + Math::vec2{ -2.0 * face, -30.0 };
        double     airBlend = 0.15;
        footTargetL         = LerpV(footTargetL, airPoseL, airBlend);
        footTargetR         = LerpV(footTargetR, airPoseR, airBlend);
    }
    else if (isMoving)
    {
        double strideBase   = 16.0;
        double stepLiftBase = 8.0;
        double stride       = strideBase * Clamp(speedAbs / 180.0, 0.6, 1.8);
        double stepLift     = stepLiftBase * Clamp(speedAbs / 180.0, 0.5, 1.6);

        auto ProcFoot = [&](Math::vec2 hip, double ph, double strideMul) -> Math::vec2
        {
            double swing = (stride * strideMul) * std::sin(ph);
            double lift  = stepLift * Clamp(std::sin(ph), 0.0, 1.0);
            double x     = hip.x + (face * swing);
            double y     = floorY + lift;
            return { x, y };
        };

        double phNear = walkPhase;
        double phFar  = walkPhase + 3.14159;

        Math::vec2 procNear = ProcFoot(faceRight ? hipR : hipL, phNear, 1.05);
        Math::vec2 procFar  = ProcFoot(faceRight ? hipL : hipR, phFar, 0.95);

        double walkFootBlend = 0.4;
        if (faceRight)
        {
            footTargetR = LerpV(footTargetR, procNear, walkFootBlend);
            footTargetL = LerpV(footTargetL, procFar, walkFootBlend);
        }
        else
        {
            footTargetL = LerpV(footTargetL, procNear, walkFootBlend);
            footTargetR = LerpV(footTargetR, procFar, walkFootBlend);
        }
    }
    else
    {
        Math::vec2 restNear      = { (faceRight ? hipR.x : hipL.x) + 4.0 * face, floorY };
        Math::vec2 restFar       = { (faceRight ? hipL.x : hipR.x) - 4.0 * face, floorY };
        double     idleFootBlend = 0.2;

        if (faceRight)
        {
            footTargetR = LerpV(footTargetR, restNear, idleFootBlend);
            footTargetL = LerpV(footTargetL, restFar, idleFootBlend);
        }
        else
        {
            footTargetL = LerpV(footTargetL, restNear, idleFootBlend);
            footTargetR = LerpV(footTargetR, restFar, idleFootBlend);
        }
    }

    footTargetL = ClampLimbLength(hipL, footTargetL, thighL_ + shinL_);
    footTargetR = ClampLimbLength(hipR, footTargetR, thighR_ + shinR_);

    // 6. Arm Targets
    double     armOffsetY  = isGrounded ? -25.0 : -15.0;
    Math::vec2 armRestNear = (faceRight ? shoulderR : shoulderL) + Math::vec2{ 4.0 * face, armOffsetY };
    Math::vec2 armRestFar  = (faceRight ? shoulderL : shoulderR) + Math::vec2{ -6.0 * face, armOffsetY };

    double restSmooth = 0.2;
    if (faceRight)
    {
        handTargetR = LerpV(handTargetR, armRestNear, restSmooth);
        handTargetL = LerpV(handTargetL, armRestFar, restSmooth);
    }
    else
    {
        handTargetL = LerpV(handTargetL, armRestNear, restSmooth);
        handTargetR = LerpV(handTargetR, armRestFar, restSmooth);
    }

    handTargetL = ClampLimbLength(shoulderL, handTargetL, uArmL + fArmL);
    handTargetR = ClampLimbLength(shoulderR, handTargetR, uArmR + fArmR);

    // 7. Solve IK (Modified SolveTwoBoneStraight handles length constraint)
    TwoBone armR = SolveTwoBoneStraight(shoulderR, uArmR, fArmR, handTargetR, false);
    TwoBone armL = SolveTwoBoneStraight(shoulderL, uArmL, fArmL, handTargetL, false);
    TwoBone legR = SolveTwoBoneStraight(hipR, thighR_, shinR_, footTargetR, false);
    TwoBone legL = SolveTwoBoneStraight(hipL, thighL_, shinL_, footTargetL, false);

    // 8. Apply Bones
    auto ConnectBone = [&](std::string name, Math::vec2 start, Math::vec2 end)
    {
        Bone* b = skeleton->GetBone(name);
        if (!b)
            return;

        Math::vec2 dir         = end - start;
        double     len         = dir.Length();
        double     targetAngle = std::atan2(dir.y, dir.x) + (3.14159 / 2.0);

        if (name == "Hips")
        {
            b->localPosition = start - GetPosition();
            b->angle         = 0.0;
            return;
        }

        double parentWorldAngle = 0.0;
        Bone*  p                = b->parent;
        while (p)
        {
            parentWorldAngle += p->angle;
            p = p->parent;
        }

        b->angle  = targetAngle - parentWorldAngle;
        b->length = len;

        if (b->parent)
        {
            Math::vec2 diff  = start - b->parent->worldStartPos;
            double     c     = std::cos(-parentWorldAngle);
            double     s     = std::sin(-parentWorldAngle);
            b->localPosition = { diff.x * c - diff.y * s, diff.x * s + diff.y * c };
        }
    };

    skeleton->Update(0.0);

    ConnectBone("Hips", pelvis, pelvis);
    ConnectBone("SpineLower", pelvis, spineMid);
    ConnectBone("SpineUpper", spineMid, chest);
    ConnectBone("Neck", chest, neck);
    ConnectBone("Head", neck, headTop);
    ConnectBone("Nose", headTop + Math::vec2{ 0, -5 }, nosePos);

    ConnectBone("L_Clavicle", chest, shoulderL);
    ConnectBone("L_Arm_Up", shoulderL, armL.mid);
    ConnectBone("L_Arm_Low", armL.mid, armL.end);

    ConnectBone("R_Clavicle", chest, shoulderR);
    ConnectBone("R_Arm_Up", shoulderR, armR.mid);
    ConnectBone("R_Arm_Low", armR.mid, armR.end);

    ConnectBone("L_Thigh", hipL, legL.mid);
    ConnectBone("L_Calf", legL.mid, legL.end);

    ConnectBone("R_Thigh", hipR, legR.mid);
    ConnectBone("R_Calf", legR.mid, legR.end);
}

void Player::ResetState()
{
    SetPosition(startPosition);
    previousPosition     = startPosition;
    velocityY            = 0.0;
    isJumping            = true;
    currentPlatformIndex = std::nullopt;
    faceRight            = true;

    dashComponent.isDashing         = false;
    dashComponent.dashTimer         = 0.0;
    dashComponent.dashCooldownTimer = 0.0;

    currentSpeedMultiplier = 1.0;
    interactionTarget      = nullptr;
    isInteracting          = false;

    if (shieldComponent != nullptr)
    {
        RemoveGOComponent<Shield>();
    }
    shieldComponent = new Shield(this);
    AddGOComponent(shieldComponent);
}

void Player::SetSavePoint(Math::vec2 new_spawn_point)
{
    startPosition = new_spawn_point;
    Engine::GetLogger().LogEvent("Player save point updated!");
}

void Player::HandleInput(double dt)
{
    auto& input = Engine::GetInput();

    if (shieldComponent)
    {
        shieldComponent->HandleInput(dt);
    }

    if (input.KeyJustPressed(CS230::Input::Keys::R))
    {
        ResetState();
        Engine::GetLogger().LogEvent("Event: Player Respawned (R)");
        return;
    }

    if (input.KeyJustPressed(CS230::Input::Keys::LShift))
    {
        dashComponent.TryStartDash(faceRight);
    }

    if (!dashComponent.IsDashing())
    {
        double targetMult = 1.0;
        if (shieldComponent && shieldComponent->IsGuardUp())
        {
            targetMult = minShieldSpeedMult;
        }

        if (currentSpeedMultiplier > targetMult)
        {
            currentSpeedMultiplier -= shieldSlowdownRate * dt;
            if (currentSpeedMultiplier < targetMult)
                currentSpeedMultiplier = targetMult;
        }
        else if (currentSpeedMultiplier < targetMult)
        {
            currentSpeedMultiplier += shieldSlowdownRate * dt;
            if (currentSpeedMultiplier > targetMult)
                currentSpeedMultiplier = targetMult;
        }

        double currentSpeed = baseSpeed * currentSpeedMultiplier;

        Math::vec2 move{ 0.0, 0.0 };
        if (input.KeyDown(CS230::Input::Keys::A))
        {
            move.x -= 1.0;
            faceRight = false;
        }
        if (input.KeyDown(CS230::Input::Keys::D))
        {
            move.x += 1.0;
            faceRight = true;
        }

        SetVelocity({ move.x * currentSpeed, GetVelocity().y });

        if (!isJumping && (input.KeyJustPressed(CS230::Input::Keys::W) || input.KeyJustPressed(CS230::Input::Keys::Space)))
        {
            jumpBufferTimer = jumpStrength;
        }

        if (coyoteTimer > 0.0 && jumpBufferTimer > 0.0 && velocityY <= 0.0)
        {
            velocityY = jumpStrength;
            isJumping = true;

            jumpBufferTimer = 0.0;
            coyoteTimer     = 0.0;

            Engine::GetLogger().LogEvent("Event: Player Jump (Buffered/Coyote)");
        }
    }
}

bool Player::CanCollideWith(GameObjectTypes other_object_type)
{
    if (other_object_type == GameObjectTypes::Floor)
        return true;
    if (other_object_type == GameObjectTypes::Sign)
        return true;
    if (other_object_type == GameObjectTypes::Bonfire)
        return true;
    if (other_object_type == GameObjectTypes::Door)
        return true;
    if (other_object_type == GameObjectTypes::PushableMirror)
        return true;

    return false;
}

void Player::ResolveCollision(GameObject* other_object)
{
    auto  textManager = Engine::GetGameStateManager().GetGSComponent<WorldTextManager>();
    auto& input       = Engine::GetInput();

    if (other_object->Type() == GameObjectTypes::Floor)
    {
        CS230::RectCollision* my_collider = GetGOComponent<CS230::RectCollision>();
        if (!my_collider)
            return;
        CS230::SATCollision* other_collider = other_object->GetGOComponent<CS230::SATCollision>();
        if (!other_collider)
            return;

        Math::rect my_box    = my_collider->WorldBoundary();
        Math::rect other_box = other_collider->WorldBoundary().FindBoundary();

        double prev_bottom = previousPosition.y - (PLAYER_COLLISION_SIZE.y / 2.0);
        double prev_top    = previousPosition.y + (PLAYER_COLLISION_SIZE.y / 2.0);
        double prev_left   = previousPosition.x - (PLAYER_COLLISION_SIZE.x / 2.0);
        double prev_right  = previousPosition.x + (PLAYER_COLLISION_SIZE.x / 2.0);

        double platform_top    = other_box.Top();
        double platform_bottom = other_box.Bottom();
        double platform_left   = other_box.Left();
        double platform_right  = other_box.Right();

        bool was_above = prev_bottom >= platform_top;
        bool was_below = prev_top <= platform_bottom;
        bool was_left  = prev_right <= platform_left;
        bool was_right = prev_left >= platform_right;

        double overlap_bottom = my_box.Top() - other_box.Bottom();
        double overlap_top    = other_box.Top() - my_box.Bottom();
        double overlap_left   = my_box.Right() - other_box.Left();
        double overlap_right  = other_box.Right() - my_box.Left();

        bool horizontal_overlap = my_box.Right() > other_box.Left() && my_box.Left() < other_box.Right();
        bool vertical_overlap   = my_box.Top() > other_box.Bottom() && my_box.Bottom() < other_box.Top();

        if (!horizontal_overlap || !vertical_overlap)
            return;

        if (velocityY <= 0 && was_above && horizontal_overlap)
        {
            SetPosition({ GetPosition().x, platform_top + (PLAYER_COLLISION_SIZE.y / 2.0) });
            velocityY   = 0.0;
            isJumping   = false;
            coyoteTimer = coyoteTime;
        }
        else if (velocityY > 0 && was_below && horizontal_overlap)
        {
            SetPosition({ GetPosition().x, platform_bottom - (PLAYER_COLLISION_SIZE.y / 2.0) });
            velocityY = 0.0;
        }
        else if (GetVelocity().x > 0 && was_left && vertical_overlap)
        {
            SetPosition({ platform_left - (PLAYER_COLLISION_SIZE.x / 2.0), GetPosition().y });
            SetVelocity({ 0.0, GetVelocity().y });
        }
        else if (GetVelocity().x < 0 && was_right && vertical_overlap)
        {
            SetPosition({ platform_right + (PLAYER_COLLISION_SIZE.x / 2.0), GetPosition().y });
            SetVelocity({ 0.0, GetVelocity().y });
        }
        else
        {
            double min_overlap = overlap_bottom;
            int    axis        = 0;

            if (overlap_top < min_overlap)
            {
                min_overlap = overlap_top;
                axis        = 1;
            }
            if (overlap_left < min_overlap)
            {
                min_overlap = overlap_left;
                axis        = 2;
            }
            if (overlap_right < min_overlap)
            {
                min_overlap = overlap_right;
                axis        = 3;
            }

            switch (axis)
            {
                case 0:
                    SetPosition({ GetPosition().x, platform_bottom - (PLAYER_COLLISION_SIZE.y / 2.0) });
                    if (velocityY > 0)
                        velocityY = 0.0;
                    break;
                case 1:
                    SetPosition({ GetPosition().x, platform_top + (PLAYER_COLLISION_SIZE.y / 2.0) });
                    if (velocityY < 0)
                        velocityY = 0.0;
                    isJumping   = false;
                    coyoteTimer = coyoteTime;
                    break;
                case 2: SetPosition({ platform_left - (PLAYER_COLLISION_SIZE.x / 2.0), GetPosition().y }); break;
                case 3: SetPosition({ platform_right + (PLAYER_COLLISION_SIZE.x / 2.0), GetPosition().y }); break;
            }
        }
    }
    else if (other_object->Type() == GameObjectTypes::Sign || other_object->Type() == GameObjectTypes::Bonfire || other_object->Type() == GameObjectTypes::Door)
    {
        interactionTarget = other_object;

        if (input.KeyJustPressed(CS230::Input::Keys::F))
        {
            isInteracting = true;
        }

        if (isInteracting)
        {
            other_object->Interact(this);
        }
        else
        {
            if (textManager != nullptr)
            {
                textManager->ShowTextBelow(other_object, "Press 'F'", 0.5, CS200::WHITE);
            }
        }
    }
    else if (other_object->Type() == GameObjectTypes::PushableMirror)
    {
        auto mirrorBox = static_cast<PushableMirror*>(other_object);

        CS230::RectCollision* my_collider    = GetGOComponent<CS230::RectCollision>();
        CS230::RectCollision* other_collider = mirrorBox->GetGOComponent<CS230::RectCollision>();

        if (my_collider && other_collider)
        {
            Math::rect my_rect    = my_collider->WorldBoundary();
            Math::rect other_rect = other_collider->WorldBoundary();

            bool isSideCollision = my_rect.Top() > other_rect.Bottom() + 5.0 && my_rect.Bottom() < other_rect.Top() - 5.0;

            if (isSideCollision)
            {
                if (GetPosition().x < mirrorBox->GetPosition().x)
                {
                    if (GetVelocity().x > 0)
                        mirrorBox->Push({ GetVelocity().x * 0.9, 0 });
                }
                else
                {
                    if (GetVelocity().x < 0)
                        mirrorBox->Push({ GetVelocity().x * 0.9, 0 });
                }
            }
        }
    }
}

void Player::Draw(const Math::TransformationMatrix& camera_matrix)
{
    CS200::IRenderer2D&        renderer  = Engine::GetRenderer2D();
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(PLAYER_COLLISION_SIZE);

    renderer.DrawRectangle(transform, CS200::GREEN, CS200::CLEAR, 0.0);

    if (shieldComponent)
    {
        shieldComponent->Draw(renderer, camera_matrix);
    }

    if (skeleton)
    {
        // Define Draw Order based on Facing Direction
        std::vector<std::string> drawOrder;

        auto AddLeftLimbs = [&]()
        {
            drawOrder.push_back("L_Thigh");
            drawOrder.push_back("L_Calf");
            drawOrder.push_back("L_Clavicle");
            drawOrder.push_back("L_Arm_Up");
            drawOrder.push_back("L_Arm_Low");
        };
        auto AddRightLimbs = [&]()
        {
            drawOrder.push_back("R_Thigh");
            drawOrder.push_back("R_Calf");
            drawOrder.push_back("R_Clavicle");
            drawOrder.push_back("R_Arm_Up");
            drawOrder.push_back("R_Arm_Low");
        };
        auto AddBody = [&]()
        {
            drawOrder.push_back("Hips");
            drawOrder.push_back("SpineLower");
            drawOrder.push_back("SpineUpper");
            drawOrder.push_back("Neck");
            drawOrder.push_back("Head");
            drawOrder.push_back("Nose");
        };

        if (faceRight)
        {
            // Right: Left(Far) -> Body -> Right(Near)
            AddLeftLimbs();
            AddBody();
            AddRightLimbs();
        }
        else
        {
            // Left: Right(Far) -> Body -> Left(Near)
            AddRightLimbs();
            AddBody();
            AddLeftLimbs();
        }

        for (const auto& name : drawOrder)
        {
            Bone* b = skeleton->GetBone(name);
            if (b)
            {
                renderer.DrawLine(b->worldStartPos, b->worldEndPos, CS200::WHITE, 2.0);

                Math::TransformationMatrix circleTransform = Math::TranslationMatrix(b->worldStartPos) * Math::ScaleMatrix({ 2.5, 2.5 });
                renderer.DrawCircle(circleTransform, CS200::WHITE);
            }
        }
    }

    CS230::GameObject::Draw(camera_matrix);
}

void Player::DrawImGui()
{
    ImGui::PushID(this);
    if (ImGui::TreeNode("Player"))
    {
        Math::vec2 pos  = GetPosition();
        float      p[2] = { static_cast<float>(pos.x), static_cast<float>(pos.y) };
        if (ImGui::DragFloat2("Position", p))
        {
            SetPosition({ p[0], p[1] });
        }

        Math::vec2 vel  = GetVelocity();
        float      v[2] = { static_cast<float>(vel.x), static_cast<float>(vel.y) };
        if (ImGui::DragFloat2("Velocity", v))
        {
            SetVelocity({ v[0], v[1] });
            velocityY = v[1];
        }

        ImGui::Separator();
        ImGui::Text("Movement State:");
        ImGui::Checkbox("Is Jumping", &isJumping);
        ImGui::Checkbox("Face Right", &faceRight);
        ImGui::Text("Coyote Timer: %.2f", coyoteTimer);
        ImGui::Text("Jump Buffer: %.2f", jumpBufferTimer);

        ImGui::Separator();
        ImGui::Text("Dash State:");
        ImGui::Checkbox("Is Dashing", &dashComponent.isDashing);
        ImGui::Text("Dash Timer: %.2f", dashComponent.dashTimer);
        ImGui::Text("Dash Cooldown: %.2f", dashComponent.dashCooldownTimer);

        ImGui::Separator();
        if (shieldComponent)
        {
            shieldComponent->DrawImGui();
        }

        ImGui::TreePop();
    }
    ImGui::PopID();

    animEditor.DrawImGui();
}