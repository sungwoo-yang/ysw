#include "Player.hpp"
#include "CS200/IRenderer2D.hpp"

#include "Engine/AudioManager.hpp"
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
}

Player::Player(Math::vec2 in_start_pos)
    : CS230::GameObject(in_start_pos), isJumping(true), velocityY(0.0), faceRight(true), shieldComponent(nullptr), startPosition(in_start_pos), previousPosition(in_start_pos),
      healthState(HealthState::Full), playerHp(5.0), maxPlayerHp(5.0), recoverDelayTimer(0.0), tookDamageThisFrame(false), invincibilityTimer(0.0)
{
    // Initialize core components
    shieldComponent = new Shield(this);
    AddGOComponent(shieldComponent);

    // Add physical collision bounds
    AddGOComponent(new CS230::RectCollision(PLAYER_COLLISION_BOX, this));
    dashComponent.dashCooldown = 1.0;
}

void Player::Update(double dt)
{
    interactionTarget = nullptr;
    previousPosition  = GetPosition();

    // Jumping State
    wasJumpingLastFrame = isJumping;

    // Process invincibility timer
    if (invincibilityTimer > 0.0)
    {
        invincibilityTimer -= dt;
    }

    // Process platforming assist timers
    if (jumpBufferTimer > 0.0)
        jumpBufferTimer -= dt;
    if (coyoteTimer > 0.0)
        coyoteTimer -= dt;

    HandleInput(dt);
    dashComponent.UpdateTimers(dt);

    // Apply gravity unless disabled by dashing mechanics
    bool applyGravity = true;
    if (dashComponent.IsDashing() && dashComponent.disableGravityOnDash)
        applyGravity = false;

    if (applyGravity)
        velocityY -= gravity * dt;

    // Override horizontal velocity if dashing
    double finalVelX = GetVelocity().x;
    if (dashComponent.IsDashing())
        finalVelX = dashComponent.GetDashVelocityX();
    SetVelocity({ finalVelX, velocityY });

    isJumping            = true; // Assume jumping until collision resolution says otherwise
    currentPlatformIndex = std::nullopt;

    CS230::GameObject::Update(dt);

    UpdateHealthState(dt);
}

void Player::ResetState()
{
    // Complete reset of player entity for respawns
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

    playerHp            = maxPlayerHp;
    healthState         = HealthState::Full;
    recoverDelayTimer   = 0.0;
    tookDamageThisFrame = false;
    invincibilityTimer  = 0.0;

    // Reinitialize shield component
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

    // Respawn hotkey
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

        // Jump processing
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
    if (other_object_type == GameObjectTypes::Gate)
        return true;

    return false;
}

