#pragma once

#include "Engine/GameObjectTypes.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Vec2.hpp"

#include <vector>

class Player;
class TargetStar;

namespace Boss
{
    class ShieldEnergy;

    class ShieldChargeShot
    {
    public:
        ShieldChargeShot(Player* in_player, ShieldEnergy* in_shieldEnergy);

        void SetTargetStars(const std::vector<TargetStar*>& in_targetStars);

        void Update(double dt);
        void Draw(const Math::TransformationMatrix& camera_matrix);

        bool  IsCharging() const;
        bool  IsReadyToFire() const;
        float GetChargeRatio() const;

    private:
        void StartCharging();
        void CancelCharging();
        void Fire();

        Math::vec2 GetMouseWorldPosition() const;
        Math::vec2 ClipEndByBlockingObjects(Math::vec2 start, Math::vec2 end) const;

        TargetStar* FindFirstHitTarget(Math::vec2 start, Math::vec2 end) const;

        static bool IsChargeShotBlocker(GameObjectTypes type);
        static bool SegmentIntersectsRect(Math::vec2 start, Math::vec2 end, const Math::rect& rect, double& out_t);

        static double DistanceToSegmentSquared(Math::vec2 point, Math::vec2 a, Math::vec2 b);

        Player*       player       = nullptr;
        ShieldEnergy* shieldEnergy = nullptr;

        std::vector<TargetStar*> targetStars;

        bool isCharging  = false;
        bool readyToFire = false;

        double chargeTimer = 0.0;

        bool   beamVisible = false;
        double beamTimer   = 0.0;

        Math::vec2 beamStart = { 0.0, 0.0 };
        Math::vec2 beamEnd   = { 0.0, 0.0 };
    };
}