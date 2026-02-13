#include "Door.hpp"
#include "Mode2.hpp" 
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Game/WorldTextManager.hpp"

Door::Door(Math::vec2 in_position, Math::vec2 in_size) : CS230::GameObject(in_position), size(in_size)
{
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        { static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };
    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void Door::Draw(const Math::TransformationMatrix& camera_matrix)
{
    CS200::IRenderer2D& renderer = Engine::GetRenderer2D();
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(size);

    // Draw Door object
    renderer.DrawRectangle(transform, 0x8B4513FF, CS200::WHITE, 2.0);
    CS230::GameObject::Draw(camera_matrix);
}

void Door::Interact([[maybe_unused]]CS230::GameObject* interactor)
{
    // Show interaction UI
    auto textManager = Engine::GetGameStateManager().GetGSComponent<WorldTextManager>();
    if (textManager)
    {
        textManager->ShowTextAbove(this, "Press 'F' to Enter", 0.6, CS200::WHITE);
    }

    // Transition to Mode2 on key press
    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::F))
    {
        Engine::GetLogger().LogEvent("Entering Door -> Mode2");
        
        // Clear and transition to next level
        Engine::GetGameStateManager().Clear();
        Engine::GetGameStateManager().PushState<Mode2>();
    }
}