#include "MainMenu.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/InputMapper.hpp"
#include "Engine/Logger.hpp"
#include "Engine/SettingsManager.hpp"
#include "Engine/Window.hpp"
#include "Mode1.hpp"
#include <imgui.h>

void MainMenu::Load()
{
    Engine::GetWindow().Clear(0x111111FF);
    Engine::GetLogger().LogEvent("Main Menu Loaded");

    CS230::Font& titleFont = Engine::GetFont(0);
    CS230::Font& menuFont  = Engine::GetFont(1);

    titleTexture    = titleFont.PrintToTexture("Ollim", CS200::CYAN);
    startTexture    = menuFont.PrintToTexture("Start Game", CS200::WHITE);
    settingsTexture = menuFont.PrintToTexture("Settings", CS200::WHITE);
    exitTexture     = menuFont.PrintToTexture("Exit", CS200::WHITE);

    tabDisplayTexture  = menuFont.PrintToTexture("Display", CS200::WHITE);
    tabControlsTexture = menuFont.PrintToTexture("Controls", CS200::WHITE);
    backTexture        = menuFont.PrintToTexture("Back", CS200::WHITE);
    waitingTexture     = menuFont.PrintToTexture("Press Any Key...", CS200::YELLOW);

    res1600Texture    = menuFont.PrintToTexture("1600x900", CS200::WHITE);
    res1280Texture    = menuFont.PrintToTexture("1280x720", CS200::WHITE);
    windowedTexture   = menuFont.PrintToTexture("Windowed", CS200::WHITE);
    borderlessTexture = menuFont.PrintToTexture("Borderless", CS200::WHITE);
    fullscreenTexture = menuFont.PrintToTexture("Fullscreen", CS200::WHITE);
    applyTexture      = menuFont.PrintToTexture("Apply Settings", CS200::GREEN);
    labelResolution   = menuFont.PrintToTexture("Resolution", CS200::GREY);
    labelMode         = menuFont.PrintToTexture("Window Mode", CS200::GREY);

    SetupButtons();

    auto& settings  = CS230::SettingsManager::Instance().GetSettings();
    pendingResIndex = (settings.resolutionX == 1600) ? 0 : 1;
    if (settings.fullscreen)
        pendingMode = WindowMode::Fullscreen;
    else if (settings.borderless)
        pendingMode = WindowMode::Borderless;
    else
        pendingMode = WindowMode::Windowed;
}

