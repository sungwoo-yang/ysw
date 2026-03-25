#include "Game/Sign.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/RGBA.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Game/WorldTextManager.hpp"

Sign::Sign(Math::vec2 start_pos, Math::vec2 size, std::string msg) : CS230::GameObject(start_pos), signSize(size), message(std::move(msg))
{
    // Create a bounding box for interaction proximity detection
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };
    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void Sign::Draw(const Math::TransformationMatrix& camera_matrix)
{
    CS200::IRenderer2D&        renderer  = Engine::GetRenderer2D();
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(signSize);

    // Draw the sign's physical body as a yellow rectangle
    renderer.DrawRectangle(transform, 0xFFFF00FF, CS200::CLEAR, 0.0);

    CS230::GameObject::Draw(camera_matrix);
}

void Sign::Interact([[maybe_unused]] CS230::GameObject* interactor)
{
    // Retrieve the UI text manager to display the sign's content in world space
    auto textManager = Engine::GetGameStateManager().GetGSComponent<WorldTextManager>();
    if (textManager)
    {
        // Display the message floating above the sign
        textManager->ShowTextAbove(this, message, 0.75, CS200::WHITE);
    }

    // Log the event when the player explicitly reads the sign
    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::F))
    {
        Engine::GetLogger().LogEvent("Event: Sign interacted, showing: " + message);
    }
}