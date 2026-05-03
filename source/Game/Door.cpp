#include "Door.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"

Door::Door(Math::vec2 in_position, Math::vec2 in_size) : CS230::GameObject(in_position), size(in_size)
{
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };

    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void Door::Draw(const Math::TransformationMatrix& camera_matrix)
{
    CS200::IRenderer2D& renderer = Engine::GetRenderer2D();

    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(size);

    renderer.DrawRectangle(transform, 0x8B4513FF, CS200::WHITE, 2.0);

    CS230::GameObject::Draw(camera_matrix);
}

void Door::Interact([[maybe_unused]] CS230::GameObject* interactor)
{
    interactionRequested = true;

    Engine::GetLogger().LogEvent("Door interaction requested: " + GetName());
}

bool Door::ConsumeInteractionRequest()
{
    if (!interactionRequested)
    {
        return false;
    }

    interactionRequested = false;
    return true;
}