void MainMenu::SetupButtons()
{
    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    double      centerX = static_cast<double>(winSize.x) * 0.5;
    double      centerY = static_cast<double>(winSize.y) * 0.5;

    double mainStartY = centerY - 50.0;
    double gap        = 70.0;

    if (startTexture)
    {
        Math::ivec2 size = startTexture->GetSize();
        startButtonRect  = {
            { centerX - size.x / 2.0,          mainStartY },
            { centerX + size.x / 2.0, mainStartY + size.y }
        };
    }
    if (settingsTexture)
    {
        Math::ivec2 size   = settingsTexture->GetSize();
        settingsButtonRect = {
            { centerX - size.x / 2.0,          mainStartY - gap },
            { centerX + size.x / 2.0, mainStartY - gap + size.y }
        };
    }
    if (exitTexture)
    {
        Math::ivec2 size = exitTexture->GetSize();
        exitButtonRect   = {
            { centerX - size.x / 2.0,          mainStartY - gap * 2.0 },
            { centerX + size.x / 2.0, mainStartY - gap * 2.0 + size.y }
        };
    }

    double tabY   = static_cast<double>(winSize.y) - 80.0;
    double tabGap = 200.0;

    if (tabDisplayTexture)
    {
        Math::ivec2 size = tabDisplayTexture->GetSize();
        tabDisplayRect   = {
            { centerX - tabGap / 2.0 - size.x,          tabY },
            {          centerX - tabGap / 2.0, tabY + size.y }
        };
    }
    if (tabControlsTexture)
    {
        Math::ivec2 size = tabControlsTexture->GetSize();
        tabControlsRect  = {
            {          centerX + tabGap / 2.0,          tabY },
            { centerX + tabGap / 2.0 + size.x, tabY + size.y }
        };
    }

    if (backTexture)
    {
        Math::ivec2 size = backTexture->GetSize();
        backRect         = {
            { static_cast<double>(winSize.x) - size.x - 30.0,                               30.0 },
            {          static_cast<double>(winSize.x) - 30.0, 30.0 + static_cast<double>(size.y) }
        };
    }

    double dispLeftX  = centerX - 250.0;
    double dispRightX = centerX + 100.0;
    double dispY      = centerY + 50.0;
    double dispGap    = 60.0;

    if (res1600Texture)
        res1600Rect = {
            {                               dispLeftX,                               dispY },
            { dispLeftX + res1600Texture->GetSize().x, dispY + res1600Texture->GetSize().y }
        };
    if (res1280Texture)
        res1280Rect = {
            {                               dispLeftX,                               dispY - dispGap },
            { dispLeftX + res1280Texture->GetSize().x, dispY - dispGap + res1280Texture->GetSize().y }
        };

    if (windowedTexture)
        windowedRect = {
            {                                dispRightX,                                dispY },
            { dispRightX + windowedTexture->GetSize().x, dispY + windowedTexture->GetSize().y }
        };
    if (borderlessTexture)
        borderlessRect = {
            {                                  dispRightX,                                  dispY - dispGap },
            { dispRightX + borderlessTexture->GetSize().x, dispY - dispGap + borderlessTexture->GetSize().y }
        };
    if (fullscreenTexture)
        fullscreenRect = {
            {                                  dispRightX,                                  dispY - dispGap * 2 },
            { dispRightX + fullscreenTexture->GetSize().x, dispY - dispGap * 2 + fullscreenTexture->GetSize().y }
        };

    if (applyTexture)
    {
        Math::ivec2 size = applyTexture->GetSize();
        applyRect        = {
            { centerX - size.x / 2.0,                               100.0 },
            { centerX + size.x / 2.0, 100.0 + static_cast<double>(size.y) }
        };
    }

    keyBindButtons.clear();
    CS230::Font& listFont = Engine::GetFont(1);
    auto&        mapper   = CS230::InputMapper::Instance();

    double listStartY = tabY - 100.0;
    double rowHeight  = 45.0;

    for (int i = 0; i < static_cast<int>(CS230::GameAction::Count); ++i)
    {
        CS230::GameAction action  = static_cast<CS230::GameAction>(i);
        std::string       name    = mapper.ActionToString(action);
        std::string       keyName = "[" + mapper.GetBindingName(action) + "]";

        KeyBindButton btn;
        btn.action      = action;
        btn.actionName  = name;
        btn.nameTexture = listFont.PrintToTexture(name, CS200::WHITE);
        btn.keyTexture  = listFont.PrintToTexture(keyName, CS200::YELLOW);

        double rowY       = listStartY - (i * rowHeight);
        double leftAlign  = centerX - 300.0;
        double rightAlign = centerX + 300.0;

        btn.rect = {
            {  leftAlign,        rowY },
            { rightAlign, rowY + 35.0 }
        };
        keyBindButtons.push_back(btn);
    }
}

void MainMenu::Update([[maybe_unused]] double dt)
{
    if (currentState == MenuState::Main)
        UpdateMainMenu(dt);
    else
        UpdateSettingsMenu(dt);
}

void MainMenu::UpdateMainMenu(double)
{
    auto&       input    = Engine::GetInput();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();
    mousePos.y           = static_cast<double>(winSize.y) - mousePos.y;

    auto CheckHover = [&](const Math::rect& rect)
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
            Engine::GetLogger().LogEvent("Start Game Button Clicked");
            Engine::GetGameStateManager().Clear();
            Engine::GetGameStateManager().PushState<Mode1>();
        }
        else if (isSettingsHovered)
        {
            Engine::GetLogger().LogEvent("Settings Button Clicked");
            currentState = MenuState::Settings;
            currentTab   = SettingsTab::Display;
            SetupButtons();
        }
        else if (isExitHovered)
        {
            Engine::GetLogger().LogEvent("Exit Button Clicked");
            Engine::Instance().Stop();
        }
    }
}

