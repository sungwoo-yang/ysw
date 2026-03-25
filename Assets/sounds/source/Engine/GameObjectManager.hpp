/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include "GameObject.hpp"
#include "Matrix.hpp"
#include <list>

namespace Math
{
    class TransformationMatrix;
}

namespace CS230
{
    class GameObjectManager : public CS230::Component
    {
    public:
        void Add(GameObject* object);
        void Unload();

        void UpdateAll(double dt);
        void DrawAll(Math::TransformationMatrix camera_matrix);
        void CollisionTest();
        void DrawAllImGui();

        const std::list<GameObject*>& GetObjects() const
        {
            return objects;
        }

    private:
        std::list<GameObject*> objects;
    };
}