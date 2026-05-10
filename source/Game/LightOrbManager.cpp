#include "LightOrbManager.hpp"

#include "BossConfig.hpp"
#include "Engine/GameObjectManager.hpp"
#include "LightOrb.hpp"

#include <algorithm>

namespace Boss
{
    LightOrbManager::LightOrbManager(Player* in_player, ShieldEnergy* in_shieldEnergy)
        : player(in_player), shieldEnergy(in_shieldEnergy)
    {
    }

    void LightOrbManager::SetGameObjectManager(CS230::GameObjectManager* in_gom)
    {
        gom = in_gom;
    }

    void LightOrbManager::Update(double dt)
    {
        if (spawnCooldownTimer > 0.0)
        {
            spawnCooldownTimer -= dt;

            if (spawnCooldownTimer < 0.0)
            {
                spawnCooldownTimer = 0.0;
            }
        }

        for (LightOrb* orb : lightOrbs)
        {
            if (orb == nullptr)
            {
                continue;
            }

            if (orb->IsCollected() && shieldEnergy != nullptr)
            {
                shieldEnergy->AddEnergy(Config::LightOrbEnergyGain);
            }
        }

        RemoveCollectedOrDestroyedOrbs();
    }

    bool LightOrbManager::TrySpawn(Math::vec2 position)
    {
        RemoveCollectedOrDestroyedOrbs();

        if (gom == nullptr || player == nullptr)
        {
            return false;
        }

        if (spawnCooldownTimer > 0.0)
        {
            return false;
        }

        if (GetActiveOrbCount() >= Config::MaxLightOrbCount)
        {
            return false;
        }

        if (!CanSpawnAt(position))
        {
            return false;
        }

        LightOrb* orb = new LightOrb(position);
        orb->SetPlayer(player);

        lightOrbs.push_back(orb);
        gom->Add(orb);

        spawnCooldownTimer = Config::LightOrbSpawnCooldown;

        return true;
    }

    void LightOrbManager::Clear()
    {
        for (LightOrb* orb : lightOrbs)
        {
            if (orb != nullptr)
            {
                orb->Destroy();
            }
        }

        lightOrbs.clear();
        spawnCooldownTimer = 0.0;
    }

    int LightOrbManager::GetActiveOrbCount() const
    {
        int count = 0;

        for (LightOrb* orb : lightOrbs)
        {
            if (orb != nullptr && !orb->Destroyed() && !orb->IsCollected())
            {
                ++count;
            }
        }

        return count;
    }

    bool LightOrbManager::CanSpawnAt(Math::vec2 position) const
    {
        const double minDistance = static_cast<double>(Config::MinLightOrbSpawnDistance);
        const double minDistanceSq = minDistance * minDistance;

        for (LightOrb* orb : lightOrbs)
        {
            if (orb == nullptr || orb->Destroyed() || orb->IsCollected())
            {
                continue;
            }

            const double distanceSq = (orb->GetPosition() - position).LengthSquared();

            if (distanceSq < minDistanceSq)
            {
                return false;
            }
        }

        return true;
    }

    void LightOrbManager::RemoveCollectedOrDestroyedOrbs()
    {
        lightOrbs.erase(
            std::remove_if(
                lightOrbs.begin(),
                lightOrbs.end(),
                [](LightOrb* orb)
                {
                    return orb == nullptr || orb->Destroyed() || orb->IsCollected();
                }),
            lightOrbs.end());
    }
}