void MainMenu::UpdateSettingsMenu(double)
{
    auto&       input    = Engine::GetInput();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();
    mousePos.y           = static_cast<double>(winSize.y) - mousePos.y;

    auto CheckClick = [&](const Math::rect& rect)
    {
        return input.MouseButtonJustPressed(CS230::Input::MouseButton::Left) && mousePos.x >= rect.Left() && mousePos.x <= rect.Right() && mousePos.y >= rect.Bottom() && mousePos.y <= rect.Top();
    };

    if (isWaitingForKey)
    {
        UpdateControlsTab();
        return;
    }

    if (CheckClick(tabDisplayRect))
    {
        Engine::GetLogger().LogEvent("Switched to Display Tab");
        currentTab = SettingsTab::Display;
    }
    if (CheckClick(tabControlsRect))
    {
        Engine::GetLogger().LogEvent("Switched to Controls Tab");
        currentTab = SettingsTab::Controls;
        SetupButtons();
    }

    if (CheckClick(backRect))
    {
        Engine::GetLogger().LogEvent("Back to Main Menu");
        currentState = MenuState::Main;
        CS230::SettingsManager::Instance().SaveSettings();
        CS230::InputMapper::Instance().SaveBindings();
    }

    if (currentTab == SettingsTab::Display)
    {
        UpdateDisplayTab();
    }
    else
    {
        UpdateControlsTab();
    }
}

void MainMenu::UpdateDisplayTab()
{
    auto&       input    = Engine::GetInput();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();
    mousePos.y           = static_cast<double>(winSize.y) - mousePos.y;

    auto CheckClick = [&](const Math::rect& rect)
    {
        return input.MouseButtonJustPressed(CS230::Input::MouseButton::Left) && mousePos.x >= rect.Left() && mousePos.x <= rect.Right() && mousePos.y >= rect.Bottom() && mousePos.y <= rect.Top();
    };

    if (CheckClick(res1600Rect))
        pendingResIndex = 0;
    if (CheckClick(res1280Rect))
        pendingResIndex = 1;

    if (CheckClick(windowedRect))
        pendingMode = WindowMode::Windowed;
    if (CheckClick(borderlessRect))
        pendingMode = WindowMode::Borderless;
    if (CheckClick(fullscreenRect))
        pendingMode = WindowMode::Fullscreen;

    if (CheckClick(applyRect))
    {
        Engine::GetLogger().LogEvent("Applying Display Settings...");
        auto& settingsMgr = CS230::SettingsManager::Instance();
        settingsMgr.SetResolution(resolutions[pendingResIndex].x, resolutions[pendingResIndex].y);
        settingsMgr.SetWindowMode(pendingMode == WindowMode::Fullscreen, pendingMode == WindowMode::Borderless);
        SetupButtons();
    }
}

