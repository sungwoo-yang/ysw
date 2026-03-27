#include "WhiteLaser.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "TargetStar.hpp"

WhiteLaser::WhiteLaser(Math::vec2 in_startPos, Math::vec2 in_dir, Player* in_player) : Laser(in_startPos, in_dir, in_player)
{
    color = 0xFFFFFFFF;
}

void WhiteLaser::Update([[maybe_unused]] double dt)
{
    if (!isActive)
        return;

    CalculatePath(maxBounces, maxLength);
    CheckTargetIntersections(hitRadius);

    auto gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

    for (auto obj : gom->GetObjects())
    {
        if (obj->Type() == GameObjectTypes::Target)
        {
            TargetStar* target    = static_cast<TargetStar*>(obj);
            Math::vec2  targetPos = target->GetPosition();
            double      hitRad = target->GetRadius();

            for (size_t i = 0; i < pathPoints.size() - 1; ++i)
            {
                if (DistToSegmentSquared(targetPos, pathPoints[i], pathPoints[i + 1]) < hitRad * hitRad)
                {
                    target->OnHit();
                    break;
                }
            }
        }
    }
}