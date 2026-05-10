#include "BossLaser.hpp"

#include "BossConfig.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"

#include <algorithm>
#include <cmath>

namespace Boss
{
    BossLaser::BossLaser(
        Math::vec2 start,
        Math::vec2 in_direction,
        LaserType type,
        LaserSource source,
        double length,
        double warningTime,
        double activeTime)
        : CS230::GameObject(start),
          startPosition(start),
          direction(in_direction.LengthSquared() > 0.0 ? in_direction.Normalize() : Math::vec2{ 1.0, 0.0 }),
          laserType(type),
          laserSource(source),
          laserLength(length),
          warningDuration(warningTime),
          activeDuration(activeTime)
    {
        if (warningDuration <= 0.0)
        {
            laserState = LaserState::Active;
        }
    }

    void BossLaser::Update(double dt)
    {
        if (laserState == LaserState::Expired)
        {
            return;
        }

        stateTimer += dt;

        if (laserState == LaserState::Warning)
        {
            if (stateTimer >= warningDuration)
            {
                laserState = LaserState::Active;
                stateTimer = 0.0;
            }
        }
        else if (laserState == LaserState::Active)
        {
            if (activeDuration > 0.0 && stateTimer >= activeDuration)
            {
                Expire();
            }
        }

        CS230::GameObject::Update(dt);
    }

    void BossLaser::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
    {
        if (laserState == LaserState::Expired || !IsVisible())
        {
            return;
        }

        auto& renderer = Engine::GetRenderer2D();

        const Math::vec2 endPosition = GetEndPosition();

        if (laserState == LaserState::Warning)
        {
            renderer.DrawLine(startPosition, endPosition, GetWarningColor(), 3.0);
        }
        else if (laserState == LaserState::Active)
        {
            renderer.DrawLine(startPosition, endPosition, CS200::WHITE, GetDrawWidth() + 4.0);
            renderer.DrawLine(startPosition, endPosition, GetDrawColor(), GetDrawWidth());
        }
    }

    void BossLaser::SetStartPosition(Math::vec2 start)
    {
        startPosition = start;
        SetPosition(start);
    }

    void BossLaser::SetDirection(Math::vec2 newDirection)
    {
        if (newDirection.LengthSquared() <= 0.0)
        {
            return;
        }

        direction = newDirection.Normalize();
    }

    Math::vec2 BossLaser::GetStartPosition() const
    {
        return startPosition;
    }

    Math::vec2 BossLaser::GetDirection() const
    {
        return direction;
    }

    Math::vec2 BossLaser::GetEndPosition() const
    {
        return startPosition + direction * laserLength;
    }

    LaserType BossLaser::GetLaserType() const
    {
        return laserType;
    }

    LaserSource BossLaser::GetLaserSource() const
    {
        return laserSource;
    }

    LaserState BossLaser::GetLaserState() const
    {
        return laserState;
    }

    bool BossLaser::IsWarning() const
    {
        return laserState == LaserState::Warning;
    }

    bool BossLaser::IsActiveLaser() const
    {
        return laserState == LaserState::Active;
    }

    bool BossLaser::IsExpired() const
    {
        return laserState == LaserState::Expired;
    }

    bool BossLaser::CanParry() const
    {
        switch (laserType)
        {
            case LaserType::Yellow:
                return false;

            case LaserType::Orange:
                return false;

            case LaserType::RedTracking:
                return false;

            case LaserType::ShortParry:
                return true;
        }

        return false;
    }

    bool BossLaser::CreatesLightOrb() const
    {
        switch (laserType)
        {
            case LaserType::Yellow:
                return true;

            case LaserType::Orange:
            case LaserType::RedTracking:
            case LaserType::ShortParry:
                return false;
        }

        return false;
    }

    double BossLaser::GetDamage() const
    {
        switch (laserType)
        {
            case LaserType::Yellow:
                return Config::YellowLaserDamage;

            case LaserType::Orange:
                return Config::OrangeLaserDamage;

            case LaserType::RedTracking:
                return Config::RedTrackingLaserDamage;

            case LaserType::ShortParry:
                return Config::ShortParryLaserDamage;
        }

        return 0.0;
    }

    float BossLaser::GetParryEnergyGain() const
    {
        if (laserType == LaserType::ShortParry)
        {
            return Config::ShortParryEnergyGain;
        }

        return 0.0f;
    }

    bool BossLaser::CollidesWithPoint(Math::vec2 point, double radius) const
    {
        if (laserState != LaserState::Active)
        {
            return false;
        }

        const double width = GetDrawWidth();
        const double hitRadius = radius + width * 0.5;

        return DistanceToSegmentSquared(point, startPosition, GetEndPosition()) <= hitRadius * hitRadius;
    }

    bool BossLaser::GetGroundIntersection(Math::vec2& out_position) const
    {
        const Math::vec2 endPosition = GetEndPosition();

        const double y1 = startPosition.y;
        const double y2 = endPosition.y;
        const double groundY = static_cast<double>(Config::GroundY);

        if ((groundY < std::min(y1, y2)) || (groundY > std::max(y1, y2)))
        {
            return false;
        }

        const double dy = y2 - y1;

        if (std::abs(dy) <= 0.0001)
        {
            return false;
        }

        const double t = (groundY - y1) / dy;

        if (t < 0.0 || t > 1.0)
        {
            return false;
        }

        out_position = startPosition + (endPosition - startPosition) * t;
        return true;
    }

    bool BossLaser::HasSpawnedLightOrb() const
    {
        return hasSpawnedLightOrb;
    }

    void BossLaser::MarkLightOrbSpawned()
    {
        hasSpawnedLightOrb = true;
    }

    void BossLaser::Expire()
    {
        laserState = LaserState::Expired;
        SetIsActive(false);
        SetVisible(false);
        Destroy();
    }

    double BossLaser::DistanceToSegmentSquared(Math::vec2 point, Math::vec2 a, Math::vec2 b)
    {
        const double lengthSquared = (b - a).LengthSquared();

        if (lengthSquared <= 0.0)
        {
            return (point - a).LengthSquared();
        }

        const double t = std::clamp((point - a).Dot(b - a) / lengthSquared, 0.0, 1.0);
        const Math::vec2 projection = a + (b - a) * t;

        return (point - projection).LengthSquared();
    }

    CS200::RGBA BossLaser::GetDrawColor() const
    {
        switch (laserType)
        {
            case LaserType::Yellow:
                return 0xFFFF00FF;

            case LaserType::Orange:
                return 0xFF9900FF;

            case LaserType::RedTracking:
                return 0xFF0000FF;

            case LaserType::ShortParry:
                return 0xFF3333FF;
        }

        return CS200::WHITE;
    }

    CS200::RGBA BossLaser::GetWarningColor() const
    {
        switch (laserType)
        {
            case LaserType::Yellow:
                return 0xFFFF88FF;

            case LaserType::Orange:
                return 0xFFCC66FF;

            case LaserType::RedTracking:
                return 0xFF7777FF;

            case LaserType::ShortParry:
                return 0xFFAAAAFF;
        }

        return CS200::WHITE;
    }

    double BossLaser::GetDrawWidth() const
    {
        switch (laserType)
        {
            case LaserType::Yellow:
                return 16.0;

            case LaserType::Orange:
                return 14.0;

            case LaserType::RedTracking:
                return 24.0;

            case LaserType::ShortParry:
                return 12.0;
        }

        return 12.0;
    }
}