void MainMenu::UpdateControlsTab()
{
    auto& input  = Engine::GetInput();
    auto& mapper = CS230::InputMapper::Instance();

    if (isWaitingForKey)
    {
        if (input.KeyJustPressed(CS230::Input::Keys::Escape))
        {
            Engine::GetLogger().LogEvent("Key Binding Cancelled");
            isWaitingForKey = false;
            rebindAction    = CS230::GameAction::Count;
            return;
        }

        for (int i = 0; i < static_cast<int>(CS230::Input::Keys::Count); ++i)
        {
            CS230::Input::Keys key = static_cast<CS230::Input::Keys>(i);
            if (input.KeyJustPressed(key))
            {
                mapper.SetBinding(rebindAction, key);
                Engine::GetLogger().LogEvent("Key Binding Updated: " + mapper.GetBindingName(rebindAction));
                isWaitingForKey = false;
                rebindAction    = CS230::GameAction::Count;
                SetupButtons();
                break;
            }
        }
        return;
    }

    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();
    mousePos.y           = static_cast<double>(winSize.y) - mousePos.y;

    if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        for (const auto& btn : keyBindButtons)
        {
            if (mousePos.x >= btn.rect.Left() && mousePos.x <= btn.rect.Right() && mousePos.y >= btn.rect.Bottom() && mousePos.y <= btn.rect.Top())
            {
                Engine::GetLogger().LogEvent("Rebinding Action: " + btn.actionName);
                isWaitingForKey = true;
                rebindAction    = btn.action;
                break;
            }
        }
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
    if (titleTexture)
    {
        Math::vec2 pos = { static_cast<double>(Engine::GetWindow().GetSize().x) * 0.5 - titleTexture->GetSize().x * 1.5, static_cast<double>(Engine::GetWindow().GetSize().y) * 0.5 + 150.0 };
        titleTexture->Draw(Math::TranslationMatrix(Math::vec2{ pos.x, pos.y }) * Math::ScaleMatrix({ 3.0, 3.0 }), CS200::CYAN);
    }

    auto DrawBtn = [&](std::shared_ptr<CS230::Texture> tex, Math::rect rect, bool hover)
    {
        if (tex)
            tex->Draw(Math::TranslationMatrix(Math::vec2{ rect.point_1.x, rect.point_1.y }), hover ? CS200::YELLOW : CS200::WHITE);
    };

    DrawBtn(startTexture, startButtonRect, isStartHovered);
    DrawBtn(settingsTexture, settingsButtonRect, isSettingsHovered);
    DrawBtn(exitTexture, exitButtonRect, isExitHovered);
}

void MainMenu::DrawSettingsMenu()
{
    auto DrawTab = [&](std::shared_ptr<CS230::Texture> tex, Math::rect rect, bool isActive)
    {
        if (tex)
            tex->Draw(Math::TranslationMatrix(Math::vec2{ rect.point_1.x, rect.point_1.y }), isActive ? CS200::GREEN : CS200::GREY);
    };

    DrawTab(tabDisplayTexture, tabDisplayRect, currentTab == SettingsTab::Display);
    DrawTab(tabControlsTexture, tabControlsRect, currentTab == SettingsTab::Controls);

    if (backTexture)
        backTexture->Draw(Math::TranslationMatrix(Math::vec2{ backRect.point_1.x, backRect.point_1.y }), CS200::WHITE);

    if (currentTab == SettingsTab::Display)
    {
        DrawDisplayTab();
    }
    else
    {
        DrawControlsTab();
    }
}

void MainMenu::DrawDisplayTab()
{
    auto DrawOption = [&](std::shared_ptr<CS230::Texture> tex, Math::rect rect, bool selected)
    {
        if (tex)
            tex->Draw(Math::TranslationMatrix(Math::vec2{ rect.point_1.x, rect.point_1.y }), selected ? CS200::CYAN : CS200::GREY);
    };

    if (labelResolution)
        labelResolution->Draw(Math::TranslationMatrix(Math::vec2{ res1600Rect.Left(), res1600Rect.Top() + 20.0 }), CS200::WHITE);

    DrawOption(res1600Texture, res1600Rect, pendingResIndex == 0);
    DrawOption(res1280Texture, res1280Rect, pendingResIndex == 1);

    if (labelMode)
        labelMode->Draw(Math::TranslationMatrix(Math::vec2{ windowedRect.Left(), windowedRect.Top() + 20.0 }), CS200::WHITE);

    DrawOption(windowedTexture, windowedRect, pendingMode == WindowMode::Windowed);
    DrawOption(borderlessTexture, borderlessRect, pendingMode == WindowMode::Borderless);
    DrawOption(fullscreenTexture, fullscreenRect, pendingMode == WindowMode::Fullscreen);

    if (applyTexture)
        applyTexture->Draw(Math::TranslationMatrix(Math::vec2{ applyRect.point_1.x, applyRect.point_1.y }), CS200::WHITE);
}

