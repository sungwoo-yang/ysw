/**
 * \file
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include <cmath>
#include <limits>
#include <numbers>

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

        [[nodiscard]] bool operator==(const vec2& v) const;
        [[nodiscard]] bool operator!=(const vec2& v) const;

        [[nodiscard]] vec2 operator+(const vec2& v) const;
        vec2&              operator+=(const vec2& v);

        [[nodiscard]] vec2 operator-(const vec2& v) const;
        vec2&              operator-=(const vec2& v);

        [[nodiscard]] vec2 operator*(const double scale) const;
        vec2&              operator*=(const double scale);

        [[nodiscard]] vec2 operator/(const double divisor) const;
        vec2&              operator/=(const double divisor);

        [[nodiscard]] vec2 operator-() const;

        [[nodiscard]] double Length() const;
        [[nodiscard]] double LengthSquared() const;
        [[nodiscard]] vec2   Normalize() const;
        [[nodiscard]] double Dot(const vec2& v) const;
    };

    [[nodiscard]] vec2 operator*(double scale, const vec2& v);

    // Dot Product
    [[nodiscard]] inline double dot(const vec2& v1, const vec2& v2)
    {
        return v1.x * v2.x + v1.y * v2.y;
    }

    // Get Perpendicular Vector
    [[nodiscard]] inline vec2 GetPerpendicular(const vec2& v)
    {
        return { v.y, -v.x };
    }

    struct ivec2
    {
        int x{ 0 };
        int y{ 0 };

        constexpr ivec2() = default;
        constexpr ivec2(int x_, int y_) : x(x_), y(y_) { };

        explicit operator vec2() const
        {
            return vec2{ static_cast<double>(x), static_cast<double>(y) };
        }

        [[nodiscard]] bool operator==(const ivec2& v) const;
        [[nodiscard]] bool operator!=(const ivec2& v) const;

        [[nodiscard]] ivec2  operator+(const ivec2& v) const;
        ivec2& operator+=(const ivec2& v);

        [[nodiscard]] ivec2  operator-(const ivec2& v) const;
        ivec2& operator-=(const ivec2& v);

        [[nodiscard]] ivec2  operator*(const int scale) const;
        ivec2& operator*=(const int scale);

        [[nodiscard]] ivec2  operator/(const int divisor) const;
        ivec2& operator/=(const int divisor);

        [[nodiscard]] vec2 operator*(const double scale) const;
        [[nodiscard]] vec2 operator/(const double divisor) const;

        [[nodiscard]] ivec2 operator-() const;
    };
}