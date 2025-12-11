#include "MainMenu.hpp"
#include "Mode1.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Window.hpp"


void MainMenu::Load()
{
    Engine::GetWindow().Clear(0x111111FF);

    CS230::Font& titleFont = Engine::GetFont(0);
    CS230::Font& menuFont  = Engine::GetFont(1);

    titleTexture = titleFont.PrintToTexture("Ollim", CS200::WHITE);
    startTexture = menuFont.PrintToTexture("Start Game", CS200::WHITE);
    exitTexture  = menuFont.PrintToTexture("Exit", CS200::WHITE);

    SetupButtons();
}

void MainMenu::SetupButtons()
{
    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    double      centerX = static_cast<double>(winSize.x) * 0.5;
    double      centerY = static_cast<double>(winSize.y) * 0.5;

    if (startTexture)
    {
        Math::ivec2 size = startTexture->GetSize();
        Math::vec2  pos  = { centerX - size.x * 0.5, centerY - 50.0 };
        startButtonRect  = {
            pos, { pos.x + size.x, pos.y + size.y }
        };
    }

    if (exitTexture)
    {
        Math::ivec2 size = exitTexture->GetSize();
        Math::vec2  pos  = { centerX - size.x * 0.5, centerY - 150.0 };
        exitButtonRect   = {
            pos, { pos.x + size.x, pos.y + size.y }
        };
    }
}

void MainMenu::Update([[maybe_unused]] double dt)
{
    auto&       input    = Engine::GetInput();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();

    mousePos.y = static_cast<double>(winSize.y) - mousePos.y;

    auto CheckHover = [&](const Math::rect& rect) -> bool
    {
        return mousePos.x >= rect.Left() && mousePos.x <= rect.Right() && mousePos.y >= rect.Bottom() && mousePos.y <= rect.Top();
    };

    isStartHovered = CheckHover(startButtonRect);
    isExitHovered  = CheckHover(exitButtonRect);

    if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        if (isStartHovered)
        {
            Engine::GetLogger().LogEvent("Game Start Selected");
            Engine::GetGameStateManager().Clear();
            Engine::GetGameStateManager().PushState<Mode1>();
        }
        else if (isExitHovered)
        {
            Engine::GetLogger().LogEvent("Exit Selected");
            Engine::Instance().Stop();
        }
    }
}

void MainMenu::Draw()
{
    Engine::GetWindow().Clear(0x111111FF);
    auto&       renderer = Engine::GetRenderer2D();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();

    renderer.BeginScene(CS200::build_ndc_matrix(winSize));

    double centerX = static_cast<double>(winSize.x) * 0.5;
    double centerY = static_cast<double>(winSize.y) * 0.5;

    if (titleTexture)
    {
        Math::vec2  scale   = { 3.0, 3.0 };
        Math::ivec2 texSize = titleTexture->GetSize();
        Math::vec2  pos     = { centerX - (texSize.x * scale.x * 0.5), centerY + 150.0 };

        Math::TransformationMatrix transform = Math::TranslationMatrix(pos) * Math::ScaleMatrix(scale);
        titleTexture->Draw(transform, CS200::CYAN);
    }

    if (startTexture)
    {
        CS200::RGBA color = isStartHovered ? CS200::YELLOW : CS200::WHITE;
        Math::vec2  pos   = startButtonRect.point_1;

        Math::TransformationMatrix transform = Math::TranslationMatrix(pos);
        startTexture->Draw(transform, color);
    }

    if (exitTexture)
    {
        CS200::RGBA color = isExitHovered ? CS200::RED : CS200::WHITE;
        Math::vec2  pos   = exitButtonRect.point_1;

        Math::TransformationMatrix transform = Math::TranslationMatrix(pos);
        exitTexture->Draw(transform, color);
    }

    renderer.EndScene();
}

void MainMenu::DrawImGui()
{
}

void MainMenu::Unload()
{
    titleTexture.reset();
    startTexture.reset();
    exitTexture.reset();
}