void MainMenu::DrawControlsTab()
{
    for (const auto& btn : keyBindButtons)
    {
        if (btn.nameTexture)
        {
            btn.nameTexture->Draw(Math::TranslationMatrix(Math::vec2{ btn.rect.Left(), btn.rect.Bottom() }), CS200::WHITE);
        }

        if (isWaitingForKey && rebindAction == btn.action)
        {
            if (waitingTexture)
            {
                double x = btn.rect.Right() - waitingTexture->GetSize().x;
                waitingTexture->Draw(Math::TranslationMatrix(Math::vec2{ x, btn.rect.Bottom() }), CS200::RED);
            }
        }
        else
        {
            if (btn.keyTexture)
            {
                double x = btn.rect.Right() - btn.keyTexture->GetSize().x;
                btn.keyTexture->Draw(Math::TranslationMatrix(Math::vec2{ x, btn.rect.Bottom() }), CS200::YELLOW);
            }
        }
    }
}

void MainMenu::DrawImGui()
{
    ImGui::Begin("Main Menu Debug");

    ImGui::Text("Menu State: %s", currentState == MenuState::Main ? "Main" : "Settings");

    if (currentState == MenuState::Settings)
    {
        ImGui::Separator();
        ImGui::Text("Active Tab: %s", currentTab == SettingsTab::Display ? "Display" : "Controls");

        if (currentTab == SettingsTab::Display)
        {
            ImGui::Text("Selected Resolution: %s", pendingResIndex == 0 ? "1600x900" : "1280x720");
            const char* modes[] = { "Windowed", "Borderless", "Fullscreen" };
            ImGui::Text("Selected Mode: %s", modes[static_cast<int>(pendingMode)]);
        }
        else if (currentTab == SettingsTab::Controls)
        {
            if (isWaitingForKey)
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "STATUS: WAITING FOR KEY INPUT...");
            }
            else
            {
                ImGui::Text("Status: Idle");
            }
        }
    }

    ImGui::Separator();
    ImGui::Text("Mouse Pos (Screen): (%.1f, %.1f)", Engine::GetInput().GetMousePosition().x, Engine::GetInput().GetMousePosition().y);

    static bool showDebugRects = false;
    ImGui::Checkbox("Show UI Rects", &showDebugRects);

    if (showDebugRects)
    {
        auto DrawDebugRect = [](const Math::rect& r, const char* label)
        {
            ImGui::GetForegroundDrawList()->AddRect(
                ImVec2(static_cast<float>(r.Left()), static_cast<float>(Engine::GetWindow().GetSize().y - r.Top())),
                ImVec2(static_cast<float>(r.Right()), static_cast<float>(Engine::GetWindow().GetSize().y - r.Bottom())), IM_COL32(255, 0, 255, 255));
            ImGui::GetForegroundDrawList()->AddText(ImVec2(static_cast<float>(r.Left()), static_cast<float>(Engine::GetWindow().GetSize().y - r.Top() - 15.0)), IM_COL32(255, 0, 255, 255), label);
        };

        if (currentState == MenuState::Main)
        {
            DrawDebugRect(startButtonRect, "Start");
            DrawDebugRect(settingsButtonRect, "Settings");
            DrawDebugRect(exitButtonRect, "Exit");
        }
        else if (currentTab == SettingsTab::Display)
        {
            DrawDebugRect(res1600Rect, "1600x900");
            DrawDebugRect(res1280Rect, "1280x720");
            DrawDebugRect(applyRect, "Apply");
        }
    }

    ImGui::End();
}

void MainMenu::Unload()
{
    Engine::GetLogger().LogEvent("Main Menu Unloaded");
    titleTexture.reset();
    startTexture.reset();
    settingsTexture.reset();
    exitTexture.reset();

    tabDisplayTexture.reset();
    tabControlsTexture.reset();
    backTexture.reset();
    waitingTexture.reset();

    res1600Texture.reset();
    res1280Texture.reset();
    windowedTexture.reset();
    borderlessTexture.reset();
    fullscreenTexture.reset();
    applyTexture.reset();

    keyBindButtons.clear();
}