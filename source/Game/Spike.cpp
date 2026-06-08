#include "Spike.hpp"
#include "Player.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
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

        const double step = 1.5;
        for (double y = a.y; y <= c.y; y += step)
        {
            const double tAC   = (c.y > a.y) ? (y - a.y) / (c.y - a.y) : 1.0;
            const double xAC   = a.x + tAC * (c.x - a.x);
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

// pos = center of the bounding box (matches editor behavior).
// Base of each triangle at the wall-side edge of the bbox, tips at the opposite edge.
//
//  Dir::Up    : base at pos.y - hh (floor),       tips at pos.y + hh
//  Dir::Down  : base at pos.y + hh (ceiling),     tips at pos.y - hh
//  Dir::Left  : base at pos.x + hw (right wall),  tips at pos.x - hw
//  Dir::Right : base at pos.x - hw (left wall),   tips at pos.x + hw

Spike::Spike(Math::vec2 pos, Math::vec2 in_size, Dir dir)
    : CS230::GameObject(pos)
    , size(in_size)
    , direction(dir)
{
    const int hw = static_cast<int>(in_size.x * 0.5);
    const int hh = static_cast<int>(in_size.y * 0.5);
    AddGOComponent(new CS230::RectCollision({ { -hw, -hh }, { hw, hh } }, this));
}

void Spike::Draw(const Math::TransformationMatrix& /*camera_matrix*/)
{
    auto&            r   = Engine::GetRenderer2D();
    const Math::vec2 pos = GetPosition();
    const double     hw  = size.x * 0.5;
    const double     hh  = size.y * 0.5;

    const bool   isH   = (direction == Dir::Left || direction == Dir::Right);
    const double span  = isH ? size.y : size.x;
    const double depth = isH ? size.x : size.y;

    const int    count     = std::max(1, static_cast<int>(std::round(span / depth)));
    const double toothSpan = span / count;

    constexpr uint32_t FILL = 0xCC2222FF;
    constexpr uint32_t EDGE = 0xFF6666FF;

    for (int i = 0; i < count; ++i)
    {
        const double t  = -span * 0.5 + i * toothSpan;
        const double tm = t + toothSpan * 0.5;

        Math::vec2 base0, base1, tip;

        switch (direction)
        {
        case Dir::Up:
            base0 = { pos.x + t,             pos.y - hh };
            base1 = { pos.x + t + toothSpan, pos.y - hh };
            tip   = { pos.x + tm,            pos.y + hh };
            break;
        case Dir::Down:
            base0 = { pos.x + t,             pos.y + hh };
            base1 = { pos.x + t + toothSpan, pos.y + hh };
            tip   = { pos.x + tm,            pos.y - hh };
            break;
        case Dir::Left:
            base0 = { pos.x + hw, pos.y + t              };
            base1 = { pos.x + hw, pos.y + t + toothSpan  };
            tip   = { pos.x - hw, pos.y + tm              };
            break;
        case Dir::Right:
            base0 = { pos.x - hw, pos.y + t              };
            base1 = { pos.x - hw, pos.y + t + toothSpan  };
            tip   = { pos.x + hw, pos.y + tm              };
            break;
        }

        FillTriangle(r, base0, base1, tip, FILL);
        r.DrawLine(base0, tip,  EDGE, 1.0);
        r.DrawLine(base1, tip,  EDGE, 1.0);
    }
}

bool Spike::CanCollideWith(GameObjectTypes other)
{
    return other == GameObjectTypes::Player;
}

void Spike::ResolveCollision(CS230::GameObject* /*other*/)
{
    // Damage applied in Player::ResolveCollision
}
