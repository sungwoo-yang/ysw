#include "PushableMirror.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"

PushableMirror::PushableMirror(Math::vec2 start_pos, Math::vec2 size) : CS230::GameObject(start_pos), size(size)
{
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };
    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void PushableMirror::Update(double dt)
{
    velocityY -= gravity * dt;

    Math::vec2 vel = GetVelocity();
    if (std::abs(vel.x) > 0.1)
    {
        vel.x -= vel.x * friction * dt;
    }
    else
    {
        vel.x = 0.0;
    }

    SetVelocity({ vel.x, velocityY });

    CS230::GameObject::Update(dt);
}

void PushableMirror::Draw(const Math::TransformationMatrix& camera_matrix)
{
    auto&                      renderer  = Engine::GetRenderer2D();
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(size);

    renderer.DrawRectangle(transform, 0x00FFFF80, CS200::WHITE, 2.0);

    Math::vec2 half = size * 0.5;
    Math::vec2 p1   = { -half.x, half.y };
    Math::vec2 p2   = { half.x, -half.y };

    renderer.DrawLine(GetMatrix(), p1, p2, 0xDDDDDDFF, 5.0);

    CS230::GameObject::Draw(camera_matrix);
}

bool PushableMirror::CanCollideWith(GameObjectTypes other_object_type)
{
    return other_object_type == GameObjectTypes::Floor;
}

void PushableMirror::ResolveCollision(CS230::GameObject* other_object)
{
    if (other_object->Type() == GameObjectTypes::Floor)
    {
        CS230::RectCollision* my_collider    = GetGOComponent<CS230::RectCollision>();
        CS230::SATCollision*  other_collider = other_object->GetGOComponent<CS230::SATCollision>();

        if (!my_collider || !other_collider)
            return;

        Math::rect my_box    = my_collider->WorldBoundary();
        Math::rect other_box = other_collider->WorldBoundary().FindBoundary();

        if (my_box.Right() > other_box.Left() && my_box.Left() < other_box.Right() && my_box.Top() > other_box.Bottom() && my_box.Bottom() < other_box.Top())
        {
            if (velocityY <= 0 && GetPosition().y > other_box.Top())
            {
                SetPosition({ GetPosition().x, other_box.Top() + size.y / 2.0 });
                velocityY = 0.0;
            }
        }
    }
}

void PushableMirror::Push(Math::vec2 pushVelocity)
{
    Math::vec2 currentVel = GetVelocity();
    SetVelocity({ pushVelocity.x, currentVel.y });
}

std::vector<std::pair<Math::vec2, Math::vec2>> PushableMirror::GetSegments() const
{
    Math::vec2 pos  = GetPosition();
    Math::vec2 half = size * 0.5;

    Math::vec2 p1 = pos + Math::vec2{ -half.x, half.y };
    Math::vec2 p2 = pos + Math::vec2{ half.x, -half.y };

    return {
        { p1, p2 }
    };
}