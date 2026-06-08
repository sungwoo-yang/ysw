#include "Elevator.hpp"
#include "Player.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Matrix.hpp"

#include <cmath>

namespace
{
    constexpr double TWO_PI           = 6.28318530717958647692;
    constexpr double PLAYER_HALF_H    = 40.0;
    constexpr double PLAYER_HALF_W    = 20.0;
    constexpr double RIDER_FOOT_SLACK = 6.0;
    constexpr double RIDER_FALL_MAX   = 60.0;
}

Elevator::Elevator(Math::vec2 center, Math::vec2 in_size, double in_travelDist, double periodSecs)
    : CS230::GameObject(center)
    , size(in_size)
    , startPos(center)
    , travelDist(in_travelDist)
    , period(periodSecs > 0.01 ? periodSecs : 4.0)
{
    const Math::irect box{
        { static_cast<int>(-size.x * 0.5), static_cast<int>(-size.y * 0.5) },
        { static_cast<int>( size.x * 0.5), static_cast<int>( size.y * 0.5) }
    };
    AddGOComponent(new CS230::RectCollision(box, this));
}

void Elevator::Update(double dt)
{
    const double prevCenterY = GetPosition().y;

    phase = std::fmod(phase + (TWO_PI / period) * dt, TWO_PI);
    const double t    = (1.0 - std::cos(phase)) * 0.5;
    const double newY = startPos.y + t * travelDist;
    const double deltaY = newY - prevCenterY;

    // Carry rider before position update (uses prev-frame elevator top)
    if (ridingPlayer != nullptr)
    {
        const double prevElevTop  = prevCenterY + size.y * 0.5;
        const double playerFoot   = ridingPlayer->GetPosition().y - PLAYER_HALF_H;
        const double elevLeft     = GetPosition().x - size.x * 0.5;
        const double elevRight    = GetPosition().x + size.x * 0.5;
        const double playerLeft   = ridingPlayer->GetPosition().x - PLAYER_HALF_W;
        const double playerRight  = ridingPlayer->GetPosition().x + PLAYER_HALF_W;

        const bool horizOverlap = playerRight > elevLeft && playerLeft < elevRight;
        const double footDiff   = playerFoot - prevElevTop;

        // Allow slack proportional to elevator speed so fast elevators keep the rider
        const double speedSlack = std::abs(deltaY) + RIDER_FOOT_SLACK;

        if (!horizOverlap || footDiff > speedSlack || footDiff < -RIDER_FALL_MAX)
        {
            ridingPlayer = nullptr;
        }
        else
        {
            ridingPlayer->SetPosition({
                ridingPlayer->GetPosition().x,
                ridingPlayer->GetPosition().y + deltaY
            });
        }
    }

    SetVelocity({ 0.0, 0.0 });
    SetPosition({ startPos.x, newY });
    CS230::GameObject::Update(dt);
}

void Elevator::Draw(const Math::TransformationMatrix& /*camera_matrix*/)
{
    auto&            r   = Engine::GetRenderer2D();
    const Math::vec2 pos = GetPosition();
    const double     hw  = size.x * 0.5;
    const double     hh  = size.y * 0.5;

    // Body
    const auto bodyMat = GetMatrix() * Math::ScaleMatrix(size);
    r.DrawRectangle(bodyMat, 0x1A2844FF, 0x3366BBFF, 2.0);

    // Top surface highlight
    r.DrawLine({ pos.x - hw, pos.y + hh },
               { pos.x + hw, pos.y + hh },
               0x66BBFFFF, 8.0);

    // Bottom edge
    r.DrawLine({ pos.x - hw, pos.y - hh },
               { pos.x + hw, pos.y - hh },
               0x2244AAFF, 3.0);

    // Side accent ticks (left/right pillars)
    r.DrawLine({ pos.x - hw + 4.0, pos.y + hh - 10.0 },
               { pos.x - hw + 4.0, pos.y - hh + 6.0 },
               0x334477FFu, 3.0);
    r.DrawLine({ pos.x + hw - 4.0, pos.y + hh - 10.0 },
               { pos.x + hw - 4.0, pos.y - hh + 6.0 },
               0x334477FFu, 3.0);
}
