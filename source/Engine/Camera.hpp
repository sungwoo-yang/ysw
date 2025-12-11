/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include "Component.hpp"
#include "Matrix.hpp"
#include "Rect.hpp"
#include "Vec2.hpp"

namespace CS230
{
    class Camera : public Component
    {
    public:
        using CS230::Component::Update;
        
        Camera(Math::rect pz);
        void                       SetPosition(Math::vec2 new_position);
        const Math::vec2&          GetPosition() const;
        void                       SetLimit(Math::irect new_limit);
        void                       Update(const Math::vec2& player_position);
        Math::TransformationMatrix GetMatrix();

    private:
        Math::irect limit;
        Math::rect  player_zone;
        Math::vec2  position;
    };
}