#include "LaserCutRope.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "FallingBlock.hpp"

LaserCutRope::LaserCutRope(Math::vec2 in_position, Math::vec2 in_size) : CS230::GameObject(in_position), size(in_size)
{
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };

    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void LaserCutRope::Update(double dt)
{
    if (isCut)
    {
        return;
    }

    CS230::GameObject::Update(dt);
}

void LaserCutRope::Draw(const Math::TransformationMatrix& camera_matrix)
{
    if (isCut)
    {
        return;
    }

    auto& renderer = Engine::GetRenderer2D();

    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(size);

    renderer.DrawRectangle(transform, 0x00CC00FF, CS200::WHITE, 1.0);

    CS230::GameObject::Draw(camera_matrix);
}

void LaserCutRope::Cut()
{
    if (isCut)
    {
        return;
    }

    isCut = true;
    RemoveGOComponent<CS230::RectCollision>();

    auto gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

    if (gom != nullptr)
    {
        const double ropeLeft   = GetPosition().x - size.x * 0.5;
        const double ropeRight  = GetPosition().x + size.x * 0.5;
        const double ropeBottom = GetPosition().y - size.y * 0.5;

        FallingBlock* bestBlock = nullptr;
        double        bestGap   = 999999.0;

        for (auto obj : gom->GetObjects())
        {
            if (obj == nullptr || obj->Type() != GameObjectTypes::FallingBlock)
            {
                continue;
            }

            auto block = static_cast<FallingBlock*>(obj);

            if (block->IsReleased())
            {
                continue;
            }

            CS230::RectCollision* blockCollider = block->GetGOComponent<CS230::RectCollision>();

            if (blockCollider == nullptr)
            {
                continue;
            }

            Math::rect blockBox = blockCollider->WorldBoundary();

            const bool horizontalOverlap = blockBox.Right() >= ropeLeft && blockBox.Left() <= ropeRight;

            const bool blockIsBelowRope = blockBox.Top() <= ropeBottom + 20.0;

            if (!horizontalOverlap || !blockIsBelowRope)
            {
                continue;
            }

            const double gap = ropeBottom - blockBox.Top();

            if (gap >= -20.0 && gap < bestGap)
            {
                bestGap   = gap;
                bestBlock = block;
            }
        }

        if (bestBlock != nullptr)
        {
            bestBlock->Release();
        }
    }

    Engine::GetLogger().LogEvent("LaserCutRope cut!");
}

bool LaserCutRope::IsCut() const
{
    return isCut;
}