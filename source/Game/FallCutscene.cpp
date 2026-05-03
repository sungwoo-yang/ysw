#include "FallCutscene.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Window.hpp"

std::function<void()> FallCutscene::pendingNextStateAction = nullptr;

void FallCutscene::SetNextState(std::function<void()> nextStateAction)
{
    pendingNextStateAction = std::move(nextStateAction);
}

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
        if (pendingNextStateAction)
        {
            auto action            = std::move(pendingNextStateAction);
            pendingNextStateAction = nullptr;
            action();
        }
    }
}

void FallCutscene::Draw()
{
    auto&       renderer     = Engine::GetRenderer2D();
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
