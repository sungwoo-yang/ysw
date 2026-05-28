#include "Player.hpp"
#include "CS200/IRenderer2D.hpp"

#include "Engine/AudioManager.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "Engine/Polygon.h"
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

    constexpr double MAX_WALKABLE_SLOPE_ANGLE_DEG = 70.0;
    constexpr double EDGE_NORMAL_SAMPLE_OFFSET    = 3.0;
    constexpr double TOP_EDGE_NORMAL_Y_MIN        = 0.25;
    constexpr double BOTTOM_EDGE_NORMAL_Y_MAX     = -0.25;

    Math::TransformationMatrix RectangleTransform(Math::vec2 center, Math::vec2 size)
    {
        return Math::TranslationMatrix(center) * Math::ScaleMatrix(size);
    }

    bool PointInsidePolygon(const Polygon& polygon, Math::vec2 point)
    {
        bool        inside = false;
        const auto& verts  = polygon.vertices;

        if (verts.empty())
        {
            return false;
        }

        for (size_t i = 0, j = verts.size() - 1; i < verts.size(); j = i++)
        {
            const Math::vec2 vi = verts[i];
            const Math::vec2 vj = verts[j];

            const bool intersect = ((vi.y > point.y) != (vj.y > point.y)) && (point.x < (vj.x - vi.x) * (point.y - vi.y) / ((vj.y - vi.y) + 0.000001) + vi.x);

            if (intersect)
            {
                inside = !inside;
            }
        }

        return inside;
    }

    Math::vec2 GetEdgeOutwardNormal(const Polygon& polygon, Math::vec2 p1, Math::vec2 p2)
    {
        constexpr double EDGE_EPS = 0.000001;

        const Math::vec2 edge = p2 - p1;

        Math::vec2 normal{ -edge.y, edge.x };

        const double normal_length_sq = normal.LengthSquared();

        if (normal_length_sq < EDGE_EPS)
        {
            return { 0.0, 0.0 };
        }

        normal = normal * (1.0 / std::sqrt(normal_length_sq));

        const Math::vec2 mid{ (p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5 };

        const bool plus_inside  = PointInsidePolygon(polygon, mid + normal * EDGE_NORMAL_SAMPLE_OFFSET);
        const bool minus_inside = PointInsidePolygon(polygon, mid - normal * EDGE_NORMAL_SAMPLE_OFFSET);

        // If +normal side is inside and -normal side is outside,
        // then normal points inward, so flip it.
        if (plus_inside && !minus_inside)
        {
            normal = normal * -1.0;
        }
        else if (plus_inside == minus_inside)
        {
            // Unreliable edge classification.
            // This can happen near complex concave regions or tiny edges.
            return { 0.0, 0.0 };
        }

        return normal;
    }

    bool IsWalkableTopEdge(const Polygon& polygon, Math::vec2 p1, Math::vec2 p2)
    {
        const Math::vec2 outward = GetEdgeOutwardNormal(polygon, p1, p2);

        // A top/walkable edge has its outside direction mostly upward.
        return outward.y > TOP_EDGE_NORMAL_Y_MIN;
    }

    bool IsBlockingBottomEdge(const Polygon& polygon, Math::vec2 p1, Math::vec2 p2)
    {
        const Math::vec2 outward = GetEdgeOutwardNormal(polygon, p1, p2);

        // A bottom/ceiling edge has its outside direction mostly downward.
        return outward.y < BOTTOM_EDGE_NORMAL_Y_MAX;
    }

    double GetEdgeAngleDegrees(Math::vec2 p1, Math::vec2 p2)
    {
        const double dx = p2.x - p1.x;
        const double dy = p2.y - p1.y;

        if (std::abs(dx) < 0.000001 && std::abs(dy) < 0.000001)
        {
            return 0.0;
        }

        return std::atan2(std::abs(dy), std::abs(dx)) * 180.0 / PI;
    }

    bool IsWalkableSlopeEdge(const Polygon& polygon, Math::vec2 p1, Math::vec2 p2)
    {
        if (!IsWalkableTopEdge(polygon, p1, p2))
        {
            return false;
        }

        const double angle = GetEdgeAngleDegrees(p1, p2);

        return angle <= MAX_WALKABLE_SLOPE_ANGLE_DEG;
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
    if (other_object_type == GameObjectTypes::FallingBlock)
        return true;

    return false;
}

void Player::RecordWallContact(int direction)
{
    isTouchingWall = true;
    wallDirection  = direction;
}

void Player::LandOnSurface(double surface_y)
{
    if (wasJumpingLastFrame)
    {
        AudioManager::Play("SFX_Landing");
        wasJumpingLastFrame = false;
    }

    SetPosition({ GetPosition().x, surface_y + (PLAYER_COLLISION_SIZE.y / 2.0) });

    velocityY = 0.0;
    SetVelocity({ GetVelocity().x, 0.0 });

    isJumping                = false;
    coyoteTimer              = coyoteTime;
    wallJumpControlLockTimer = 0.0;
}

void Player::HitCeiling(double ceiling_y)
{
    SetPosition({ GetPosition().x, ceiling_y - (PLAYER_COLLISION_SIZE.y / 2.0) });

    velocityY = 0.0;
    SetVelocity({ GetVelocity().x, 0.0 });

    isJumping = true;
}

void Player::HitWall(double wall_x, int direction)
{
    if (direction > 0)
    {
        SetPosition({ wall_x - (PLAYER_COLLISION_SIZE.x / 2.0), GetPosition().y });
        RecordWallContact(1);
    }
    else
    {
        SetPosition({ wall_x + (PLAYER_COLLISION_SIZE.x / 2.0), GetPosition().y });
        RecordWallContact(-1);
    }

    SetVelocity({ 0.0, GetVelocity().y });
}

bool Player::ResolveFloorSurfaceSnap(const Polygon& floor_poly, const Math::rect& my_box, double prev_bottom)
{
    if (floor_poly.vertexCount < 3 || velocityY > 0.0)
    {
        return false;
    }

    constexpr double SURFACE_EDGE_EPS          = 0.001;
    constexpr double SURFACE_X_MARGIN          = 6.0;
    constexpr double SURFACE_CONTACT_TOLERANCE = 8.0;
    constexpr double MAX_SURFACE_STEP_UP       = 8.0;
    constexpr double MAX_SURFACE_SNAP_DOWN     = 2.0;
    constexpr double MIN_SURFACE_SIDE_SPEED    = 1.0;
    constexpr double FOOT_INSET                = 2.0;

    const double current_bottom = my_box.Bottom();

    const bool moving_right = GetVelocity().x > MIN_SURFACE_SIDE_SPEED;
    const bool moving_left  = GetVelocity().x < -MIN_SURFACE_SIDE_SPEED;

    bool   found_surface       = false;
    double surface_y           = 0.0;
    double surface_max_step_up = MAX_SURFACE_STEP_UP;

    auto try_probe_x = [&](double probe_x)
    {
        const auto& verts = floor_poly.vertices;

        for (size_t i = 0; i < verts.size(); ++i)
        {
            Math::vec2 p1 = verts[i];
            Math::vec2 p2 = verts[(i + 1) % verts.size()];

            const double dx = p2.x - p1.x;
            const double dy = p2.y - p1.y;

            // Vertical edges are walls, not walkable floor surfaces.
            if (std::abs(dx) < SURFACE_EDGE_EPS)
            {
                continue;
            }

            if (!IsWalkableSlopeEdge(floor_poly, p1, p2))
            {
                continue;
            }

            // Degenerate edge guard.
            if ((p2 - p1).LengthSquared() < SURFACE_EDGE_EPS)
            {
                continue;
            }

            const double min_x = std::min(p1.x, p2.x) - SURFACE_X_MARGIN;
            const double max_x = std::max(p1.x, p2.x) + SURFACE_X_MARGIN;

            if (probe_x < min_x || probe_x > max_x)
            {
                continue;
            }

            double t = (probe_x - p1.x) / dx;
            t        = std::clamp(t, 0.0, 1.0);

            const double candidate_y = p1.y + dy * t;
            const double delta       = candidate_y - current_bottom;

            const double edge_angle = GetEdgeAngleDegrees(p1, p2);

            const double dynamic_step_up = edge_angle > 5.0 ? std::clamp(MAX_SURFACE_STEP_UP + std::abs(GetVelocity().x) * 0.03, MAX_SURFACE_STEP_UP, 24.0) : MAX_SURFACE_STEP_UP;

            const bool crossed_surface_candidate = prev_bottom >= candidate_y && current_bottom <= candidate_y + SURFACE_CONTACT_TOLERANCE;

            const bool step_candidate = delta >= -MAX_SURFACE_SNAP_DOWN && delta <= dynamic_step_up;

            // Landing and step-up are different.
            // Fast falling/dashing should still catch the floor if the player crossed the surface.
            if (!crossed_surface_candidate && !step_candidate)
            {
                continue;
            }

            // Use the highest valid surface.
            if (!found_surface || candidate_y > surface_y)
            {
                found_surface       = true;
                surface_y           = candidate_y;
                surface_max_step_up = dynamic_step_up;
            }
        }
    };

    // Center probe.
    try_probe_x(GetPosition().x);

    // Left/right foot probes.
    // These help when jumping, dashing, or landing near a slope edge.
    try_probe_x(my_box.Left() + FOOT_INSET);
    try_probe_x(my_box.Right() - FOOT_INSET);

    // Extra front probe for slope seams.
    if (moving_right)
    {
        try_probe_x(my_box.Right());
    }
    else if (moving_left)
    {
        try_probe_x(my_box.Left());
    }

    if (!found_surface)
    {
        return false;
    }

    const double delta_to_surface = surface_y - current_bottom;

    const bool crossed_surface = prev_bottom >= surface_y && current_bottom <= surface_y + SURFACE_CONTACT_TOLERANCE;

    const bool close_enough_to_step = delta_to_surface >= -MAX_SURFACE_SNAP_DOWN && delta_to_surface <= surface_max_step_up;

    if (!crossed_surface && !close_enough_to_step)
    {
        return false;
    }

    LandOnSurface(surface_y);
    return true;
}

bool Player::ResolveFloorCeilingCollision(const Polygon& floor_poly, const Math::rect& my_box, double prev_top)
{
    if (floor_poly.vertexCount < 3 || velocityY <= 0.0)
    {
        return false;
    }

    constexpr double CEILING_EDGE_EPS          = 0.001;
    constexpr double CEILING_X_MARGIN          = 4.0;
    constexpr double CEILING_CONTACT_TOLERANCE = 8.0;
    constexpr double HEAD_INSET                = 2.0;

    const double current_top = my_box.Top();

    bool   found_ceiling = false;
    double ceiling_y     = 0.0;

    auto try_head_probe_x = [&](double probe_x)
    {
        const auto& verts = floor_poly.vertices;

        for (size_t i = 0; i < verts.size(); ++i)
        {
            Math::vec2 p1 = verts[i];
            Math::vec2 p2 = verts[(i + 1) % verts.size()];

            const double dx = p2.x - p1.x;

            if (std::abs(dx) < CEILING_EDGE_EPS)
            {
                continue;
            }

            if ((p2 - p1).LengthSquared() < CEILING_EDGE_EPS)
            {
                continue;
            }

            if (!IsBlockingBottomEdge(floor_poly, p1, p2))
            {
                continue;
            }

            const double min_x = std::min(p1.x, p2.x) - CEILING_X_MARGIN;
            const double max_x = std::max(p1.x, p2.x) + CEILING_X_MARGIN;

            if (probe_x < min_x || probe_x > max_x)
            {
                continue;
            }

            double t = (probe_x - p1.x) / dx;
            t        = std::clamp(t, 0.0, 1.0);

            const double candidate_y = p1.y + (p2.y - p1.y) * t;

            const bool crossed_ceiling = prev_top <= candidate_y && current_top >= candidate_y - CEILING_CONTACT_TOLERANCE;

            if (!crossed_ceiling)
            {
                continue;
            }

            // Use the lowest valid ceiling, because that is the first underside hit.
            if (!found_ceiling || candidate_y < ceiling_y)
            {
                found_ceiling = true;
                ceiling_y     = candidate_y;
            }
        }
    };

    try_head_probe_x(GetPosition().x);
    try_head_probe_x(my_box.Left() + HEAD_INSET);
    try_head_probe_x(my_box.Right() - HEAD_INSET);

    if (!found_ceiling)
    {
        return false;
    }

    HitCeiling(ceiling_y);
    return true;
}

bool Player::ResolveFloorVerticalWallCollision(const Polygon& floor_poly, const Math::rect& my_box, double prev_left, double prev_right)
{
    if (floor_poly.vertexCount < 3)
    {
        return false;
    }

    constexpr double WALL_EDGE_EPS            = 0.001;
    constexpr double WALL_Y_MARGIN            = 2.0;
    constexpr double WALL_MIN_HEIGHT          = 8.0;
    constexpr double MIN_SIDE_WALL_SPEED      = 0.5;
    constexpr double WALL_CROSS_TOLERANCE     = 2.0;
    constexpr double SLOPE_SEAM_FOOT_MARGIN   = 28.0;
    constexpr double SLOPE_SEAM_MIN_ANGLE_DEG = 5.0;

    const bool moving_right = GetVelocity().x > MIN_SIDE_WALL_SPEED;
    const bool moving_left  = GetVelocity().x < -MIN_SIDE_WALL_SPEED;

    const bool trying_right = moving_right || my_box.Right() > prev_right;
    const bool trying_left  = moving_left || my_box.Left() < prev_left;

    const auto& verts = floor_poly.vertices;

    auto is_walkable_nonflat_slope = [&](Math::vec2 a, Math::vec2 b)
    {
        return IsWalkableSlopeEdge(floor_poly, a, b) && GetEdgeAngleDegrees(a, b) > SLOPE_SEAM_MIN_ANGLE_DEG;
    };

    for (size_t i = 0; i < verts.size(); ++i)
    {
        Math::vec2 p1 = verts[i];
        Math::vec2 p2 = verts[(i + 1) % verts.size()];

        const double dx = p2.x - p1.x;
        const double dy = p2.y - p1.y;

        // Only vertical edges are side walls.
        if (std::abs(dx) >= WALL_EDGE_EPS)
        {
            continue;
        }

        if (std::abs(dy) < WALL_MIN_HEIGHT)
        {
            continue;
        }

        const Math::vec2 prev = verts[(i + verts.size() - 1) % verts.size()];
        const Math::vec2 next = verts[(i + 2) % verts.size()];

        const bool p1_connected_to_sloped_surface = is_walkable_nonflat_slope(prev, p1);

        const bool p2_connected_to_sloped_surface = is_walkable_nonflat_slope(p2, next);

        const bool foot_near_p1 = std::abs(my_box.Bottom() - p1.y) <= SLOPE_SEAM_FOOT_MARGIN;

        const bool foot_near_p2 = std::abs(my_box.Bottom() - p2.y) <= SLOPE_SEAM_FOOT_MARGIN;

        // Only skip artificial slope seams.
        // A flat floor connected to a vertical wall must still behave as a real wall.
        if ((p1_connected_to_sloped_surface && foot_near_p1) || (p2_connected_to_sloped_surface && foot_near_p2))
        {
            continue;
        }

        const double wall_x      = p1.x;
        const double wall_bottom = std::min(p1.y, p2.y);
        const double wall_top    = std::max(p1.y, p2.y);

        constexpr double MICRO_STEP_UP_MAX   = 10.0;
        constexpr double MICRO_STEP_DOWN_MAX = 6.0;

        const double foot_to_wall_top = wall_top - my_box.Bottom();

        const bool can_ignore_as_micro_step = foot_to_wall_top <= MICRO_STEP_UP_MAX && foot_to_wall_top >= -MICRO_STEP_DOWN_MAX;

        if (can_ignore_as_micro_step)
        {
            continue;
        }

        const bool vertical_body_overlap = my_box.Top() > wall_bottom + WALL_Y_MARGIN && my_box.Bottom() < wall_top - WALL_Y_MARGIN;

        if (!vertical_body_overlap)
        {
            continue;
        }

        const bool crossed_from_left = trying_right && prev_right <= wall_x + WALL_CROSS_TOLERANCE && my_box.Right() >= wall_x - WALL_CROSS_TOLERANCE;

        const bool penetrating_from_left = trying_right && my_box.Left() < wall_x && my_box.Right() >= wall_x - WALL_CROSS_TOLERANCE;

        if (crossed_from_left || penetrating_from_left)
        {
            HitWall(wall_x, 1);
            return true;
        }

        const bool crossed_from_right = trying_left && prev_left >= wall_x - WALL_CROSS_TOLERANCE && my_box.Left() <= wall_x + WALL_CROSS_TOLERANCE;

        const bool penetrating_from_right = trying_left && my_box.Right() > wall_x && my_box.Left() <= wall_x + WALL_CROSS_TOLERANCE;

        if (crossed_from_right || penetrating_from_right)
        {
            HitWall(wall_x, -1);
            return true;
        }
    }

    return false;
}

bool Player::ResolveFloorDiagonalWallCollision(const Polygon& floor_poly, const Math::rect& my_box, double prev_left, double prev_right, double prev_bottom, double prev_top)
{
    if (floor_poly.vertexCount < 3)
    {
        return false;
    }

    constexpr double DIAGONAL_EDGE_EPS         = 0.001;
    constexpr double BODY_INSET                = 2.0;
    constexpr double Y_MARGIN                  = 8.0;
    constexpr double CROSS_TOLERANCE           = 5.0;
    constexpr double PUSH_OUT_SKIN             = 0.75;
    constexpr double MIN_SIDE_NORMAL_X         = 0.20;
    constexpr double WALL_SLIDE_NORMAL_Y_MAX   = 0.35;
    constexpr double CEILING_LIKE_NORMAL_Y     = -0.35;
    // constexpr double DIAGONAL_SEAM_FOOT_MARGIN = 28.0;
    constexpr int    PROBE_COUNT               = 21;

    const double current_left   = my_box.Left();
    const double current_right  = my_box.Right();
    const double current_bottom = my_box.Bottom();
    const double current_top    = my_box.Top();
    const double prev_center_x  = (prev_left + prev_right) * 0.5;

    const double swept_bottom = std::min(prev_bottom, current_bottom) + BODY_INSET;
    const double swept_top    = std::max(prev_top, current_top) - BODY_INSET;

    if (swept_top <= swept_bottom)
    {
        return false;
    }

    bool found_block_from_left  = false;
    bool found_block_from_right = false;

    double left_block_x  = 999999.0;
    double right_block_x = -999999.0;

    Math::vec2 left_block_normal  = { 0.0, 0.0 };
    Math::vec2 right_block_normal = { 0.0, 0.0 };

    auto test_probe_y = [&](double probe_y)
    {
        const auto& verts = floor_poly.vertices;

        for (size_t i = 0; i < verts.size(); ++i)
        {
            Math::vec2 p1 = verts[i];
            Math::vec2 p2 = verts[(i + 1) % verts.size()];

            const double dx = p2.x - p1.x;
            const double dy = p2.y - p1.y;

            if (std::abs(dx) < DIAGONAL_EDGE_EPS)
            {
                continue;
            }

            if (std::abs(dy) < DIAGONAL_EDGE_EPS)
            {
                continue;
            }

            if ((p2 - p1).LengthSquared() < DIAGONAL_EDGE_EPS)
            {
                continue;
            }

            if (IsWalkableSlopeEdge(floor_poly, p1, p2))
            {
                continue;
            }

            const Math::vec2 prev = verts[(i + verts.size() - 1) % verts.size()];
            const Math::vec2 next = verts[(i + 2) % verts.size()];

            const bool connected_to_walkable_slope = IsWalkableSlopeEdge(floor_poly, prev, p1) || IsWalkableSlopeEdge(floor_poly, p2, next);

            const double edge_angle = GetEdgeAngleDegrees(p1, p2);

            constexpr double DIAGONAL_SEAM_MAX_EDGE_HEIGHT = 128.0;
            constexpr double DIAGONAL_SEAM_MAX_EDGE_ANGLE  = 80.0;

            // External path seams can be tall or near-vertical.
            // So do not rely only on edge height/angle.
            constexpr double EXTERNAL_SEAM_STEP_UP_MAX   = 42.0;
            constexpr double EXTERNAL_SEAM_STEP_DOWN_MAX = 8.0;

            const bool can_be_internal_slope_seam = std::abs(dy) <= DIAGONAL_SEAM_MAX_EDGE_HEIGHT || edge_angle <= DIAGONAL_SEAM_MAX_EDGE_ANGLE;

            const double edge_top_y       = std::max(p1.y, p2.y);
            const double foot_to_edge_top = edge_top_y - current_bottom;

            const bool foot_can_step_over_edge_top = foot_to_edge_top <= EXTERNAL_SEAM_STEP_UP_MAX && foot_to_edge_top >= -EXTERNAL_SEAM_STEP_DOWN_MAX;

            const bool moving_on_floor_or_slope = velocityY <= 0.0;

            // This handles path-to-path or rect-to-path seams.
            // The edge may be long or near-vertical, but if the player's foot is near
            // the edge top, it should behave like a small step/seam rather than a wall.
            if (moving_on_floor_or_slope && foot_can_step_over_edge_top)
            {
                continue;
            }

            // This handles normal same-polygon slope seams.
            if (connected_to_walkable_slope && can_be_internal_slope_seam && foot_can_step_over_edge_top)
            {
                continue;
            }

            const Math::vec2 outward = GetEdgeOutwardNormal(floor_poly, p1, p2);

            // Strong downward-facing edges are underside/ceiling-like.
            // Do not resolve them as side walls, because that can push the player
            // across a thin/tapered polygon such as path39's pointed lower region.
            if (outward.y < CEILING_LIKE_NORMAL_Y)
            {
                continue;
            }

            if (std::abs(outward.x) < MIN_SIDE_NORMAL_X)
            {
                continue;
            }

            const double min_y = std::min(p1.y, p2.y) - Y_MARGIN;
            const double max_y = std::max(p1.y, p2.y) + Y_MARGIN;

            if (probe_y < min_y || probe_y > max_y)
            {
                continue;
            }

            double t = (probe_y - p1.y) / dy;
            t        = std::clamp(t, 0.0, 1.0);

            const double edge_x = p1.x + dx * t;

            const bool blocks_from_right = outward.x > 0.0;
            const bool blocks_from_left  = outward.x < 0.0;

            const bool was_on_left_side = prev_center_x <= edge_x + CROSS_TOLERANCE;

            const bool crossed_from_left = blocks_from_left && was_on_left_side && current_right >= edge_x - CROSS_TOLERANCE && current_left < edge_x;

            const bool penetrating_from_left = blocks_from_left && was_on_left_side && current_left < edge_x && current_right >= edge_x - CROSS_TOLERANCE;

            if (crossed_from_left || penetrating_from_left)
            {
                if (!found_block_from_left || edge_x < left_block_x)
                {
                    found_block_from_left = true;
                    left_block_x          = edge_x;
                    left_block_normal     = outward;
                }
            }

            const bool was_on_right_side = prev_center_x >= edge_x - CROSS_TOLERANCE;

            const bool crossed_from_right = blocks_from_right && was_on_right_side && current_left <= edge_x + CROSS_TOLERANCE && current_right > edge_x;

            const bool penetrating_from_right = blocks_from_right && was_on_right_side && current_right > edge_x && current_left <= edge_x + CROSS_TOLERANCE;

            if (crossed_from_right || penetrating_from_right)
            {
                if (!found_block_from_right || edge_x > right_block_x)
                {
                    found_block_from_right = true;
                    right_block_x          = edge_x;
                    right_block_normal     = outward;
                }
            }
        }
    };

    for (int i = 0; i < PROBE_COUNT; ++i)
    {
        const double t       = static_cast<double>(i) / static_cast<double>(PROBE_COUNT - 1);
        const double probe_y = swept_bottom + (swept_top - swept_bottom) * t;

        test_probe_y(probe_y);
    }

    const double half_width = PLAYER_COLLISION_SIZE.x / 2.0;

    bool       should_push   = false;
    double     new_x         = GetPosition().x;
    int        wall_dir      = 0;
    Math::vec2 chosen_normal = { 0.0, 0.0 };

    const double left_target_x  = left_block_x - half_width - PUSH_OUT_SKIN;
    const double right_target_x = right_block_x + half_width + PUSH_OUT_SKIN;

    const bool can_push_left = found_block_from_left && left_target_x < GetPosition().x;

    const bool can_push_right = found_block_from_right && right_target_x > GetPosition().x;

    if (can_push_left && can_push_right)
    {
        const double separator_x = (left_block_x + right_block_x) * 0.5;

        if (prev_center_x <= separator_x)
        {
            should_push   = true;
            new_x         = left_target_x;
            wall_dir      = 1;
            chosen_normal = left_block_normal;
        }
        else
        {
            should_push   = true;
            new_x         = right_target_x;
            wall_dir      = -1;
            chosen_normal = right_block_normal;
        }
    }
    else if (can_push_left)
    {
        should_push   = true;
        new_x         = left_target_x;
        wall_dir      = 1;
        chosen_normal = left_block_normal;
    }
    else if (can_push_right)
    {
        should_push   = true;
        new_x         = right_target_x;
        wall_dir      = -1;
        chosen_normal = right_block_normal;
    }

    if (!should_push)
    {
        return false;
    }

    SetPosition({ new_x, GetPosition().y });

    Math::vec2 current_velocity = GetVelocity();

    if (wall_dir > 0 && current_velocity.x > 0.0)
    {
        current_velocity.x = 0.0;
    }
    else if (wall_dir < 0 && current_velocity.x < 0.0)
    {
        current_velocity.x = 0.0;
    }

    const bool ceiling_like_wall = chosen_normal.y < CEILING_LIKE_NORMAL_Y;

    if (ceiling_like_wall && current_velocity.y > 0.0)
    {
        current_velocity.y = 0.0;
    }

    SetVelocity(current_velocity);
    velocityY = current_velocity.y;

    const bool allow_wall_contact = wall_dir != 0 && !ceiling_like_wall && std::abs(chosen_normal.y) <= WALL_SLIDE_NORMAL_Y_MAX;

    if (allow_wall_contact)
    {
        RecordWallContact(wall_dir);
    }

    return true;
}

bool Player::ResolveFloorVertexSideCollision(const Polygon& floor_poly, const Math::rect& my_box, double prev_left, double prev_right, double prev_bottom, double prev_top)
{
    if (floor_poly.vertexCount < 3)
    {
        return false;
    }

    constexpr double BODY_INSET       = 3.0;
    constexpr double CROSS_TOLERANCE  = 5.0;
    constexpr double PUSH_OUT_SKIN    = 0.75;
    // constexpr double FOOT_SKIP_MARGIN = 28.0;
    constexpr double MIN_SIDE_SPEED   = 0.1;

    const double current_left   = my_box.Left();
    const double current_right  = my_box.Right();
    const double current_bottom = my_box.Bottom();
    const double current_top    = my_box.Top();

    const double swept_bottom = std::min(prev_bottom, current_bottom) + BODY_INSET;
    const double swept_top    = std::max(prev_top, current_top) - BODY_INSET;

    if (swept_top <= swept_bottom)
    {
        return false;
    }

    const bool moving_right = GetVelocity().x > MIN_SIDE_SPEED || current_right > prev_right;

    const bool moving_left = GetVelocity().x < -MIN_SIDE_SPEED || current_left < prev_left;

    bool   found_block_from_left  = false;
    bool   found_block_from_right = false;
    double left_block_x           = 999999.0;
    double right_block_x          = -999999.0;

    const auto& verts = floor_poly.vertices;

    for (size_t i = 0; i < verts.size(); ++i)
    {
        const Math::vec2 vertex = verts[i];

        if (vertex.y < swept_bottom || vertex.y > swept_top)
        {
            continue;
        }

        const Math::vec2 prev_vertex = verts[(i + verts.size() - 1) % verts.size()];
        const Math::vec2 next_vertex = verts[(i + 1) % verts.size()];

        const bool connected_to_walkable_surface = IsWalkableSlopeEdge(floor_poly, prev_vertex, vertex) || IsWalkableSlopeEdge(floor_poly, vertex, next_vertex);

        // A path-to-path seam often appears as a local top vertex.
        // A real wall base is usually a local bottom vertex, so do not skip that.
        const bool vertex_is_local_top = vertex.y >= prev_vertex.y - 0.001 && vertex.y >= next_vertex.y - 0.001;

        constexpr double EXTERNAL_VERTEX_STEP_UP_MAX   = 42.0;
        constexpr double EXTERNAL_VERTEX_STEP_DOWN_MAX = 8.0;

        const double foot_to_vertex = vertex.y - current_bottom;

        const bool foot_can_step_over_vertex = foot_to_vertex <= EXTERNAL_VERTEX_STEP_UP_MAX && foot_to_vertex >= -EXTERNAL_VERTEX_STEP_DOWN_MAX;

        if ((connected_to_walkable_surface || vertex_is_local_top || foot_can_step_over_vertex) && foot_can_step_over_vertex && velocityY <= 0.0)
        {
            continue;
        }

        const bool crossed_from_left = moving_right && prev_right <= vertex.x + CROSS_TOLERANCE && current_right >= vertex.x - CROSS_TOLERANCE && current_left < vertex.x;

        if (crossed_from_left)
        {
            found_block_from_left = true;
            left_block_x          = std::min(left_block_x, vertex.x);
        }

        const bool crossed_from_right = moving_left && prev_left >= vertex.x - CROSS_TOLERANCE && current_left <= vertex.x + CROSS_TOLERANCE && current_right > vertex.x;

        if (crossed_from_right)
        {
            found_block_from_right = true;
            right_block_x          = std::max(right_block_x, vertex.x);
        }
    }

    const double half_width = PLAYER_COLLISION_SIZE.x / 2.0;

    bool   should_push = false;
    double new_x       = GetPosition().x;
    int    wall_dir    = 0;

    if (found_block_from_left)
    {
        const double target_x = left_block_x - half_width - PUSH_OUT_SKIN;
        const double push     = target_x - GetPosition().x;

        if (push < 0.0)
        {
            should_push = true;
            new_x       = target_x;
            wall_dir    = 1;
        }
    }

    if (found_block_from_right)
    {
        const double target_x = right_block_x + half_width + PUSH_OUT_SKIN;
        const double push     = target_x - GetPosition().x;

        if (push > 0.0)
        {
            if (!should_push || std::abs(push) < std::abs(new_x - GetPosition().x))
            {
                should_push = true;
                new_x       = target_x;
                wall_dir    = -1;
            }
        }
    }

    if (!should_push)
    {
        return false;
    }

    SetPosition({ new_x, GetPosition().y });

    Math::vec2 current_velocity = GetVelocity();

    if (wall_dir > 0 && current_velocity.x > 0.0)
    {
        current_velocity.x = 0.0;
    }
    else if (wall_dir < 0 && current_velocity.x < 0.0)
    {
        current_velocity.x = 0.0;
    }

    SetVelocity(current_velocity);

    if (wall_dir != 0)
    {
        RecordWallContact(wall_dir);
    }

    return true;
}

bool Player::ResolveAABBFallback(const Math::rect& my_box, const Math::rect& other_box, double prev_bottom, double prev_top, double prev_left, double prev_right)
{
    const double platform_top    = other_box.Top();
    const double platform_bottom = other_box.Bottom();
    const double platform_left   = other_box.Left();
    const double platform_right  = other_box.Right();

    const bool horizontal_overlap = my_box.Right() > other_box.Left() && my_box.Left() < other_box.Right();

    const bool vertical_overlap = my_box.Top() > other_box.Bottom() && my_box.Bottom() < other_box.Top();

    if (!horizontal_overlap || !vertical_overlap)
    {
        return false;
    }

    const bool was_above = prev_bottom >= platform_top;
    const bool was_left  = prev_right <= platform_left;
    const bool was_right = prev_left >= platform_right;

    constexpr double MICRO_STEP_UP_MAX   = 10.0;
    constexpr double MICRO_STEP_DOWN_MAX = 6.0;

    const double foot_to_platform_top = platform_top - my_box.Bottom();

    const bool can_micro_step_to_top = foot_to_platform_top <= MICRO_STEP_UP_MAX && foot_to_platform_top >= -MICRO_STEP_DOWN_MAX && my_box.Top() > platform_top + 4.0;

    if (velocityY <= 0.0 && was_above && horizontal_overlap)
    {
        LandOnSurface(platform_top);
        return true;
    }

    if (velocityY > 0.0 && horizontal_overlap && prev_top <= platform_bottom && my_box.Top() >= platform_bottom)
    {
        HitCeiling(platform_bottom);
        return true;
    }

    if (GetVelocity().x > 0.0 && was_left && vertical_overlap)
    {
        if (can_micro_step_to_top)
        {
            LandOnSurface(platform_top);
            return true;
        }

        HitWall(platform_left, 1);
        return true;
    }

    if (GetVelocity().x < 0.0 && was_right && vertical_overlap)
    {
        if (can_micro_step_to_top)
        {
            LandOnSurface(platform_top);
            return true;
        }

        HitWall(platform_right, -1);
        return true;
    }

    const double overlap_bottom = my_box.Top() - other_box.Bottom();
    const double overlap_top    = other_box.Top() - my_box.Bottom();
    const double overlap_left   = my_box.Right() - other_box.Left();
    const double overlap_right  = other_box.Right() - my_box.Left();

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
            // Push below only when this was actually a ceiling hit.
            if (velocityY > 0.0 && prev_top <= platform_bottom)
            {
                HitCeiling(platform_bottom);
            }
            else if (was_left)
            {
                HitWall(platform_left, 1);
            }
            else if (was_right)
            {
                HitWall(platform_right, -1);
            }
            break;

        case 1:
            // Push onto the top only when this was actually a landing.
            if (velocityY <= 0.0 && prev_bottom >= platform_top)
            {
                LandOnSurface(platform_top);
            }
            else if (was_left)
            {
                HitWall(platform_left, 1);
            }
            else if (was_right)
            {
                HitWall(platform_right, -1);
            }
            break;

        case 2: HitWall(platform_left, 1); break;

        case 3: HitWall(platform_right, -1); break;
    }

    return true;
}

