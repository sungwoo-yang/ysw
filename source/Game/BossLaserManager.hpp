#pragma once

#include "BossLaser.hpp"
#include "Engine/Vec2.hpp"
#include "ShieldEnergy.hpp"

#include <vector>

class Player;

namespace Boss
{
    class LightOrbManager;

    class BossLaserManager
    {
    public:
        BossLaserManager(Player* in_player, ShieldEnergy* in_shieldEnergy, LightOrbManager* in_lightOrbManager);

        void Update(double dt);
        void Draw(const Math::TransformationMatrix& camera_matrix);

        BossLaser* SpawnLaser(
            Math::vec2 start,
            Math::vec2 direction,
            LaserType type,
            LaserSource source,
            double length,
            double warningTime,
            double activeTime);

        void Clear();

        int GetActiveLaserCount() const;

    private:
        void HandlePlayerCollision(BossLaser& laser);
        void HandleLightOrbSpawn(BossLaser& laser);
        void RemoveExpiredLasers();

        Player* player = nullptr;
        ShieldEnergy* shieldEnergy = nullptr;
        LightOrbManager* lightOrbManager = nullptr;

        std::vector<BossLaser*> lasers;
    };
}