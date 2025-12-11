#include "Splash.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Texture.hpp"
#include "Engine/Window.hpp"
#include "MainMenu.hpp"

void Splash::Load()
{
    Engine::GetWindow().Clear(CS200::WHITE);
    timer = displayTime;
}

void Splash::Update(double dt)
{
    timer -= dt;

    auto& input = Engine::GetInput();
    if (timer <= 0.0 || input.KeyJustPressed(CS230::Input::Keys::Space) || input.KeyJustPressed(CS230::Input::Keys::Enter))
    {
        Engine::GetGameStateManager().PushState<MainMenu>();
    }
}

void Splash::Draw()
{
    Engine::GetWindow().Clear(CS200::WHITE);
    auto& renderer = Engine::GetRenderer2D();

    Math::ivec2                winSize       = Engine::GetWindow().GetSize();
    Math::TransformationMatrix screen_matrix = CS200::build_ndc_matrix(winSize);

    renderer.BeginScene(screen_matrix);

    CS230::Font& font = Engine::GetFont(0);

    auto texture = font.PrintToTexture("DIGIPEN GAME ENGINE", 0xFF0000FF);
    if (texture)
    {
        Math::vec2  scale   = { 2.0, 2.0 };
        Math::ivec2 texSize = texture->GetSize();

        Math::vec2 centerPos = { winSize.x * 0.5, winSize.y * 0.55 };
        Math::vec2 drawPos   = centerPos - Math::vec2{ texSize.x * scale.x * 0.5, texSize.y * scale.y * 0.5 };

        Math::TransformationMatrix transform = Math::TranslationMatrix(drawPos) * Math::ScaleMatrix(scale);
        texture->Draw(transform);
    }

    auto subTex = font.PrintToTexture("Press Space to Start", 0x000000FF);
    if (subTex)
    {
        Math::vec2  scale   = { 1.0, 1.0 };
        Math::ivec2 texSize = subTex->GetSize();

        Math::vec2 centerPos = { winSize.x * 0.5, winSize.y * 0.45 };
        Math::vec2 drawPos   = centerPos - Math::vec2{ texSize.x * scale.x * 0.5, texSize.y * scale.y * 0.5 };

        Math::TransformationMatrix transform = Math::TranslationMatrix(drawPos);
        subTex->Draw(transform);
    }

    renderer.EndScene();
}

void Splash::DrawImGui()
{
}

void Splash::Unload()
{
}