
/**
 * \file
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Vec2.hpp"
#include <cmath>

namespace Math
{
    // vec2
    bool vec2::operator==(const vec2& v) const
    {
        return x == v.x && y == v.y;
    }

    bool vec2::operator!=(const vec2& v) const
    {
        return x != v.x || y != v.y;
    }

    vec2 vec2::operator+(const vec2& v) const
    {
        return { x + v.x, y + v.y };
    }

    vec2& vec2::operator+=(const vec2& v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    vec2 vec2::operator-(const vec2& v) const
    {
        return { x - v.x, y - v.y };
    }

    vec2& vec2::operator-=(const vec2& v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    vec2 vec2::operator*(const double scale) const
    {
        return { x * scale, y * scale };
    }

    vec2& vec2::operator*=(const double scale)
    {
        x *= scale;
        y *= scale;
        return *this;
    }

    vec2 vec2::operator/(const double divisor) const
    {
        return { x / divisor, y / divisor };
    }

    vec2& vec2::operator/=(const double divisor)
    {
        x /= divisor;
        y /= divisor;
        return *this;
    }

    vec2 vec2::operator-() const
    {
        return { -x, -y };
    }

    double vec2::Length() const
    {
        return std::sqrt(x * x + y * y);
    }

    vec2 vec2::Normalize() const
    {
        double len = Length();
        if (len > std::numeric_limits<double>::epsilon())
        {
            return { x / len, y / len };
        }
        return { 0.0, 0.0 };
    }

    double vec2::Dot(const vec2& v) const
    {
        return x * v.x + y * v.y;
    }

    vec2 operator*(double scale, const vec2& v)
    {
        return { scale * v.x, scale * v.y };
    }

    // ivec2
    bool ivec2::operator==(const ivec2& v) const
    {
        return x == v.x && y == v.y;
    }

    bool ivec2::operator!=(const ivec2& v) const
    {
        return x != v.x || y != v.y;
    }

    ivec2 ivec2::operator+(const ivec2& v) const
    {
        return { x + v.x, y + v.y };
    }

    ivec2& ivec2::operator+=(const ivec2& v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    ivec2 ivec2::operator-(const ivec2& v) const
    {
        return { x - v.x, y - v.y };
    }

    ivec2& ivec2::operator-=(const ivec2& v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    ivec2 ivec2::operator*(const int scale) const
    {
        return { x * scale, y * scale };
    }

    ivec2& ivec2::operator*=(const int scale)
    {
        x *= scale;
        y *= scale;
        return *this;
    }

    ivec2 ivec2::operator/(const int divisor) const
    {
        return { x / divisor, y / divisor };
    }

    ivec2& ivec2::operator/=(const int divisor)
    {
        x /= divisor;
        y /= divisor;
        return *this;
    }

    vec2 ivec2::operator*(const double scale) const
    {
        return { x * scale, y * scale };
    }

    vec2 ivec2::operator/(const double divisor) const
    {
        return { x / divisor, y / divisor };
    }

    ivec2 ivec2::operator-() const
    {
        return { -x, -y };
    }
}
