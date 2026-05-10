#pragma once

#include "BossTypes.hpp"

#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"

class Player;

namespace Boss
{
    class BossLaser : public CS230::GameObject
    {
    public:
        BossLaser(
            Math::vec2 start,
            Math::vec2 direction,
            LaserType type,
            LaserSource source,
            double length,
            double warningTime,
            double activeTime);

        void Update(double dt) override;
        void Draw(const Math::TransformationMatrix& camera_matrix) override;

        GameObjectTypes Type() override
        {
            return GameObjectTypes::Laser;
        }

        std::string TypeName() override
        {
            return "BossLaser";
        }

        void SetStartPosition(Math::vec2 start);
        void SetDirection(Math::vec2 newDirection);

        Math::vec2 GetStartPosition() const;
        Math::vec2 GetDirection() const;
        Math::vec2 GetEndPosition() const;

        LaserType GetLaserType() const;
        LaserSource GetLaserSource() const;
        LaserState GetLaserState() const;

        bool IsWarning() const;
        bool IsActiveLaser() const;
        bool IsExpired() const;

        bool CanParry() const;
        bool CreatesLightOrb() const;
        double GetDamage() const;
        float GetParryEnergyGain() const;

        bool CollidesWithPoint(Math::vec2 point, double radius) const;
        bool GetGroundIntersection(Math::vec2& out_position) const;

        bool HasSpawnedLightOrb() const;
        void MarkLightOrbSpawned();

        void Expire();

    private:
        static double DistanceToSegmentSquared(Math::vec2 point, Math::vec2 a, Math::vec2 b);

        CS200::RGBA GetDrawColor() const;
        CS200::RGBA GetWarningColor() const;
        double GetDrawWidth() const;

        Math::vec2 startPosition;
        Math::vec2 direction;

        LaserType laserType;
        LaserSource laserSource;
        LaserState laserState = LaserState::Warning;

        double laserLength = 3000.0;
        double warningDuration = 0.0;
        double activeDuration = 0.0;

        double stateTimer = 0.0;

        bool hasSpawnedLightOrb = false;
    };
}