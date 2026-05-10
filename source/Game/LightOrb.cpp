#include "LightOrb.hpp"

#include "BossConfig.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Player.hpp"

namespace Boss
{
    LightOrb::LightOrb(Math::vec2 spawnPosition)
        : CS230::GameObject(spawnPosition)
    {
    }

    void LightOrb::SetPlayer(Player* in_player)
    {
        player = in_player;
    }

    bool LightOrb::IsCollected() const
    {
        return isCollected;
    }

    bool LightOrb::IsAttracting() const
    {
        return isAttracting;
    }

    void LightOrb::Update(double dt)
    {
        if (isCollected || player == nullptr)
        {
            CS230::GameObject::Update(dt);
            return;
        }

        const Math::vec2 toPlayer = player->GetPosition() - GetPosition();
        const double distanceSquared = toPlayer.LengthSquared();

        const double attractRadius = static_cast<double>(Config::LightOrbAttractRadius);
        const double collectRadius = static_cast<double>(Config::LightOrbCollectRadius);

        if (!isAttracting && distanceSquared <= attractRadius * attractRadius)
        {
            isAttracting = true;
        }

        if (isAttracting)
        {
            const double distance = std::sqrt(distanceSquared);

            if (distance <= collectRadius)
            {
                isCollected = true;
                Destroy();
                return;
            }

            if (distance > 0.0)
            {
                const Math::vec2 direction = toPlayer / distance;
                UpdatePosition(direction * static_cast<double>(Config::LightOrbMoveSpeed) * dt);
            }
        }

        CS230::GameObject::Update(dt);
    }

    void LightOrb::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
    {
        if (isCollected || !IsVisible())
        {
            return;
        }

        auto& renderer = Engine::GetRenderer2D();

        const double pulseScale = isAttracting ? 1.25 : 1.0;
        const double radius = visualRadius * pulseScale;

        Math::TransformationMatrix transform =
            GetMatrix() * Math::ScaleMatrix({ radius, radius });

        renderer.DrawCircle(transform, 0xFFFF66FF);
    }
}