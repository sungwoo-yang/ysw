/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once
#include "Component.hpp"
#include <algorithm>
#include <vector>

namespace CS230
{
    class ComponentManager
    {
    public:
        ~ComponentManager()
        {
            Clear();
        }

        void UpdateAll(double dt)
        {
            for (Component* component : components)
            {
                component->Update(dt);
            }
        }

        void AddComponent(Component* component)
        {
            components.push_back(component);
        }

        template <typename T>
        T* GetComponent()
        {
            for (Component* component : components)
            {
                T* ptr = dynamic_cast<T*>(component);
                if (ptr != nullptr)
                {
                    return ptr;
                }
            }
            return nullptr;
        }

        template <typename T>
        void RemoveComponent()
        {
            auto it = std::find_if(components.begin(), components.end(), [](Component* element) { return (dynamic_cast<T*>(element) != nullptr); });
            delete *it;
            components.erase(it);
        }

        void Clear()
        {
            for (Component* component : components)
            {
                delete component;
            }
            components.clear();
        }

    private:
        std::vector<Component*> components;
    };
}
