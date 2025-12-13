#include "MainMenu.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Window.hpp"
#include "Mode1.hpp"

void MainMenu::Load()
{
    Engine::GetWindow().Clear(0x111111FF);

    CS230::Font& titleFont = Engine::GetFont(0);
    CS230::Font& menuFont  = Engine::GetFont(1);

    titleTexture    = titleFont.PrintToTexture("Ollim", CS200::WHITE);
    startTexture    = menuFont.PrintToTexture("Start Game", CS200::WHITE);
    settingsTexture = menuFont.PrintToTexture("Settings", CS200::WHITE);
    exitTexture     = menuFont.PrintToTexture("Exit", CS200::WHITE);

    res1600Texture = menuFont.PrintToTexture("1600x900", CS200::WHITE);
    res1280Texture = menuFont.PrintToTexture("1280x720", CS200::WHITE);

    windowedTexture   = menuFont.PrintToTexture("Windowed", CS200::WHITE);
    borderlessTexture = menuFont.PrintToTexture("Borderless", CS200::WHITE); // [추가]
    fullscreenTexture = menuFont.PrintToTexture("Fullscreen", CS200::WHITE);

    defaultTexture = menuFont.PrintToTexture("Default", CS200::WHITE);
    applyTexture   = menuFont.PrintToTexture("Apply", CS200::WHITE);
    backTexture    = menuFont.PrintToTexture("Back", CS200::WHITE);

    labelResolution = menuFont.PrintToTexture("Resolution:", CS200::WHITE);
    labelMode       = menuFont.PrintToTexture("Mode:", CS200::WHITE);

    SetupButtons();
}

void MainMenu::SetupButtons()
{
    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    double      centerX = static_cast<double>(winSize.x) * 0.5;
    double      centerY = static_cast<double>(winSize.y) * 0.5;

    // [메인 메뉴 버튼]
    if (startTexture)
    {
        Math::ivec2 size = startTexture->GetSize();
        Math::vec2  pos  = { centerX - size.x * 0.5, centerY + 20.0 };
        startButtonRect  = {
            pos, { pos.x + size.x, pos.y + size.y }
        };
    }
    if (settingsTexture)
    {
        Math::ivec2 size   = settingsTexture->GetSize();
        Math::vec2  pos    = { centerX - size.x * 0.5, centerY - 80.0 };
        settingsButtonRect = {
            pos, { pos.x + size.x, pos.y + size.y }
        };
    }
    if (exitTexture)
    {
        Math::ivec2 size = exitTexture->GetSize();
        Math::vec2  pos  = { centerX - size.x * 0.5, centerY - 180.0 };
        exitButtonRect   = {
            pos, { pos.x + size.x, pos.y + size.y }
        };
    }

    // [설정 메뉴 버튼]
    double leftColX  = centerX - 200.0;
    double rightColX = centerX + 100.0;
    double startY    = centerY + 100.0;
    double spacing   = 80.0;

    // 해상도
    if (res1600Texture)
        res1600Rect = {
            {                               leftColX,                               startY },
            { leftColX + res1600Texture->GetSize().x, startY + res1600Texture->GetSize().y }
        };
    if (res1280Texture)
        res1280Rect = {
            {                               leftColX,                               startY - spacing },
            { leftColX + res1280Texture->GetSize().x, startY - spacing + res1280Texture->GetSize().y }
        };

    // 화면 모드 (3개: Windowed, Borderless, Fullscreen)
    if (windowedTexture)
        windowedRect = {
            {                                rightColX,                                startY },
            { rightColX + windowedTexture->GetSize().x, startY + windowedTexture->GetSize().y }
        };
    if (borderlessTexture)
        borderlessRect = {
            {                                  rightColX,                                  startY - spacing },
            { rightColX + borderlessTexture->GetSize().x, startY - spacing + borderlessTexture->GetSize().y }
        };
    if (fullscreenTexture)
        fullscreenRect = {
            {                                  rightColX,                                  startY - spacing * 2.0 },
            { rightColX + fullscreenTexture->GetSize().x, startY - spacing * 2.0 + fullscreenTexture->GetSize().y }
        };

    // 하단 버튼
    if (defaultTexture)
        defaultRect = {
            {                                                    50.0,                                                    50.0 },
            { 50.0 + static_cast<double>(defaultTexture->GetSize().x), 50.0 + static_cast<double>(defaultTexture->GetSize().y) }
        };

    double bottomY     = 50.0;
    double rightMargin = static_cast<double>(winSize.x) - 50.0;

    if (backTexture)
    {
        Math::vec2 size = static_cast<Math::vec2>(backTexture->GetSize());
        backRect        = {
            { rightMargin - size.x,          bottomY },
            {          rightMargin, bottomY + size.y }
        };
    }
    if (applyTexture)
    {
        Math::vec2 size   = static_cast<Math::vec2>(applyTexture->GetSize());
        double     applyX = backRect.Left() - size.x - 50.0;
        applyRect         = {
            {          applyX,          bottomY },
            { applyX + size.x, bottomY + size.y }
        };
    }
}

