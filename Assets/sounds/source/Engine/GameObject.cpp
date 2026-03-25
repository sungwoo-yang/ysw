/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "GameObject.hpp"
#include "Collision.hpp"
#include "Engine.hpp"
#include "GameStateManager.hpp"
#include "ShowCollision.hpp"
#include "Sprite.hpp"

namespace CS230
{

    GameObject::GameObject(Math::vec2 pos) : GameObject(pos, 0, { 1, 1 })
    {
    }

    GameObject::GameObject(Math::vec2 pos, double rot, Math::vec2 sc) : current_state(&state_none), matrix_outdated(true), rotation(rot), scale(sc), position(pos), velocity({ 0, 0 }), destroy(false)
    {
    }

    void GameObject::Update(double dt)
    {
        current_state->Update(this, dt);
        if (velocity.x != 0 || velocity.y != 0)
        {
            UpdatePosition(velocity * dt);
        }
        UpdateGOComponents(dt);
        current_state->CheckExit(this);
    }

    void GameObject::Draw(const Math::TransformationMatrix& camera_matrix)
    {
        Sprite* sprite = GetGOComponent<Sprite>();
        if (sprite != nullptr)
        {
            sprite->Draw(camera_matrix * GetMatrix());
        }

        auto show_collision = Engine::GetGameStateManager().GetGSComponent<CS230::ShowCollision>();
        if (show_collision != nullptr && show_collision->Enabled())
        {
            Collision* collision = GetGOComponent<Collision>();
            if (collision != nullptr)
            {
                collision->Draw(camera_matrix);
            }
        }
    }

    bool GameObject::IsCollidingWith(GameObject* other_object)
    {
        Collision* collider = GetGOComponent<Collision>();
        return collider != nullptr && collider->IsCollidingWith(other_object);
    }

    bool GameObject::IsCollidingWith(Math::vec2 point)
    {
        Collision* collider = GetGOComponent<Collision>();
        return collider != nullptr && collider->IsCollidingWith(point);
    }

    bool GameObject::CanCollideWith([[maybe_unused]] GameObjectTypes other_object_type)
    {
        return false;
    }

    const Math::TransformationMatrix& GameObject::GetMatrix()
    {
        if (matrix_outdated)
        {
            object_matrix   = Math::TranslationMatrix(position) * Math::RotationMatrix(rotation) * Math::ScaleMatrix(scale);
            matrix_outdated = false;
        }
        return object_matrix;
    }

    const Math::vec2& GameObject::GetPosition() const
    {
        return position;
    }

    const Math::vec2& GameObject::GetVelocity() const
    {
        return velocity;
    }

    const Math::vec2& GameObject::GetScale() const
    {
        return scale;
    }

    double GameObject::GetRotation() const
    {
        return rotation;
    }

    void GameObject::SetPosition(Math::vec2 new_position)
    {
        position        = new_position;
        matrix_outdated = true;
    }

    void GameObject::UpdatePosition(Math::vec2 delta)
    {
        position += delta;
        matrix_outdated = true;
    }

    void GameObject::SetVelocity(Math::vec2 new_velocity)
    {
        velocity = new_velocity;
    }

    void GameObject::UpdateVelocity(Math::vec2 delta)
    {
        velocity += delta;
    }

    void GameObject::SetScale(Math::vec2 new_scale)
    {
        scale           = new_scale;
        matrix_outdated = true;
    }

    void GameObject::UpdateScale(Math::vec2 delta)
    {
        scale += delta;
        matrix_outdated = true;
    }

    void GameObject::SetRotation(double new_rotation)
    {
        rotation        = new_rotation;
        matrix_outdated = true;
    }

    void GameObject::UpdateRotation(double delta)
    {
        rotation += delta;
        matrix_outdated = true;
    }

    void GameObject::change_state(State* new_state)
    {
        current_state = new_state;
        current_state->Enter(this);
    }

    void GameObject::Destroy()
    {
        destroy = true;
    }

    bool GameObject::Destroyed() const
    {
        return destroy;
    }

} // namespace CS230