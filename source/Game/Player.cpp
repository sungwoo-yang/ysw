#include "Player.hpp"
#include "CS200/IRenderer2D.hpp"

#include "Engine/AudioManager.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "Engine/TextureManager.hpp"

#include "PushableMirror.hpp"
#include "Shield.hpp"
#include "WorldTextManager.hpp"
#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace
{
    constexpr Math::irect PLAYER_COLLISION_BOX{
        { -20, -40 },
        {  20,  40 }
    };
    constexpr Math::vec2 PLAYER_COLLISION_SIZE{ 40.0, 80.0 };
    constexpr Math::vec2 SHIELD_COOLDOWN_BAR_SIZE{ 68.0, 8.0 };
    constexpr Math::vec2 SHIELD_COOLDOWN_BAR_OFFSET{ 0.0, 58.0 };

    Math::TransformationMatrix RectangleTransform(Math::vec2 center, Math::vec2 size)
    {
        return Math::TranslationMatrix(center) * Math::ScaleMatrix(size);
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
    if (wallJumpControlLockTimer > 0.0)
        wallJumpControlLockTimer -= dt;

    HandleInput(dt);
    dashComponent.UpdateTimers(dt);

    // Apply gravity unless disabled by dashing mechanics
    auto&      input         = Engine::GetInput();
    const bool jumpHeld      = input.KeyDown(CS230::Input::Keys::W) || input.KeyDown(CS230::Input::Keys::Space);
    const bool shouldCutJump = isJumping && velocityY > 0.0 && !jumpHeld && !dashComponent.IsDashing();

    bool applyGravity = true;
    if (dashComponent.IsDashing() && dashComponent.disableGravityOnDash)
        applyGravity = true;

    if (applyGravity)
    {
        const double gravityMultiplier = shouldCutJump ? jumpReleaseGravityMultiplier : 1.0;
        velocityY -= gravity * gravityMultiplier * dt;
    }

    const bool pressingTowardWall = (wallDirection < 0 && input.KeyDown(CS230::Input::Keys::A)) || (wallDirection > 0 && input.KeyDown(CS230::Input::Keys::D));
    const bool canWallSlide       = isJumping && isTouchingWall && wallDirection != 0 && pressingTowardWall && !dashComponent.IsDashing() && velocityY <= 0.0;
    isWallSliding                 = canWallSlide;

    if (canWallSlide)
    {
        wallContactTimer += dt;
        const double maxFallSpeed = wallContactTimer < wallStickDuration ? wallStickFallSpeed : wallSlideSpeed;
        velocityY                 = std::max(velocityY, -maxFallSpeed);
    }
    else
    {
        wallContactTimer = 0.0;
    }

    // Override horizontal velocity if dashing
    double finalVelX = GetVelocity().x;
    if (dashComponent.IsDashing())
        finalVelX = dashComponent.GetDashVelocityX();
    SetVelocity({ finalVelX, velocityY });

    isJumping            = true;
    currentPlatformIndex = std::nullopt;
    isTouchingWall       = false;
    wallDirection        = 0;

    CS230::GameObject::Update(dt);

    UpdateHealthState(dt);
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

    currentSpeedMultiplier   = 1.0;
    interactionTarget        = nullptr;
    isInteracting            = false;
    isTouchingWall           = false;
    isWallSliding            = false;
    wallDirection            = 0;
    wallContactTimer         = 0.0;
    wallJumpControlLockTimer = 0.0;

    playerHp            = maxPlayerHp;
    healthState         = HealthState::Full;
    recoverDelayTimer   = 0.0;
    tookDamageThisFrame = false;
    invincibilityTimer  = 0.0;

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

        double targetMaxSpeed = maxRunSpeed * currentSpeedMultiplier;

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

        double currentVelX = GetVelocity().x;
        double targetVelX  = move.x * targetMaxSpeed;

        double     accel                = playerAcceleration;
        double     friction             = playerFriction;
        const bool canControlHorizontal = wallJumpControlLockTimer <= 0.0;

        if (!canControlHorizontal)
        {
            currentVelX = GetVelocity().x;
        }
        else if (std::abs(move.x) > 0.1)
        {
            const bool changingDirection = (move.x > 0.0 && currentVelX < 0.0) || (move.x < 0.0 && currentVelX > 0.0);
            if (changingDirection)
            {
                currentVelX = 0.0;
            }
            else if (currentVelX < targetVelX)
            {
                currentVelX = std::min(currentVelX + accel * dt, targetVelX);
            }
            else if (currentVelX > targetVelX)
            {
                currentVelX = std::max(currentVelX - accel * dt, targetVelX);
            }
        }
        else
        {
            if (currentVelX > 0.0)
            {
                currentVelX = std::max(0.0, currentVelX - friction * dt);
            }
            else if (currentVelX < 0.0)
            {
                currentVelX = std::min(0.0, currentVelX + friction * dt);
            }
        }

        SetVelocity({ currentVelX, GetVelocity().y });

        // Jump processing
        const bool jumpPressed = input.KeyJustPressed(CS230::Input::Keys::W) || input.KeyJustPressed(CS230::Input::Keys::Space);
        if (jumpPressed)
        {
            jumpBufferTimer = jumpBufferTime;
        }

        if (isJumping && isTouchingWall && wallDirection != 0 && jumpBufferTimer > 0.0)
        {
            const double jumpDirection = static_cast<double>(-wallDirection);
            currentVelX                = jumpDirection * wallJumpHorizontalStrength;
            velocityY                  = wallJumpVerticalStrength;
            faceRight                  = jumpDirection > 0.0;

            SetVelocity({ currentVelX, velocityY });

            isTouchingWall           = false;
            isWallSliding            = false;
            wallDirection            = 0;
            wallContactTimer         = 0.0;
            wallJumpControlLockTimer = wallJumpControlLockDuration;
            jumpBufferTimer          = 0.0;
            coyoteTimer              = 0.0;

            Engine::GetLogger().LogEvent("Event: Player Wall Jump");
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

void Player::RecordWallContact(int direction)
{
    isTouchingWall = true;
    wallDirection  = direction;
}

void Player::ResolveCollision(GameObject* other_object)
{
    auto  textManager = Engine::GetGameStateManager().GetGSComponent<WorldTextManager>();
    auto& input       = Engine::GetInput();

    if (other_object->Type() == GameObjectTypes::Floor || other_object->Type() == GameObjectTypes::Gate)
    {
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
            if (wasJumpingLastFrame)
            {
                AudioManager::Play("SFX_Landing");
                wasJumpingLastFrame = false;
            }

            SetPosition({ GetPosition().x, platform_top + (PLAYER_COLLISION_SIZE.y / 2.0) });
            velocityY                = 0.0;
            isJumping                = false;
            coyoteTimer              = coyoteTime;
            wallJumpControlLockTimer = 0.0;
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
            RecordWallContact(1);
        }
        else if (GetVelocity().x < 0 && was_right && vertical_overlap)
        {
            SetPosition({ platform_right + (PLAYER_COLLISION_SIZE.x / 2.0), GetPosition().y });
            SetVelocity({ 0.0, GetVelocity().y });
            RecordWallContact(-1);
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
                    isJumping                = false;
                    coyoteTimer              = coyoteTime;
                    wallJumpControlLockTimer = 0.0;
                    break;
                case 2:
                    SetPosition({ platform_left - (PLAYER_COLLISION_SIZE.x / 2.0), GetPosition().y });
                    if (GetVelocity().x > 0.0)
                        SetVelocity({ 0.0, GetVelocity().y });
                    RecordWallContact(1);
                    break;
                case 3:
                    SetPosition({ platform_right + (PLAYER_COLLISION_SIZE.x / 2.0), GetPosition().y });
                    if (GetVelocity().x < 0.0)
                        SetVelocity({ 0.0, GetVelocity().y });
                    RecordWallContact(-1);
                    break;
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

    // auto texture = CS230::TextureManager().Load("Assets/Character.png");

    DrawShieldCooldown(renderer);

    CS230::GameObject::Draw(camera_matrix);
}

void Player::DrawShieldCooldown(CS200::IRenderer2D& renderer) const
{
    if (shieldComponent == nullptr)
    {
        return;
    }

    const bool isFrozen  = shieldComponent->IsFrozen();
    const bool isCooling = shieldComponent->IsCoolingDown();
    if (!isFrozen && !isCooling)
    {
        return;
    }

    const double readyRatio = isFrozen ? shieldComponent->GetFreezeRatio() : 1.0 - shieldComponent->GetCooldownRatio();
    const double fillRatio  = std::clamp(readyRatio, 0.0, 1.0);

    const Math::vec2 center = GetPosition() + SHIELD_COOLDOWN_BAR_OFFSET;
    renderer.DrawRectangle(RectangleTransform(center, SHIELD_COOLDOWN_BAR_SIZE), 0x000000A0, 0xFFFFFFFF, 1.0);

    if (fillRatio <= 0.0)
    {
        return;
    }

    const double     fillWidth = (SHIELD_COOLDOWN_BAR_SIZE.x - 2.0) * fillRatio;
    const Math::vec2 fillSize{ fillWidth, SHIELD_COOLDOWN_BAR_SIZE.y - 2.0 };
    const Math::vec2 fillCenter{ center.x - (SHIELD_COOLDOWN_BAR_SIZE.x * 0.5) + 1.0 + (fillWidth * 0.5), center.y };

    const CS200::RGBA fillColor = isFrozen ? 0xFF4040E0 : 0x00FFFFFF;
    renderer.DrawRectangle(RectangleTransform(fillCenter, fillSize), fillColor, CS200::CLEAR, 0.0);
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

bool Player::IsDead() const
{
    return healthState == HealthState::Dead || playerHp <= 0.0;
}

double Player::GetHP() const
{
    return playerHp;
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
        ImGui::Checkbox("Developer Mode", &developerMode);
        if (developerMode)
        {
            ImGui::Text("Movement Tuning:");

            float gravityValue = static_cast<float>(gravity);
            if (ImGui::DragFloat("Gravity", &gravityValue, 10.0f, 100.0f, 5000.0f))
            {
                gravity = static_cast<double>(std::max(gravityValue, 0.0f));
            }

            float jumpStrengthValue = static_cast<float>(jumpStrength);
            if (ImGui::DragFloat("Jump Strength", &jumpStrengthValue, 10.0f, 0.0f, 3000.0f))
            {
                jumpStrength = static_cast<double>(std::max(jumpStrengthValue, 0.0f));
            }

            float releaseGravityValue = static_cast<float>(jumpReleaseGravityMultiplier);
            if (ImGui::DragFloat("Jump Release Gravity Mult", &releaseGravityValue, 0.05f, 1.0f, 8.0f))
            {
                jumpReleaseGravityMultiplier = static_cast<double>(std::max(releaseGravityValue, 1.0f));
            }

            float maxSpeedValue = static_cast<float>(maxRunSpeed);
            if (ImGui::DragFloat("Player Max Speed", &maxSpeedValue, 10.0f, 0.0f, 3000.0f))
            {
                maxRunSpeed = static_cast<double>(std::max(maxSpeedValue, 0.0f));
            }

            float playerAccelValue = static_cast<float>(playerAcceleration);
            if (ImGui::DragFloat("Player Acceleration", &playerAccelValue, 10.0f, 100.0f, 10000.0f))
            {
                playerAcceleration = static_cast<double>(std::max(playerAccelValue, 0.0f));
            }

            float playerFrictionValue = static_cast<float>(playerFriction);
            if (ImGui::DragFloat("Player Friction", &playerFrictionValue, 10.0f, 0.0f, 10000.0f))
            {
                playerFriction = static_cast<double>(std::max(playerFrictionValue, 0.0f));
            }

            float wallJumpLockValue = static_cast<float>(wallJumpControlLockDuration);
            if (ImGui::DragFloat("Wall Jump Input Lock", &wallJumpLockValue, 0.01f, 0.0f, 1.0f))
            {
                wallJumpControlLockDuration = static_cast<double>(std::max(wallJumpLockValue, 0.0f));
            }

            if (ImGui::Button("Reset Movement Tuning"))
            {
                gravity                      = 1500.0;
                jumpStrength                 = 800.0;
                jumpReleaseGravityMultiplier = 2.5;
                maxRunSpeed                  = 500.0;
                playerAcceleration           = 2200.0;
                playerFriction               = 1800.0;
                wallJumpControlLockDuration  = 0.12;
            }
        }

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
}
