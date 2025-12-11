#include "Game/Bonfire.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/RGBA.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Game/Player.hpp"          
#include "Game/WorldTextManager.hpp"

Bonfire::Bonfire(Math::vec2 start_pos, Math::vec2 size) : CS230::GameObject(start_pos), bonfireSize(size)
{
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
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
    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::F))
    {
        Player* player = dynamic_cast<Player*>(interactor);
        if (player != nullptr)
        {
            player->SetSavePoint(GetPosition() + Math::vec2({ 0, 50 }));
            Engine::GetLogger().LogEvent("Event: Bonfire interacted, save point updated!");
        }
    }

    auto textManager = Engine::GetGameStateManager().GetGSComponent<WorldTextManager>();
    if (textManager)
    {
        textManager->ShowTextAbove(this, "Checkpoint Saved.", 0.5, CS200::WHITE);
    }
}