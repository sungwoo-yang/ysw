#pragma once

#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

class Player;

namespace Boss
{
    class LightOrb : public CS230::GameObject
    {
    public:
        explicit LightOrb(Math::vec2 spawnPosition);

        void Update(double dt) override;
        void Draw(const Math::TransformationMatrix& camera_matrix) override;

        GameObjectTypes Type() override
        {
            return GameObjectTypes::Particle;
        }

        std::string TypeName() override
        {
            return "LightOrb";
        }

        void SetPlayer(Player* in_player);

        bool IsCollected() const;
        bool IsAttracting() const;

    private:
        Player* player = nullptr;

        bool isAttracting = false;
        bool isCollected  = false;

        double visualRadius = 14.0;
    };
}