void Player::ResolveCollision(GameObject* other_object)
{
    auto  textManager = Engine::GetGameStateManager().GetGSComponent<WorldTextManager>();
    auto& input       = Engine::GetInput();

    if (other_object->Type() == GameObjectTypes::Floor || other_object->Type() == GameObjectTypes::Gate)
    {
        // AABB / SAT Collision Resolution for world geometry
        CS230::RectCollision* my_collider = GetGOComponent<CS230::RectCollision>();
        if (!my_collider)
            return;

        Math::rect other_box;

        if (other_object->Type() == GameObjectTypes::Floor)
        {
            CS230::SATCollision* floor_collider = other_object->GetGOComponent<CS230::SATCollision>();
            if (!floor_collider)
                return;
            other_box = floor_collider->WorldBoundary().FindBoundary();
        }
        else if (other_object->Type() == GameObjectTypes::Gate)
        {
            CS230::RectCollision* gate_collider = other_object->GetGOComponent<CS230::RectCollision>();
            if (!gate_collider)
                return;
            other_box = gate_collider->WorldBoundary();
        }

        Math::rect my_box = my_collider->WorldBoundary();

        double prev_bottom = previousPosition.y - (PLAYER_COLLISION_SIZE.y / 2.0);
        double prev_top    = previousPosition.y + (PLAYER_COLLISION_SIZE.y / 2.0);
        double prev_left   = previousPosition.x - (PLAYER_COLLISION_SIZE.x / 2.0);
        double prev_right  = previousPosition.x + (PLAYER_COLLISION_SIZE.x / 2.0);

        double platform_top    = other_box.Top();
        double platform_bottom = other_box.Bottom();
        double platform_left   = other_box.Left();
        double platform_right  = other_box.Right();

        // Determine relative positions from previous frame to find collision normal
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

        // Resolve penetration by shifting player out of the overlapping axis
        if (velocityY <= 0 && was_above && horizontal_overlap)
        {
            // Landing Sounds
            if (wasJumpingLastFrame)
            {
                AudioManager::Play("SFX_Landing");
                wasJumpingLastFrame = false;
            }


            // Landing on top of a surface
            SetPosition({ GetPosition().x, platform_top + (PLAYER_COLLISION_SIZE.y / 2.0) });
            velocityY   = 0.0;
            isJumping   = false;
            coyoteTimer = coyoteTime; // Refresh coyote time on ground contact
        }
        else if (velocityY > 0 && was_below && horizontal_overlap)
        {
            // Hitting head on ceiling
            SetPosition({ GetPosition().x, platform_bottom - (PLAYER_COLLISION_SIZE.y / 2.0) });
            velocityY = 0.0;
        }
        else if (GetVelocity().x > 0 && was_left && vertical_overlap)
        {
            // Hitting wall moving right
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

    CS200::RGBA playerColor = CS200::GREEN;

    if (invincibilityTimer > 0.0)
    {
        if (static_cast<int>(invincibilityTimer * 10.0) % 2 == 0)
        {
            playerColor = (playerColor & 0xFFFFFF00) | 100;
        }
    }

    switch (healthState)
    {
        case HealthState::Full: playerColor = CS200::GREEN; break;
        case HealthState::Healthy: playerColor = CS200::CYAN; break;
        case HealthState::Hurt: playerColor = CS200::YELLOW; break;
        case HealthState::Critical: playerColor = 0xFFA500FF; break;
        case HealthState::NearDeath: playerColor = CS200::RED; break;
        case HealthState::Dead: playerColor = 0x8B0000FF; break;
        default: playerColor = CS200::GREEN; break;
    }

    renderer.DrawRectangle(transform, playerColor, CS200::CLEAR, 0.0);

    if (shieldComponent)
    {
        shieldComponent->Draw(renderer, camera_matrix);
    }

    CS230::GameObject::Draw(camera_matrix);
}

void Player::ApplyLaserDamage(double damageAmount)
{
    if (healthState == HealthState::Dead)
        return;
    if (damageAmount <= 0.0)
        return;
    if (invincibilityTimer > 0.0)
        return;

    tookDamageThisFrame = true;
    recoverDelayTimer   = recoverDelayDuration;
    invincibilityTimer  = invincibilityDuration;

    playerHp = std::max(0.0, playerHp - damageAmount);

    if (playerHp <= 0.0)
    {
        playerHp    = 0.0;
        healthState = HealthState::Dead;
        Engine::GetLogger().LogEvent("Player died from laser damage.");
    }
}

void Player::UpdateHealthState(double dt)
{
    if (healthState == HealthState::Dead)
        return;

    if (recoverDelayTimer > 0.0)
    {
        recoverDelayTimer = std::max(0.0, recoverDelayTimer - dt);
    }
    else if (playerHp < maxPlayerHp)
    {
        playerHp = std::min(maxPlayerHp, playerHp + 1.0 * dt);
    }

    int hpInt = static_cast<int>(playerHp + 0.001);
    if (hpInt >= 5)
        healthState = HealthState::Full;
    else if (hpInt == 4)
        healthState = HealthState::Healthy;
    else if (hpInt == 3)
        healthState = HealthState::Hurt;
    else if (hpInt == 2)
        healthState = HealthState::Critical;
    else if (hpInt == 1)
        healthState = HealthState::NearDeath;
    else
        healthState = HealthState::Dead;

    tookDamageThisFrame = false;
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
            SetPosition({ static_cast<double>(p[0]), static_cast<double>(p[1]) });
        }

        Math::vec2 vel  = GetVelocity();
        float      v[2] = { static_cast<float>(vel.x), static_cast<float>(vel.y) };
        if (ImGui::DragFloat2("Velocity", v))
        {
            SetVelocity({ static_cast<double>(v[0]), static_cast<double>(v[1]) });
            velocityY = static_cast<double>(v[1]);
        }

        ImGui::Separator();
        ImGui::Text("Player HP: %.2f / %.2f", playerHp, maxPlayerHp);

        const char* stateItems[] = { "Dead", "Near Death", "Critical", "Hurt", "Healthy", "Full" };
        int         currentItem  = 0;
        int         hpInt        = static_cast<int>(playerHp + 0.001);

        if (hpInt >= 5)
            currentItem = 5;
        else if (hpInt == 4)
            currentItem = 4;
        else if (hpInt == 3)
            currentItem = 3;
        else if (hpInt == 2)
            currentItem = 2;
        else if (hpInt == 1)
            currentItem = 1;
        else
            currentItem = 0;

        if (ImGui::Combo("Health State", &currentItem, stateItems, 6))
        {
            playerHp = static_cast<double>(currentItem);
        }
        ImGui::Text("Recover Timer: %.2f", recoverDelayTimer);
        ImGui::Text("Invincibility Timer: %.2f", invincibilityTimer);

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

    // animEditor.DrawImGui();
}