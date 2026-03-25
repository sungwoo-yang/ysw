#include "Game/Bonfire.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/RGBA.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Game/Player.hpp"
#include "Game/WorldTextManager.hpp"

Bonfire::Bonfire(Math::vec2 in_start_pos, Math::vec2 in_size) : CS230::GameObject(in_start_pos), bonfireSize(in_size)
{
// Define collision area centered on the object position
    Math::irect collision_box{
        { static_cast<int>(-in_size.x / 2.0), static_cast<int>(-in_size.y / 2.0) },
        {  static_cast<int>(in_size.x / 2.0),  static_cast<int>(in_size.y / 2.0) }
    };
    
    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void Bonfire::Draw(const Math::TransformationMatrix& camera_matrix)
{
    CS200::IRenderer2D&        renderer  = Engine::GetRenderer2D();
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(bonfireSize);

    renderer.DrawRectangle(transform, 0xFF0000FF, CS200::CLEAR, 0.0);

    CS230::GameObject::Draw(camera_matrix);
}

void Bonfire::Interact(CS230::GameObject* interactor)
{
    // Update player save point when 'F' key is pressed
    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::F))
    {
        Player* player = dynamic_cast<Player*>(interactor);
        if (player != nullptr)
        {
            // Offset save point slightly above to prevent spawning inside floor
            player->SetSavePoint(GetPosition() + Math::vec2({ 0, 50 }));
            Engine::GetLogger().LogEvent("Event: Bonfire interacted, save point updated!");
        }
    }

    // Always show feedback text when player is in range
    auto textManager = Engine::GetGameStateManager().GetGSComponent<WorldTextManager>();
    if (textManager)
    {
        textManager->ShowTextAbove(this, "Checkpoint Saved.", 0.5, CS200::WHITE);
    }
}