/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include "Engine/Component.hpp"

namespace CS230
{
    class ShowCollision : public CS230::Component
    {
    public:
        ShowCollision();
        void Update(double dt) override;
        bool Enabled();

    private:
        bool enabled;
    };
}