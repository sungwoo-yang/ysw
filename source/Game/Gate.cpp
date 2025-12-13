#include "Gate.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"

Gate::Gate(Math::vec2 position, Math::vec2 size) : CS230::GameObject(position), size(size), isOpen(false), color(0xFF00FFFF)
{
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };

    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void Gate::Update(double dt)
{
    CS230::GameObject::Update(dt);
}

void Gate::Draw(const Math::TransformationMatrix& camera_matrix)
{
    if (!isOpen)
    {
        auto&                      renderer  = Engine::GetRenderer2D();
        Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(size);
        renderer.DrawRectangle(transform, color, CS200::WHITE, 2.0);
        CS230::GameObject::Draw(camera_matrix);
    }
}

bool Gate::CanCollideWith(GameObjectTypes other_object_type)
{
    if (isOpen)
        return false;
    if (other_object_type == GameObjectTypes::Player)
        return true;

    return false;
}

void Gate::Open()
{
    if (!isOpen)
    {
        isOpen = true;
        RemoveGOComponent<CS230::RectCollision>();
        Engine::GetLogger().LogEvent("Gate Opened!");
    }
}

void Gate::Close()
{
    if (isOpen)
    {
        isOpen = false;
        Math::irect collision_box{
            { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
            {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
        };
        AddGOComponent(new CS230::RectCollision(collision_box, this));

        Engine::GetLogger().LogEvent("Gate Closed!");
    }
}