void Player::ResolveCollision(GameObject* other_object)
{
    auto  textManager = Engine::GetGameStateManager().GetGSComponent<WorldTextManager>();
    auto& input       = Engine::GetInput();

    if (other_object->Type() == GameObjectTypes::Floor || other_object->Type() == GameObjectTypes::Gate || other_object->Type() == GameObjectTypes::FallingBlock)
    {
        CS230::RectCollision* my_collider = GetGOComponent<CS230::RectCollision>();
        if (!my_collider)
            return;

        Math::rect other_box;
        Polygon    floor_poly;
        bool       has_floor_poly = false;

        if (other_object->Type() == GameObjectTypes::Floor)
        {
            CS230::SATCollision* floor_collider = other_object->GetGOComponent<CS230::SATCollision>();
            if (!floor_collider)
                return;

            floor_poly     = floor_collider->WorldBoundary();
            has_floor_poly = true;
            other_box      = floor_poly.FindBoundary();
        }
        else if (other_object->Type() == GameObjectTypes::Gate)
        {
            CS230::RectCollision* gate_collider = other_object->GetGOComponent<CS230::RectCollision>();
            if (!gate_collider)
                return;

            other_box = gate_collider->WorldBoundary();
        }
        else if (other_object->Type() == GameObjectTypes::FallingBlock)
        {
            CS230::RectCollision* block_collider = other_object->GetGOComponent<CS230::RectCollision>();
            if (!block_collider)
                return;

            other_box = block_collider->WorldBoundary();
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

        auto nearly_equal = [](double a, double b)
        {
            constexpr double EPS = 0.01;
            return std::abs(a - b) < EPS;
        };

        auto is_axis_aligned_rect_floor = [&]()
        {
            if (other_object->Type() != GameObjectTypes::Floor || !has_floor_poly)
                return false;

            std::vector<Math::vec2> unique_vertices;

            for (const Math::vec2& v : floor_poly.vertices)
            {
                bool already_exists = false;

                for (const Math::vec2& u : unique_vertices)
                {
                    if (nearly_equal(v.x, u.x) && nearly_equal(v.y, u.y))
                    {
                        already_exists = true;
                        break;
                    }
                }

                if (!already_exists)
                {
                    unique_vertices.push_back(v);
                }
            }

            if (unique_vertices.size() != 4)
                return false;

            for (const Math::vec2& v : unique_vertices)
            {
                const bool on_left_or_right = nearly_equal(v.x, platform_left) || nearly_equal(v.x, platform_right);

                const bool on_bottom_or_top = nearly_equal(v.y, platform_bottom) || nearly_equal(v.y, platform_top);

                if (!on_left_or_right || !on_bottom_or_top)
                    return false;
            }

            return true;
        };

        const bool can_use_aabb_fallback = other_object->Type() == GameObjectTypes::Gate || other_object->Type() == GameObjectTypes::FallingBlock || is_axis_aligned_rect_floor();

        bool horizontal_overlap = my_box.Right() > other_box.Left() && my_box.Left() < other_box.Right();
        bool vertical_overlap   = my_box.Top() > other_box.Bottom() && my_box.Bottom() < other_box.Top();

        if (!horizontal_overlap || !vertical_overlap)
            return;

        if (other_object->Type() == GameObjectTypes::Floor && has_floor_poly && !can_use_aabb_fallback)
        {
            if (ResolveFloorVerticalWallCollision(floor_poly, my_box, prev_left, prev_right))
            {
                return;
            }

            if (ResolveFloorDiagonalWallCollision(floor_poly, my_box, prev_left, prev_right, prev_bottom, prev_top))
            {
                return;
            }

            if (ResolveFloorVertexSideCollision(floor_poly, my_box, prev_left, prev_right, prev_bottom, prev_top))
            {
                return;
            }

            if (ResolveFloorSurfaceSnap(floor_poly, my_box, prev_bottom))
            {
                return;
            }

            if (ResolveFloorCeilingCollision(floor_poly, my_box, prev_top))
            {
                return;
            }

            return;
        }

        ResolveAABBFallback(my_box, other_box, prev_bottom, prev_top, prev_left, prev_right);

        return;
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
