/**
 * \file
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include <limits>
#include <numbers>
#include <cmath>

constexpr double PI = std::numbers::pi;

namespace Math
{
    struct vec2
    {
        double x{ 0.0 };
        double y{ 0.0 };

        constexpr vec2() = default;

        constexpr vec2(double x_, double y_) : x(x_), y(y_)
        {
        }

        bool operator==(const vec2& v) const;
        bool operator!=(const vec2& v) const;

        vec2  operator+(const vec2& v) const;
        vec2& operator+=(const vec2& v);

        vec2  operator-(const vec2& v) const;
        vec2& operator-=(const vec2& v);

        vec2  operator*(const double scale) const;
        vec2& operator*=(const double scale);

        vec2  operator/(const double divisor) const;
        vec2& operator/=(const double divisor);

        vec2 operator-() const;

        double Length() const;
        vec2   Normalize() const;

        double Dot(const vec2& v) const;
    };

    vec2 operator*(double scale, const vec2& v);

    inline double dot(const vec2& v1, const vec2& v2)
    {
        return v1.x * v2.x + v1.y * v2.y;
    }

    inline vec2 GetPerpendicular(const vec2& v)
    {
        return { v.y, -v.x };
    }

    struct ivec2
    {
        int x{ 0 };
        int y{ 0 };

        constexpr ivec2() = default;
        constexpr ivec2(int x_, int y_) : x(x_), y(y_) { };

        explicit operator vec2()
        {
            return vec2{ static_cast<double>(x), static_cast<double>(y) };
        }

        bool operator==(const ivec2& v) const;
        bool operator!=(const ivec2& v) const;

        ivec2  operator+(const ivec2& v) const;
        ivec2& operator+=(const ivec2& v);

        ivec2  operator-(const ivec2& v) const;
        ivec2& operator-=(const ivec2& v);

        ivec2  operator*(const int scale) const;
        ivec2& operator*=(const int scale);

        ivec2  operator/(const int divisor) const;
        ivec2& operator/=(const int divisor);

        vec2 operator*(const double scale) const;
        vec2 operator/(const double divisor) const;

        ivec2 operator-() const;
    };
}