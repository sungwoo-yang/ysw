#include "Ramp.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Matrix.hpp"
#include <algorithm>
#include <cmath>

namespace
{
    void FillTriangle(CS200::IRenderer2D& r,
                      Math::vec2 a, Math::vec2 b, Math::vec2 c,
                      uint32_t color)
    {
        if (a.y > b.y) std::swap(a, b);
        if (a.y > c.y) std::swap(a, c);
        if (b.y > c.y) std::swap(b, c);
        if (c.y - a.y < 0.5) return;

        const double step = 2.0;
        for (double y = a.y; y <= c.y; y += step)
        {
            const double tAC = (c.y > a.y) ? (y - a.y) / (c.y - a.y) : 1.0;
            const double xAC = a.x + tAC * (c.x - a.x);
            double xOther;
            if (y <= b.y) {
                const double tAB = (b.y > a.y) ? (y - a.y) / (b.y - a.y) : 1.0;
                xOther = a.x + tAB * (b.x - a.x);
            } else {
                const double tBC = (c.y > b.y) ? (y - b.y) / (c.y - b.y) : 1.0;
                xOther = b.x + tBC * (c.x - b.x);
            }
            r.DrawLine({ std::min(xAC, xOther), y },
                       { std::max(xAC, xOther), y },
                       color, step);
        }
    }
}

Ramp::Ramp(Math::vec2 pos, Math::vec2 in_size, Dir dir)
    : CS230::GameObject(pos)
    , size(in_size)
    , direction(dir)
{}

void Ramp::Draw(const Math::TransformationMatrix& /*camera_matrix*/)
{
    auto&            r   = Engine::GetRenderer2D();
    const Math::vec2 pos = GetPosition();
    const double     hw  = size.x * 0.5;
    const double     hh  = size.y * 0.5;

    // Triangle corners:
    //  UpRight: bottom-left, bottom-right, top-right
    //  UpLeft:  bottom-left, bottom-right, top-left
    Math::vec2 bl = { pos.x - hw, pos.y - hh };
    Math::vec2 br = { pos.x + hw, pos.y - hh };
    Math::vec2 tip = (direction == Dir::UpRight)
        ? Math::vec2{ pos.x + hw, pos.y + hh }
        : Math::vec2{ pos.x - hw, pos.y + hh };

    FillTriangle(r, bl, br, tip, 0x131313FF);

    // Slope edge highlight (the hypotenuse)
    r.DrawLine(bl, tip,  0x706050FFu, 1.5);
    r.DrawLine(br, tip,  0x706050FFu, 1.5);
    r.DrawLine(bl, br,   0x504848FFu, 1.0);
}
