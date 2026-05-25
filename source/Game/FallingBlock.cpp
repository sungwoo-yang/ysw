#include "FallingBlock.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"

FallingBlock::FallingBlock(Math::vec2 in_position, Math::vec2 in_size) : CS230::GameObject(in_position), size(in_size)
{
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };

    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void FallingBlock::Update(double dt)
{
    if (released)
    {
        velocityY -= gravity * dt;
        SetVelocity({ GetVelocity().x, velocityY });
    }
    else
    {
        SetVelocity({ 0.0, 0.0 });
        velocityY = 0.0;
    }

    CS230::GameObject::Update(dt);
}

void FallingBlock::Draw(const Math::TransformationMatrix& camera_matrix)
{
    auto& renderer = Engine::GetRenderer2D();

    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(size);

    renderer.DrawRectangle(transform, 0xCC00CCFF, CS200::WHITE, 2.0);

    CS230::GameObject::Draw(camera_matrix);
}

bool FallingBlock::CanCollideWith(GameObjectTypes other_object_type)
{
    return other_object_type == GameObjectTypes::Floor || other_object_type == GameObjectTypes::Gate || other_object_type == GameObjectTypes::Player;
}

void FallingBlock::ResolveCollision(CS230::GameObject* other_object)
{
    if (other_object->Type() == GameObjectTypes::Floor)
    {
        CS230::RectCollision* my_collider    = GetGOComponent<CS230::RectCollision>();
        CS230::SATCollision*  floor_collider = other_object->GetGOComponent<CS230::SATCollision>();

        if (!my_collider || !floor_collider)
        {
            return;
        }

        Math::rect my_box    = my_collider->WorldBoundary();
        Math::rect floor_box = floor_collider->WorldBoundary().FindBoundary();

        const bool horizontal_overlap = my_box.Right() > floor_box.Left() && my_box.Left() < floor_box.Right();

        const bool vertical_overlap = my_box.Top() > floor_box.Bottom() && my_box.Bottom() < floor_box.Top();

        if (!horizontal_overlap || !vertical_overlap)
        {
            return;
        }

        if (velocityY <= 0.0 && my_box.Bottom() >= floor_box.Bottom())
        {
            SetPosition({ GetPosition().x, floor_box.Top() + size.y / 2.0 });
            velocityY = 0.0;
            SetVelocity({ 0.0, 0.0 });
        }
    }
}

void FallingBlock::Release()
{
    if (released)
    {
        return;
    }

    released = true;
    Engine::GetLogger().LogEvent("FallingBlock released!");
}

bool FallingBlock::IsReleased() const
{
    return released;
}