void MainMenu::Update([[maybe_unused]] double dt)
{
    if (currentState == MenuState::Main)
        UpdateMainMenu(dt);
    else
        UpdateSettingsMenu(dt);
}

void MainMenu::UpdateMainMenu([[maybe_unused]] double dt)
{
    auto&       input    = Engine::GetInput();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();
    mousePos.y           = static_cast<double>(winSize.y) - mousePos.y;

    auto CheckHover = [&](const Math::rect& rect) -> bool
    {
        return mousePos.x >= rect.Left() && mousePos.x <= rect.Right() && mousePos.y >= rect.Bottom() && mousePos.y <= rect.Top();
    };

    isStartHovered    = CheckHover(startButtonRect);
    isSettingsHovered = CheckHover(settingsButtonRect);
    isExitHovered     = CheckHover(exitButtonRect);

    if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        if (isStartHovered)
        {
            Engine::GetGameStateManager().Clear();
            Engine::GetGameStateManager().PushState<Mode1>();
        }
        else if (isSettingsHovered)
        {
            currentState = MenuState::Settings;

            // 현재 상태 읽어오기 (초기값 활성화)
            Math::ivec2 currentSize = Engine::GetWindow().GetSize();
            if (currentSize.x == 1600)
                pendingResIndex = 0;
            else
                pendingResIndex = 1; // 1280

            // 실제로는 SDL에서 현재 윈도우 플래그를 읽어와야 정확하지만,
            // 여기서는 기본값을 Windowed로 시작하도록 설정
            pendingMode = WindowMode::Windowed;
        }
        else if (isExitHovered)
        {
            Engine::Instance().Stop();
        }
    }
}

void MainMenu::UpdateSettingsMenu([[maybe_unused]] double dt)
{
    auto&       input    = Engine::GetInput();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();
    mousePos.y           = static_cast<double>(winSize.y) - mousePos.y;

    auto CheckClick = [&](const Math::rect& rect) -> bool
    {
        return input.MouseButtonJustPressed(CS230::Input::MouseButton::Left) && mousePos.x >= rect.Left() && mousePos.x <= rect.Right() && mousePos.y >= rect.Bottom() && mousePos.y <= rect.Top();
    };

    // 해상도 선택
    if (CheckClick(res1600Rect))
        pendingResIndex = 0;
    if (CheckClick(res1280Rect))
        pendingResIndex = 1;

    // 모드 선택
    if (CheckClick(windowedRect))
        pendingMode = WindowMode::Windowed;
    if (CheckClick(borderlessRect))
        pendingMode = WindowMode::Borderless;
    if (CheckClick(fullscreenRect))
        pendingMode = WindowMode::Fullscreen;

    // [Default]
    if (CheckClick(defaultRect))
    {
        pendingResIndex = 1; // 1280x720
        pendingMode     = WindowMode::Windowed;
    }

    // [Back] - 메인 메뉴로 복귀
    if (CheckClick(backRect))
    {
        currentState = MenuState::Main;
    }

    // [Apply] - 설정 적용 (창 중앙 정렬 문제 해결)
    if (CheckClick(applyRect))
    {
        auto&       window  = Engine::GetWindow();
        Math::ivec2 newSize = resolutions[pendingResIndex];

        // 1. 전체화면 해제 (순서 중요: 전체화면 상태에서는 위치/크기 변경이 무시될 수 있음)
        if (pendingMode != WindowMode::Fullscreen)
        {
            window.SetFullscreen(false);
        }

        // 2. 테두리 설정 (Borderless or Windowed)
        bool border = (pendingMode == WindowMode::Windowed);
        window.SetBordered(border);

        // 3. 해상도 변경 및 중앙 정렬
        // (SetFullscreen(false) 직후에 호출해야 위치가 올바르게 중앙으로 감)
        window.ForceResize(newSize.x, newSize.y);

        // 4. 전체화면 적용 (필요한 경우)
        if (pendingMode == WindowMode::Fullscreen)
        {
            window.SetFullscreen(true);
        }

        SetupButtons(); // UI 재배치
    }
}

