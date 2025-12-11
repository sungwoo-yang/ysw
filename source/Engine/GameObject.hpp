/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once
#include "ComponentManager.hpp"
#include "Matrix.hpp"
#include "Sprite.hpp"
#include "Vec2.hpp"

enum class GameObjectTypes;

namespace CS230
{
    class Component;

    class GameObject
    {
    public:
        friend class Sprite;
        GameObject(Math::vec2 pos);
        GameObject(Math::vec2 pos, double rot, Math::vec2 sc);

        virtual ~GameObject()
        {
        }

        virtual GameObjectTypes Type()     = 0;
        virtual std::string     TypeName() = 0;
        bool                    IsCollidingWith(GameObject* other_object);
        bool                    IsCollidingWith(Math::vec2 point);
        virtual bool            CanCollideWith(GameObjectTypes other_object_type);

        virtual void Update(double dt);
        virtual void Draw(const Math::TransformationMatrix& camera_matrix);
        virtual void ResolveCollision([[maybe_unused]] GameObject* other_object) { };
        virtual void Interact([[maybe_unused]] GameObject* other_object) { };
        virtual void DrawImGui() { };

        const Math::TransformationMatrix& GetMatrix();
        const Math::vec2&                 GetPosition() const;
        const Math::vec2&                 GetVelocity() const;
        const Math::vec2&                 GetScale() const;
        double                            GetRotation() const;

        void SetPosition(Math::vec2 new_position);

        template <typename T>
        T* GetGOComponent()
        {
            return componentmanager.GetComponent<T>();
        }

        void Destroy();
        bool Destroyed() const;

    protected:
        void UpdatePosition(Math::vec2 delta);
        void SetVelocity(Math::vec2 new_velocity);
        void UpdateVelocity(Math::vec2 delta);
        void SetScale(Math::vec2 new_scale);
        void UpdateScale(Math::vec2 delta);
        void SetRotation(double new_rotation);
        void UpdateRotation(double delta);

        class State
        {
        public:
            virtual ~State()                                          = default;
            virtual void        Enter(GameObject* object)             = 0;
            virtual void        Update(GameObject* object, double dt) = 0;
            virtual void        CheckExit(GameObject* object)         = 0;
            virtual std::string GetName()                             = 0;
        };

        State* current_state;
        void   change_state(State* new_state);

        void AddGOComponent(Component* component)
        {
            componentmanager.AddComponent(component);
        }

        template <typename T>
        void RemoveGOComponent()
        {
            componentmanager.RemoveComponent<T>();
        }

        void ClearGOComponents()
        {
            componentmanager.Clear();
        }

        void UpdateGOComponents(double dt)
        {
            componentmanager.UpdateAll(dt);
        }

    private:
        Math::TransformationMatrix object_matrix;
        bool                       matrix_outdated = true;

        double     rotation;
        Math::vec2 scale;
        Math::vec2 position;
        Math::vec2 velocity;

        class State_None : public State
        {
        public:
            void Enter(GameObject*) { };
            void Update(GameObject*, double) { };
            void CheckExit(GameObject*) { };

            std::string GetName()
            {
                return "";
            }
        };

        State_None       state_none;
        ComponentManager componentmanager;

        bool destroy;
    };
}