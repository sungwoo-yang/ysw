#include "BossLaserManager.hpp"

#include "LightOrbManager.hpp"
#include "Player.hpp"
#include "Shield.hpp"

#include <algorithm>

namespace Boss
{
    BossLaserManager::BossLaserManager(Player* in_player, ShieldEnergy* in_shieldEnergy, LightOrbManager* in_lightOrbManager)
        : player(in_player), shieldEnergy(in_shieldEnergy), lightOrbManager(in_lightOrbManager)
    {
    }

    void BossLaserManager::Update(double dt)
    {
        Shield* shield                = player != nullptr ? player->GetShield() : nullptr;
        bool    shouldOpenParryWindow = false;

        for (BossLaser* laser : lasers)
        {
            if (laser == nullptr)
            {
                continue;
            }

            laser->Update(dt);

            if (!laser->IsExpired() && laser->CanParry())
            {
                shouldOpenParryWindow = true;
            }

            if (laser->IsActiveLaser())
            {
                HandleLightOrbSpawn(*laser);
                HandlePlayerCollision(*laser);
            }
        }

        if (shield != nullptr)
        {
            shield->SetParryWindowActive(shouldOpenParryWindow);
        }

        RemoveExpiredLasers();
    }

    void BossLaserManager::Draw(const Math::TransformationMatrix& camera_matrix)
    {
        for (BossLaser* laser : lasers)
        {
            if (laser != nullptr && !laser->IsExpired())
            {
                laser->Draw(camera_matrix);
            }
        }
    }

    BossLaser* BossLaserManager::SpawnLaser(Math::vec2 start, Math::vec2 direction, LaserType type, LaserSource source, double length, double warningTime, double activeTime)
    {
        BossLaser* laser = new BossLaser(start, direction, type, source, length, warningTime, activeTime);
        lasers.push_back(laser);

        return laser;
    }

    void BossLaserManager::Clear()
    {
        Shield* shield = player != nullptr ? player->GetShield() : nullptr;

        if (shield != nullptr)
        {
            shield->SetParryWindowActive(false);
        }

        for (BossLaser* laser : lasers)
        {
            if (laser != nullptr)
            {
                laser->Expire();
                delete laser;
            }
        }

        lasers.clear();
    }

    int BossLaserManager::GetActiveLaserCount() const
    {
        int count = 0;

        for (BossLaser* laser : lasers)
        {
            if (laser != nullptr && !laser->IsExpired())
            {
                ++count;
            }
        }

        return count;
    }

    void BossLaserManager::HandlePlayerCollision(BossLaser& laser)
    {
        if (player == nullptr)
        {
            return;
        }

        constexpr double PlayerLaserHitRadius = 32.0;

        if (!laser.CollidesWithPoint(player->GetPosition(), PlayerLaserHitRadius))
        {
            return;
        }

        Shield* shield = player->GetShield();

        if (laser.CanParry() && shield != nullptr && shield->ConsumeParryState())
        {
            shield->HandleHit(true);

            if (shieldEnergy != nullptr)
            {
                shieldEnergy->AddEnergy(laser.GetParryEnergyGain());
            }

            laser.Expire();
            return;
        }

        player->ApplyLaserDamage(laser.GetDamage());
    }

    void BossLaserManager::HandleLightOrbSpawn(BossLaser& laser)
    {
        if (lightOrbManager == nullptr)
        {
            return;
        }

        if (!laser.CreatesLightOrb())
        {
            return;
        }

        if (laser.HasSpawnedLightOrb())
        {
            return;
        }

        Math::vec2 spawnPosition;

        if (laser.GetGroundIntersection(spawnPosition))
        {
            if (lightOrbManager->TrySpawn(spawnPosition))
            {
                laser.MarkLightOrbSpawned();
            }
        }
    }

    void BossLaserManager::RemoveExpiredLasers()
    {
        lasers.erase(
            std::remove_if(
                lasers.begin(), lasers.end(),
                [](BossLaser* laser)
                {
                    if (laser == nullptr)
                    {
                        return true;
                    }

                    if (laser->IsExpired() || laser->Destroyed())
                    {
                        delete laser;
                        return true;
                    }

                    return false;
                }),
            lasers.end());
    }
}