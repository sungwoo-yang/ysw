#include "Staircase.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include <algorithm>

Staircase::Staircase(Math::vec2 pos, Math::vec2 in_size, int steps, Dir dir)
    : CS230::GameObject(pos)
    , size(in_size)
    , stepCount(std::max(2, steps))
    , direction(dir)
{}

void Staircase::Draw(const Math::TransformationMatrix& /*camera_matrix*/)
{
    auto&            renderer = Engine::GetRenderer2D();
    const Math::vec2 pos      = GetPosition();
    const double     hw       = size.x * 0.5;
    const double     hh       = size.y * 0.5;
    const double     stepW    = size.x / stepCount;
    const double     stepH    = size.y / stepCount;
    const double     left     = pos.x - hw;
    const double     bottom   = pos.y - hh;

    for (int i = 0; i < stepCount; ++i)
    {
        const double x0 = left + i * stepW;

        // Step top Y: ascending right or ascending left
        const double y_top = (direction == Dir::UpRight)
            ? bottom + (i + 1) * stepH
            : bottom + (stepCount - i) * stepH;

        // Draw each step column from bottom up to its surface
        const double blockH      = y_top - bottom;
        const Math::vec2 blockCenter = { x0 + stepW * 0.5, bottom + blockH * 0.5 };
        const auto mat = Math::TranslationMatrix(blockCenter)
                       * Math::ScaleMatrix(Math::vec2{ stepW - 1.0, blockH });
        renderer.DrawRectangle(mat, 0x38322CFF, 0x706050FF, 1.5);

        // Highlight on top edge (step surface)
        renderer.DrawLine({ x0 + 2.0,         y_top },
                          { x0 + stepW - 2.0,  y_top }, 0x908070FFu, 1.5);

        // Vertical riser line between steps (except last)
        if (i + 1 < stepCount)
        {
            const double riserX  = x0 + stepW;
            const double nextTop = (direction == Dir::UpRight)
                ? bottom + (i + 2) * stepH
                : bottom + (stepCount - i - 1) * stepH;
            renderer.DrawLine({ riserX, y_top   },
                              { riserX, nextTop }, 0x504840FFu, 1.0);
        }
    }
}
