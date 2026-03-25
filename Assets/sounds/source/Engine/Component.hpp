/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once

namespace CS230
{
    class Component
    {
    public:
        virtual ~Component() { };
        virtual void Update([[maybe_unused]] double dt) { };
    };
}
