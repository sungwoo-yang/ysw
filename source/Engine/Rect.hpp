/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include "Vec2.hpp"
#include <algorithm>

namespace Math
{
    struct [[nodiscard]] rect
    {
        Math::vec2 point_1{ 0.0, 0.0 };
        Math::vec2 point_2{ 0.0, 0.0 };

        double Left() const noexcept
        {
            return std::min(point_1.x, point_2.x);
        }

        double Right() const noexcept
        {
            return std::max(point_1.x, point_2.x);
        }

        double Top() const noexcept
        {
            return std::max(point_1.y, point_2.y);
        }

        double Bottom() const noexcept
        {
            return std::min(point_1.y, point_2.y);
        }

        Math::vec2 Size() const noexcept
        {
            return { Right() - Left(), Top() - Bottom() };
        }
    };

    struct [[nodiscard]] irect
    {
        Math::ivec2 point_1{ 0, 0 };
        Math::ivec2 point_2{ 0, 0 };

        int Left() const noexcept
        {
            return std::min(point_1.x, point_2.x);
        }

        int Right() const noexcept
        {
            return std::max(point_1.x, point_2.x);
        }

        int Top() const noexcept
        {
            return std::max(point_1.y, point_2.y);
        }

        int Bottom() const noexcept
        {
            return std::min(point_1.y, point_2.y);
        }

        Math::ivec2 Size() const noexcept
        {
            return { Right() - Left(), Top() - Bottom() };
        }
    };
}