void MainMenu::Draw()
{
    Engine::GetWindow().Clear(0x111111FF);
    auto&       renderer = Engine::GetRenderer2D();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();

    renderer.BeginScene(CS200::build_ndc_matrix(winSize));

    if (currentState == MenuState::Main)
        DrawMainMenu();
    else
        DrawSettingsMenu();

    renderer.EndScene();
}

void MainMenu::DrawMainMenu()
{
    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    double      centerX = static_cast<double>(winSize.x) * 0.5;
    double      centerY = static_cast<double>(winSize.y) * 0.5;

    if (titleTexture)
    {
        Math::vec2                 scale     = { 3.0, 3.0 };
        Math::ivec2                texSize   = titleTexture->GetSize();
        Math::vec2                 pos       = { centerX - (texSize.x * scale.x * 0.5), centerY + 180.0 };
        Math::TransformationMatrix transform = Math::TranslationMatrix(pos) * Math::ScaleMatrix(scale);
        titleTexture->Draw(transform, CS200::CYAN);
    }

    auto DrawBtn = [&](std::shared_ptr<CS230::Texture> tex, Math::rect rect, bool hover)
    {
        if (tex)
        {
            CS200::RGBA color = hover ? CS200::YELLOW : CS200::WHITE;
            tex->Draw(Math::TranslationMatrix(rect.point_1), color);
        }
    };

    DrawBtn(startTexture, startButtonRect, isStartHovered);
    DrawBtn(settingsTexture, settingsButtonRect, isSettingsHovered);
    DrawBtn(exitTexture, exitButtonRect, isExitHovered);
}

void MainMenu::DrawSettingsMenu()
{
    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    double      centerX = static_cast<double>(winSize.x) * 0.5;
    double      centerY = static_cast<double>(winSize.y) * 0.5;

    if (settingsTexture)
    {
        Math::vec2 scale = { 2.0, 2.0 };
        Math::vec2 pos   = { centerX - (settingsTexture->GetSize().x * scale.x * 0.5), centerY + 250.0 };
        settingsTexture->Draw(Math::TranslationMatrix(pos) * Math::ScaleMatrix(scale), CS200::CYAN);
    }

    auto DrawOption = [&](std::shared_ptr<CS230::Texture> tex, Math::rect rect, bool selected)
    {
        if (tex)
        {
            CS200::RGBA color = selected ? CS200::GREEN : CS200::GREY;
            tex->Draw(Math::TranslationMatrix(rect.point_1), color);
        }
    };

    if (labelResolution)
        labelResolution->Draw(Math::TranslationMatrix(Math::vec2{ res1600Rect.Left(), res1600Rect.Top() + 30.0 }), CS200::WHITE);

    DrawOption(res1600Texture, res1600Rect, pendingResIndex == 0);
    DrawOption(res1280Texture, res1280Rect, pendingResIndex == 1);

    if (labelMode)
        labelMode->Draw(Math::TranslationMatrix(Math::vec2{ windowedRect.Left(), windowedRect.Top() + 30.0 }), CS200::WHITE);

    DrawOption(windowedTexture, windowedRect, pendingMode == WindowMode::Windowed);
    DrawOption(borderlessTexture, borderlessRect, pendingMode == WindowMode::Borderless);
    DrawOption(fullscreenTexture, fullscreenRect, pendingMode == WindowMode::Fullscreen);

    if (defaultTexture)
        defaultTexture->Draw(Math::TranslationMatrix(defaultRect.point_1), CS200::WHITE);
    if (applyTexture)
        applyTexture->Draw(Math::TranslationMatrix(applyRect.point_1), CS200::WHITE);
    if (backTexture)
        backTexture->Draw(Math::TranslationMatrix(backRect.point_1), CS200::WHITE);
}

void MainMenu::DrawImGui()
{
}

void MainMenu::Unload()
{
    titleTexture.reset();
    startTexture.reset();
    settingsTexture.reset();
    exitTexture.reset();
    res1600Texture.reset();
    res1280Texture.reset();

    windowedTexture.reset();
    borderlessTexture.reset();
    fullscreenTexture.reset();

    defaultTexture.reset();
    applyTexture.reset();
    backTexture.reset();
    labelResolution.reset();
    labelMode.reset();
}