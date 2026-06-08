#include "LaserCutRope.hpp"

#include "CS200/IRenderer2D.hpp"
#include <algorithm>
#include <cmath>
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
        return;

    auto&            renderer = Engine::GetRenderer2D();
    const Math::vec2 pos      = GetPosition();
    const double     top      = pos.y + size.y * 0.5;
    const double     bottom   = pos.y - size.y * 0.5;

    // Chain links along the rope height
    constexpr double LINK_H   = 14.0;
    constexpr double LINK_W   = 7.0;
    const int        numLinks = std::max(1, static_cast<int>(size.y / LINK_H));

    for (int i = 0; i < numLinks; ++i)
    {
        const double t    = (i + 0.5) / static_cast<double>(numLinks);
        const double linkY = bottom + size.y * (1.0 - t);

        // Alternate horizontal / vertical oval links
        const double lw = (i % 2 == 0) ? LINK_W : LINK_W * 0.6;
        const double lh = LINK_H * 0.55;
        const auto   mat = Math::TranslationMatrix(Math::vec2{ pos.x, linkY })
                         * Math::ScaleMatrix(Math::vec2{ lw * 2.0, lh * 2.0 });
        renderer.DrawCircle(mat, CS200::CLEAR, 0xB0A080FFu, 1.5);
    }

    // Center spine line
    renderer.DrawLine({ pos.x, top }, { pos.x, bottom }, 0x80706050u, 1.0);

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