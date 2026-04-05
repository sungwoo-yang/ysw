#include "FallCutscene.hpp"
#include "Mode3.hpp" 
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Window.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"

void FallCutscene::Load()
{
    timer = 0.0;
    
    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    
    playerPos = { static_cast<double>(winSize.x) / 2.0, static_cast<double>(winSize.y) + 100.0 };
}

void FallCutscene::Update(double dt)
{
    timer += dt;

    playerPos.y -= 600.0 * dt;

    if (timer >= fallDuration)
    {
        Engine::GetGameStateManager().ChangeStateWithFade<Mode3>();
    }
}

void FallCutscene::Draw()
{
    auto& renderer = Engine::GetRenderer2D();
    Math::ivec2 display_size = Engine::GetWindow().GetSize();

    Engine::GetWindow().Clear(0x111111FF);

    Math::TransformationMatrix screen_matrix = CS200::build_ndc_matrix(display_size);
    renderer.BeginScene(screen_matrix);

    Math::TransformationMatrix transform = Math::TranslationMatrix(playerPos) * Math::ScaleMatrix(playerSize);
    renderer.DrawRectangle(transform, CS200::GREEN, CS200::CLEAR, 0.0);

    renderer.EndScene();
}

void FallCutscene::DrawImGui()
{
}

void FallCutscene::Unload()
{
}