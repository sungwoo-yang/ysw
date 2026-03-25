/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "ComponentManager.hpp"
#include <gsl/gsl>

namespace CS230
{
    class GameState
    {
    public:
        virtual void          Load()            = 0;
        virtual void          Update(double dt) = 0;
        virtual void          Unload()          = 0;
        virtual void          Draw()            = 0;
        virtual void          DrawImGui()       = 0;
        virtual gsl::czstring GetName() const   = 0;
        virtual ~GameState()                    = default;

        template <typename T>
        T* GetGSComponent()
        {
            return componentmanager.GetComponent<T>();
        }

    protected:
        void AddGSComponent(Component* component)
        {
            componentmanager.AddComponent(component);
        }

        void UpdateGSComponents(double dt)
        {
            componentmanager.UpdateAll(dt);
        }

        template <typename T>
        void RemoveGSComponent()
        {
            componentmanager.RemoveComponent<T>();
        }

        void ClearGSComponents()
        {
            componentmanager.Clear();
        }

    private:
        ComponentManager componentmanager;
    };
}
