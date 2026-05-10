#pragma once

#include "Engine/Vec2.hpp"
#include "ShieldEnergy.hpp"

#include <memory>
#include <vector>

class Player;

namespace CS230
{
    class GameObjectManager;
}

namespace Boss
{
    class LightOrb;

    class LightOrbManager
    {
    public:
        LightOrbManager(Player* in_player, ShieldEnergy* in_shieldEnergy);

        void SetGameObjectManager(CS230::GameObjectManager* in_gom);

        void Update(double dt);

        bool TrySpawn(Math::vec2 position);
        void Clear();

        int GetActiveOrbCount() const;

    private:
        bool CanSpawnAt(Math::vec2 position) const;
        void RemoveCollectedOrDestroyedOrbs();

        Player* player = nullptr;
        ShieldEnergy* shieldEnergy = nullptr;
        CS230::GameObjectManager* gom = nullptr;

        std::vector<LightOrb*> lightOrbs;

        double spawnCooldownTimer = 0.0;
    };
}