#include "MainMenu.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "CS200/RenderingAPI.hpp"
#include "Engine/AudioManager.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/InputMapper.hpp"
#include "Engine/Logger.hpp"
#include "Engine/SettingsManager.hpp"
#include "Engine/Window.hpp"
#include "Mode1.hpp"
#include <algorithm>
#include <imgui.h>
#include <initializer_list>

namespace
{
    Math::rect MakeRect(Math::vec2 bottomLeft, Math::vec2 size)
    {
        return {
            bottomLeft,
            { bottomLeft.x + size.x, bottomLeft.y + size.y }
        };
    }

    Math::TransformationMatrix RectTransform(const Math::rect& rect)
    {
        const Math::vec2 size   = rect.Size();
        const Math::vec2 center = { rect.Left() + size.x * 0.5, rect.Bottom() + size.y * 0.5 };
        return Math::TranslationMatrix(center) * Math::ScaleMatrix(size);
    }

    bool ContainsPoint(const Math::rect& rect, Math::vec2 point)
    {
        return point.x >= rect.Left() && point.x <= rect.Right() && point.y >= rect.Bottom() && point.y <= rect.Top();
    }

    Math::vec2 TextPositionInRect(const Math::rect& rect, const CS230::Texture& texture)
    {
        const Math::ivec2 texSize = texture.GetSize();
        return {
            rect.Left() + 12.0,
            rect.Bottom() + (rect.Size().y - static_cast<double>(texSize.y)) * 0.5
        };
    }

    Math::vec2 DropdownSize(std::initializer_list<std::shared_ptr<CS230::Texture>> textures)
    {
        double maxWidth  = 0.0;
        double maxHeight = 0.0;

        for (const auto& texture : textures)
        {
            if (texture == nullptr)
            {
                continue;
            }

            const Math::ivec2 size = texture->GetSize();
            maxWidth               = std::max(maxWidth, static_cast<double>(size.x));
            maxHeight              = std::max(maxHeight, static_cast<double>(size.y));
        }

        return {
            std::max(260.0, maxWidth + 43.0),
            std::max(52.0, maxHeight + 20.0)
        };
    }
}

void MainMenu::Load()
{
    AudioManager::StopBGM();

    // Clear the background to a dark color on entry
    Engine::GetWindow().Clear(0x111111FF);
    Engine::GetLogger().LogEvent("Main Menu Loaded");

    // Fetch fonts for title and UI text rendering
    CS230::Font& titleFont = Engine::GetFont(0);
    CS230::Font& menuFont  = Engine::GetFont(1);

    // Render static strings into textures for efficient drawing
    titleTexture    = titleFont.PrintToTexture("Ollim", CS200::CYAN);
    startTexture    = menuFont.PrintToTexture("Start Game", CS200::WHITE);
    settingsTexture = menuFont.PrintToTexture("Settings", CS200::WHITE);
    exitTexture     = menuFont.PrintToTexture("Exit", CS200::WHITE);

    tabDisplayTexture  = menuFont.PrintToTexture("Display", CS200::WHITE);
    tabControlsTexture = menuFont.PrintToTexture("Controls", CS200::WHITE);
    tabSoundTexture    = menuFont.PrintToTexture("Sound", CS200::WHITE);
    backTexture        = menuFont.PrintToTexture("Back", CS200::WHITE);
    waitingTexture     = menuFont.PrintToTexture("Press Any Key...", CS200::YELLOW);

    res3840Texture      = menuFont.PrintToTexture("3840x2160", CS200::WHITE);
    res2560Texture      = menuFont.PrintToTexture("2560x1440", CS200::WHITE);
    res1920Texture      = menuFont.PrintToTexture("1920x1080", CS200::WHITE);
    res1600Texture      = menuFont.PrintToTexture("1600x900", CS200::WHITE);
    res1280Texture      = menuFont.PrintToTexture("1280x720", CS200::WHITE);
    windowedTexture     = menuFont.PrintToTexture("Windowed", CS200::WHITE);
    borderlessTexture   = menuFont.PrintToTexture("Borderless", CS200::WHITE);
    fullscreenTexture   = menuFont.PrintToTexture("Fullscreen", CS200::WHITE);
    fps30Texture        = menuFont.PrintToTexture("30 FPS", CS200::WHITE);
    fps60Texture        = menuFont.PrintToTexture("60 FPS", CS200::WHITE);
    fps120Texture       = menuFont.PrintToTexture("120 FPS", CS200::WHITE);
    fps144Texture       = menuFont.PrintToTexture("144 FPS", CS200::WHITE);
    fpsUnlimitedTexture = menuFont.PrintToTexture("Unlimited", CS200::WHITE);
    applyTexture        = menuFont.PrintToTexture("Apply Settings", CS200::GREEN);
    labelResolution     = menuFont.PrintToTexture("Resolution", CS200::GREY);
    labelMode           = menuFont.PrintToTexture("Window Mode", CS200::GREY);
    labelFrameLimit     = menuFont.PrintToTexture("Frame Limit", CS200::GREY);

    // Sound
    labelBGM = menuFont.PrintToTexture("BGM Volume", CS200::GREY);
    labelSFX = menuFont.PrintToTexture("SFX Volume", CS200::GREY);

    auto& settingsMgr = CS230::SettingsManager::Instance();
    bgmLevel          = static_cast<int>(settingsMgr.GetBGMVolume() * 10.0f);
    sfxLevel          = static_cast<int>(settingsMgr.GetSFXVolume() * 10.0f);
    UpdateVolumeTextures();

    // Calculate initial positions for all UI rectangles
    SetupButtons();

    // Synchronize the UI state with current engine settings
    auto& settings  = CS230::SettingsManager::Instance().GetSettings();
    pendingResIndex = ResolutionOptionCount - 1;
    for (int i = 0; i < ResolutionOptionCount; ++i)
    {
        if (settings.resolutionX == resolutions[i].x && settings.resolutionY == resolutions[i].y)
        {
            pendingResIndex = i;
            break;
        }
    }

    pendingFrameLimitIndex = 1;
    for (int i = 0; i < FrameLimitOptionCount; ++i)
    {
        if (settings.frameLimit == frameLimits[i])
        {
            pendingFrameLimitIndex = i;
            break;
        }
    }

    if (settings.fullscreen)
        pendingMode = WindowMode::Fullscreen;
    else if (settings.borderless)
        pendingMode = WindowMode::Borderless;
    else
        pendingMode = WindowMode::Windowed;
}

void MainMenu::SetupButtons()
{
    // Determine screen center for layout calculations
    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    lastLayoutWindowSize = winSize;

    double      centerX = static_cast<double>(winSize.x) * 0.5;
    double      centerY = static_cast<double>(winSize.y) * 0.5;

    double mainStartY = centerY - 50.0;
    double gap        = 70.0;

    // Define hitboxes for the main menu stack
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

    // Define positions for top settings tabs
    double tabY   = static_cast<double>(winSize.y) - 80.0;
    double tabGap = 300.0;

    if (tabDisplayTexture)
    {
        Math::ivec2 size = tabDisplayTexture->GetSize();
        double      x    = centerX - tabGap - (size.x / 2.0);
        tabDisplayRect   = {
            {          x,          tabY },
            { x + size.x, tabY + size.y }
        };
    }
    if (tabControlsTexture)
    {
        Math::ivec2 size = tabControlsTexture->GetSize();
        double      x    = centerX - (size.x / 2.0);
        tabControlsRect  = {
            {          x,          tabY },
            { x + size.x, tabY + size.y }
        };
    }
    if (tabSoundTexture)
    {
        Math::ivec2 size = tabSoundTexture->GetSize();
        double      x    = centerX + tabGap - (size.x / 2.0);
        tabSoundRect     = {
            {          x,          tabY },
            { x + size.x, tabY + size.y }
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

    // Dropdown layout for display options
    const Math::vec2 resolutionDropdownSize = DropdownSize({ res3840Texture, res2560Texture, res1920Texture, res1600Texture, res1280Texture });
    const Math::vec2 modeDropdownSize       = DropdownSize({ windowedTexture, borderlessTexture, fullscreenTexture });
    const Math::vec2 frameDropdownSize      = DropdownSize({ fps30Texture, fps60Texture, fps120Texture, fps144Texture, fpsUnlimitedTexture });
    const double     resolutionGap          = resolutionDropdownSize.y + 4.0;
    const double     modeGap                = modeDropdownSize.y + 4.0;
    const double     frameGap               = frameDropdownSize.y + 4.0;
    const double     dispY                  = centerY + 95.0;
    const double     resolutionX            = centerX - 470.0;
    const double     modeX                  = centerX - 120.0;
    const double     frameX                 = centerX + 230.0;

    resolutionDropdownRect = MakeRect({ resolutionX, dispY }, resolutionDropdownSize);
    modeDropdownRect       = MakeRect({ modeX, dispY }, modeDropdownSize);
    frameLimitDropdownRect = MakeRect({ frameX, dispY }, frameDropdownSize);

    res3840Rect = MakeRect({ resolutionX, dispY - resolutionGap }, resolutionDropdownSize);
    res2560Rect = MakeRect({ resolutionX, dispY - resolutionGap * 2.0 }, resolutionDropdownSize);
    res1920Rect = MakeRect({ resolutionX, dispY - resolutionGap * 3.0 }, resolutionDropdownSize);
    res1600Rect = MakeRect({ resolutionX, dispY - resolutionGap * 4.0 }, resolutionDropdownSize);
    res1280Rect = MakeRect({ resolutionX, dispY - resolutionGap * 5.0 }, resolutionDropdownSize);

    windowedRect   = MakeRect({ modeX, dispY - modeGap }, modeDropdownSize);
    borderlessRect = MakeRect({ modeX, dispY - modeGap * 2.0 }, modeDropdownSize);
    fullscreenRect = MakeRect({ modeX, dispY - modeGap * 3.0 }, modeDropdownSize);

    fps30Rect        = MakeRect({ frameX, dispY - frameGap }, frameDropdownSize);
    fps60Rect        = MakeRect({ frameX, dispY - frameGap * 2.0 }, frameDropdownSize);
    fps120Rect       = MakeRect({ frameX, dispY - frameGap * 3.0 }, frameDropdownSize);
    fps144Rect       = MakeRect({ frameX, dispY - frameGap * 4.0 }, frameDropdownSize);
    fpsUnlimitedRect = MakeRect({ frameX, dispY - frameGap * 5.0 }, frameDropdownSize);

    if (applyTexture)
    {
        Math::ivec2 size = applyTexture->GetSize();
        applyRect        = {
            { centerX - size.x / 2.0,                               100.0 },
            { centerX + size.x / 2.0, 100.0 + static_cast<double>(size.y) }
        };
    }

    // Clear and build the dynamic list for key binding buttons
    keyBindButtons.clear();
    keyBindButtons.reserve(static_cast<size_t>(CS230::GameAction::Count));

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

    double bgmBarY    = centerY + 60.0;
    double sfxBarY    = centerY - 160.0;
    double blockGap   = 55.0;
    double totalWidth = 10 * blockGap;
    double startX     = centerX - (totalWidth / 2.0) + (blockGap / 2.0);

    for (int i = 0; i < 10; ++i)
    {
        bgmBarRects[i] = {
            { startX + i * blockGap - 20.0,        bgmBarY },
            { startX + i * blockGap + 20.0, bgmBarY + 30.0 }
        };
        sfxBarRects[i] = {
            { startX + i * blockGap - 20.0,        sfxBarY },
            { startX + i * blockGap + 20.0, sfxBarY + 30.0 }
        };
    }
}

void MainMenu::UpdateVolumeTextures()
{
    CS230::Font& menuFont = Engine::GetFont(1);
    valBGMTexture         = menuFont.PrintToTexture(std::to_string(bgmLevel), CS200::WHITE);
    valSFXTexture         = menuFont.PrintToTexture(std::to_string(sfxLevel), CS200::WHITE);
}

void MainMenu::Update([[maybe_unused]] double dt)
{
    if (Engine::GetWindow().GetSize() != lastLayoutWindowSize)
    {
        SetupButtons();
    }

    // Route update logic based on current menu layer
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

    // Flip Y coordinate as input is top-left but UI layout is often bottom-left
    mousePos.y = static_cast<double>(winSize.y) - mousePos.y;

    // Helper lambda to check if mouse coordinate is inside a rectangle
    auto CheckHover = [&](const Math::rect& rect)
    {
        return mousePos.x >= rect.Left() && mousePos.x <= rect.Right() && mousePos.y >= rect.Bottom() && mousePos.y <= rect.Top();
    };

    // Update hovering status for highlighting
    isStartHovered    = CheckHover(startButtonRect);
    isSettingsHovered = CheckHover(settingsButtonRect);
    isExitHovered     = CheckHover(exitButtonRect);

    // Handle clicks for each button
    if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        if (isStartHovered)
        {
            Engine::GetLogger().LogEvent("Start Game Button Clicked");
            // Engine::GetGameStateManager().Clear();
            // Engine::GetGameStateManager().PushState<Mode1>();
            Engine::GetGameStateManager().ChangeStateWithFade<Mode1>();
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

    // Adjust mouse Y for internal coordinate consistency
    mousePos.y = static_cast<double>(winSize.y) - mousePos.y;

    // Local helper to detect mouse clicks within a specific UI rectangle
    auto CheckClick = [&](const Math::rect& rect)
    {
        return input.MouseButtonJustPressed(CS230::Input::MouseButton::Left) && mousePos.x >= rect.Left() && mousePos.x <= rect.Right() && mousePos.y >= rect.Bottom() && mousePos.y <= rect.Top();
    };

    // If the menu is waiting for a key press (rebinding), bypass normal menu clicks
    if (isWaitingForKey)
    {
        UpdateControlsTab();
        return;
    }

    // Tab switching logic
    if (CheckClick(tabDisplayRect))
    {
        Engine::GetLogger().LogEvent("Switched to Display Tab");
        currentTab = SettingsTab::Display;
        resolutionDropdownOpen = false;
        modeDropdownOpen       = false;
        frameLimitDropdownOpen = false;
    }
    if (CheckClick(tabControlsRect))
    {
        Engine::GetLogger().LogEvent("Switched to Controls Tab");
        currentTab = SettingsTab::Controls;
        resolutionDropdownOpen = false;
        modeDropdownOpen       = false;
        frameLimitDropdownOpen = false;
        SetupButtons();
    }
    if (CheckClick(tabSoundRect))
    {
        Engine::GetLogger().LogEvent("Switched to Sound Tab");
        currentTab = SettingsTab::Sound;
        resolutionDropdownOpen = false;
        modeDropdownOpen       = false;
        frameLimitDropdownOpen = false;
    }

    // Save settings and return to main menu when 'Back' is clicked
    if (CheckClick(backRect))
    {
        Engine::GetLogger().LogEvent("Back to Main Menu");
        currentState = MenuState::Main;
        resolutionDropdownOpen = false;
        modeDropdownOpen       = false;
        frameLimitDropdownOpen = false;
        CS230::SettingsManager::Instance().SaveSettings();
        CS230::InputMapper::Instance().SaveBindings();
    }

    // Route logic to the active settings tab
    if (currentTab == SettingsTab::Display)
    {
        UpdateDisplayTab();
    }
    else if (currentTab == SettingsTab::Controls)
    {
        UpdateControlsTab();
    }
    else if (currentTab == SettingsTab::Sound)
    {
        UpdateSoundTab();
    }
}

void MainMenu::UpdateDisplayTab()
{
    auto&       input    = Engine::GetInput();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();
    mousePos.y           = static_cast<double>(winSize.y) - mousePos.y;
    const bool  clicked  = input.MouseButtonJustPressed(CS230::Input::MouseButton::Left);

    if (!clicked)
    {
        return;
    }

    const Math::rect resolutionRects[ResolutionOptionCount] = { res3840Rect, res2560Rect, res1920Rect, res1600Rect, res1280Rect };
    const Math::rect modeRects[WindowModeOptionCount]       = { windowedRect, borderlessRect, fullscreenRect };
    const Math::rect frameRects[FrameLimitOptionCount]      = { fps30Rect, fps60Rect, fps120Rect, fps144Rect, fpsUnlimitedRect };

    auto closeAllDropdowns = [&]()
    {
        resolutionDropdownOpen = false;
        modeDropdownOpen       = false;
        frameLimitDropdownOpen = false;
    };

    auto handleOpenDropdown = [&](const Math::rect& buttonRect, const Math::rect* optionRects, int optionCount, int& selectedIndex, bool& isOpen) -> bool
    {
        if (!isOpen)
        {
            return false;
        }

        if (ContainsPoint(buttonRect, mousePos))
        {
            isOpen = false;
            return true;
        }

        for (int i = 0; i < optionCount; ++i)
        {
            if (ContainsPoint(optionRects[i], mousePos))
            {
                selectedIndex = i;
                isOpen        = false;
                return true;
            }
        }

        isOpen = false;
        return false;
    };

    int pendingModeIndex = static_cast<int>(pendingMode);
    if (handleOpenDropdown(resolutionDropdownRect, resolutionRects, ResolutionOptionCount, pendingResIndex, resolutionDropdownOpen))
    {
        return;
    }
    if (handleOpenDropdown(modeDropdownRect, modeRects, WindowModeOptionCount, pendingModeIndex, modeDropdownOpen))
    {
        pendingMode = static_cast<WindowMode>(pendingModeIndex);
        return;
    }
    pendingMode = static_cast<WindowMode>(pendingModeIndex);
    if (handleOpenDropdown(frameLimitDropdownRect, frameRects, FrameLimitOptionCount, pendingFrameLimitIndex, frameLimitDropdownOpen))
    {
        return;
    }

    if (ContainsPoint(resolutionDropdownRect, mousePos))
    {
        closeAllDropdowns();
        resolutionDropdownOpen = true;
        return;
    }
    if (ContainsPoint(modeDropdownRect, mousePos))
    {
        closeAllDropdowns();
        modeDropdownOpen = true;
        return;
    }
    if (ContainsPoint(frameLimitDropdownRect, mousePos))
    {
        closeAllDropdowns();
        frameLimitDropdownOpen = true;
        return;
    }

    // Apply the selected settings to the actual engine system
    if (ContainsPoint(applyRect, mousePos))
    {
        Engine::GetLogger().LogEvent("Applying Display Settings...");
        auto& settingsMgr = CS230::SettingsManager::Instance();
        settingsMgr.SetWindowMode(false, pendingMode == WindowMode::Borderless);
        settingsMgr.SetResolution(resolutions[pendingResIndex].x, resolutions[pendingResIndex].y);
        settingsMgr.SetWindowMode(pendingMode == WindowMode::Fullscreen, pendingMode == WindowMode::Borderless);
        settingsMgr.SetFrameLimit(frameLimits[pendingFrameLimitIndex]);
        closeAllDropdowns();
        SetupButtons(); // Re-center UI elements for the new resolution
    }
}

void MainMenu::UpdateControlsTab()
{
    auto& input  = Engine::GetInput();
    auto& mapper = CS230::InputMapper::Instance();

    // Logic to capture the next key press for rebinding
    if (isWaitingForKey)
    {
        if (input.KeyJustPressed(CS230::Input::Keys::Escape))
        {
            Engine::GetLogger().LogEvent("Key Binding Cancelled");
            isWaitingForKey = false;
            rebindAction    = CS230::GameAction::Count;
            return;
        }

        // Iterate through all possible keys to see which one was pressed
        for (int i = 0; i < static_cast<int>(CS230::Input::Keys::Count); ++i)
        {
            CS230::Input::Keys key = static_cast<CS230::Input::Keys>(i);
            if (input.KeyJustPressed(key))
            {
                mapper.SetBinding(rebindAction, key);
                Engine::GetLogger().LogEvent("Key Binding Updated: " + mapper.GetBindingName(rebindAction));
                isWaitingForKey = false;
                rebindAction    = CS230::GameAction::Count;
                SetupButtons(); // Refresh textures to show the new key name
                break;
            }
        }
        return;
    }

    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();
    mousePos.y           = static_cast<double>(winSize.y) - mousePos.y;

    // Detect which action the user wants to rebind
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

void MainMenu::UpdateSoundTab()
{
    auto&       input    = Engine::GetInput();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    Math::vec2  mousePos = input.GetMousePosition();
    mousePos.y           = static_cast<double>(winSize.y) - mousePos.y;

    auto CheckClick = [&](const Math::rect& rect)
    {
        return input.MouseButtonJustPressed(CS230::Input::MouseButton::Left) && mousePos.x >= rect.Left() && mousePos.x <= rect.Right() && mousePos.y >= rect.Bottom() && mousePos.y <= rect.Top();
    };

    bool volumeChanged = false;

    for (int i = 0; i < 10; ++i)
    {
        if (CheckClick(bgmBarRects[i]))
        {
            if (i == 0 && bgmLevel == 1)
                bgmLevel = 0;
            else
                bgmLevel = i + 1;
            volumeChanged = true;
        }

        if (CheckClick(sfxBarRects[i]))
        {
            if (i == 0 && sfxLevel == 1)
                sfxLevel = 0;
            else
                sfxLevel = i + 1;
            volumeChanged = true;
        }
    }

    if (volumeChanged)
    {
        UpdateVolumeTextures();

        float bgmFloat = static_cast<float>(bgmLevel) / 10.0f;
        float sfxFloat = static_cast<float>(sfxLevel) / 10.0f;

        CS230::SettingsManager::Instance().SetBGMVolume(bgmFloat);
        CS230::SettingsManager::Instance().SetSFXVolume(sfxFloat);

        AudioManager::SetBGMVolume(static_cast<int>(bgmFloat * 128.0f));
        AudioManager::SetSFXVolume(static_cast<int>(sfxFloat * 128.0f));
    }
}

void MainMenu::Draw()
{
    Engine::GetWindow().Clear(0x111111FF);
    auto&       renderer = Engine::GetRenderer2D();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();

    // Global UI projection setup
    CS200::RenderingAPI::SetViewport(winSize);
    renderer.BeginScene(CS200::build_ndc_matrix(winSize));

    if (currentState == MenuState::Main)
        DrawMainMenu();
    else
        DrawSettingsMenu();

    if (showDebugRects)
    {
        DrawDebugUIRects();
    }

    renderer.EndScene();
}

void MainMenu::DrawMainMenu()
{
    // Draw scaled title text
    if (titleTexture)
    {
        Math::vec2 pos = { static_cast<double>(Engine::GetWindow().GetSize().x) * 0.5 - titleTexture->GetSize().x * 1.5, static_cast<double>(Engine::GetWindow().GetSize().y) * 0.5 + 150.0 };
        titleTexture->Draw(Math::TranslationMatrix(Math::vec2{ pos.x, pos.y }) * Math::ScaleMatrix({ 3.0, 3.0 }), CS200::CYAN);
    }

    // Lambda to draw buttons with hover highlights
    auto DrawBtn = [&](std::shared_ptr<CS230::Texture> tex, Math::rect rect, bool hover)
    {
        if (tex)
            tex->Draw(Math::TranslationMatrix(Math::vec2{ rect.Left(), rect.Bottom() }), hover ? CS200::YELLOW : CS200::WHITE);
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
            tex->Draw(Math::TranslationMatrix(Math::vec2{ rect.Left(), rect.Bottom() }), isActive ? CS200::GREEN : CS200::GREY);
    };

    // Render tab headers
    DrawTab(tabDisplayTexture, tabDisplayRect, currentTab == SettingsTab::Display);
    DrawTab(tabControlsTexture, tabControlsRect, currentTab == SettingsTab::Controls);
    DrawTab(tabSoundTexture, tabSoundRect, currentTab == SettingsTab::Sound);

    if (backTexture)
        backTexture->Draw(Math::TranslationMatrix(Math::vec2{ backRect.Left(), backRect.Bottom() }), CS200::WHITE);

    // Route rendering to the active settings tab content
    if (currentTab == SettingsTab::Display)
    {
        DrawDisplayTab();
    }
    else if (currentTab == SettingsTab::Controls)
    {
        DrawControlsTab();
    }
    else if (currentTab == SettingsTab::Sound)
    {
        DrawSoundTab();
    }
}

void MainMenu::DrawDisplayTab()
{
    auto& renderer = Engine::GetRenderer2D();

    const std::shared_ptr<CS230::Texture> resolutionTextures[ResolutionOptionCount] = { res3840Texture, res2560Texture, res1920Texture, res1600Texture, res1280Texture };
    const Math::rect                      resolutionRects[ResolutionOptionCount]    = { res3840Rect, res2560Rect, res1920Rect, res1600Rect, res1280Rect };
    const std::shared_ptr<CS230::Texture> modeTextures[WindowModeOptionCount]       = { windowedTexture, borderlessTexture, fullscreenTexture };
    const Math::rect                      modeRects[WindowModeOptionCount]          = { windowedRect, borderlessRect, fullscreenRect };
    const std::shared_ptr<CS230::Texture> frameTextures[FrameLimitOptionCount]      = { fps30Texture, fps60Texture, fps120Texture, fps144Texture, fpsUnlimitedTexture };
    const Math::rect                      frameRects[FrameLimitOptionCount]         = { fps30Rect, fps60Rect, fps120Rect, fps144Rect, fpsUnlimitedRect };

    auto DrawTextureInRect = [](const std::shared_ptr<CS230::Texture>& tex, const Math::rect& rect, CS200::RGBA color)
    {
        if (tex)
        {
            tex->Draw(Math::TranslationMatrix(TextPositionInRect(rect, *tex)), color);
        }
    };

    auto DrawArrow = [&](const Math::rect& rect, bool open)
    {
        const double x = rect.Right() - 20.0;
        const double y = rect.Bottom() + rect.Size().y * 0.5;
        if (open)
        {
            renderer.DrawLine({ x - 6.0, y - 3.0 }, { x, y + 4.0 }, CS200::WHITE, 2.0);
            renderer.DrawLine({ x + 6.0, y - 3.0 }, { x, y + 4.0 }, CS200::WHITE, 2.0);
        }
        else
        {
            renderer.DrawLine({ x - 6.0, y + 3.0 }, { x, y - 4.0 }, CS200::WHITE, 2.0);
            renderer.DrawLine({ x + 6.0, y + 3.0 }, { x, y - 4.0 }, CS200::WHITE, 2.0);
        }
    };

    auto DrawDropdown = [&](const std::shared_ptr<CS230::Texture>& label, const Math::rect& buttonRect, const std::shared_ptr<CS230::Texture>& selectedTexture,
                            const std::shared_ptr<CS230::Texture>* optionTextures, const Math::rect* optionRects, int optionCount, int selectedIndex, bool isOpen)
    {
        if (label)
        {
            label->Draw(Math::TranslationMatrix(Math::vec2{ buttonRect.Left(), buttonRect.Top() + 18.0 }), CS200::WHITE);
        }

        renderer.DrawRectangle(RectTransform(buttonRect), 0x000000A8, isOpen ? CS200::CYAN : CS200::WHITE, 2.0);
        DrawTextureInRect(selectedTexture, buttonRect, CS200::WHITE);
        DrawArrow(buttonRect, isOpen);

        if (!isOpen)
        {
            return;
        }

        for (int i = 0; i < optionCount; ++i)
        {
            const bool selected = i == selectedIndex;
            renderer.DrawRectangle(RectTransform(optionRects[i]), selected ? 0x003040D8 : 0x000000D8, selected ? CS200::CYAN : CS200::GREY, 1.0);
            DrawTextureInRect(optionTextures[i], optionRects[i], selected ? CS200::CYAN : CS200::WHITE);
        }
    };

    DrawDropdown(labelResolution, resolutionDropdownRect, resolutionTextures[pendingResIndex], resolutionTextures, resolutionRects, ResolutionOptionCount, pendingResIndex, resolutionDropdownOpen);
    DrawDropdown(labelMode, modeDropdownRect, modeTextures[static_cast<int>(pendingMode)], modeTextures, modeRects, WindowModeOptionCount, static_cast<int>(pendingMode), modeDropdownOpen);
    DrawDropdown(labelFrameLimit, frameLimitDropdownRect, frameTextures[pendingFrameLimitIndex], frameTextures, frameRects, FrameLimitOptionCount, pendingFrameLimitIndex,
                 frameLimitDropdownOpen);

    if (applyTexture)
        applyTexture->Draw(Math::TranslationMatrix(Math::vec2{ applyRect.Left(), applyRect.Bottom() }), CS200::WHITE);
}

void MainMenu::DrawDebugRect(const Math::rect& rect)
{
    auto&      renderer = Engine::GetRenderer2D();
    Math::vec2 size     = rect.Size();

    if (size.x <= 0.0 || size.y <= 0.0)
    {
        return;
    }

    const Math::vec2 center = { rect.Left() + size.x * 0.5, rect.Bottom() + size.y * 0.5 };
    const auto       transform = Math::TranslationMatrix(center) * Math::ScaleMatrix(size);
    renderer.DrawRectangle(transform, CS200::CLEAR, 0xFF00FFFF, 2.0);
}

void MainMenu::DrawDebugUIRects()
{
    if (currentState == MenuState::Main)
    {
        DrawDebugRect(startButtonRect);
        DrawDebugRect(settingsButtonRect);
        DrawDebugRect(exitButtonRect);
        return;
    }

    DrawDebugRect(tabDisplayRect);
    DrawDebugRect(tabControlsRect);
    DrawDebugRect(tabSoundRect);
    DrawDebugRect(backRect);

    if (currentTab == SettingsTab::Display)
    {
        DrawDebugRect(resolutionDropdownRect);
        DrawDebugRect(modeDropdownRect);
        DrawDebugRect(frameLimitDropdownRect);
        if (resolutionDropdownOpen)
        {
            DrawDebugRect(res3840Rect);
            DrawDebugRect(res2560Rect);
            DrawDebugRect(res1920Rect);
            DrawDebugRect(res1600Rect);
            DrawDebugRect(res1280Rect);
        }
        if (modeDropdownOpen)
        {
            DrawDebugRect(windowedRect);
            DrawDebugRect(borderlessRect);
            DrawDebugRect(fullscreenRect);
        }
        if (frameLimitDropdownOpen)
        {
            DrawDebugRect(fps30Rect);
            DrawDebugRect(fps60Rect);
            DrawDebugRect(fps120Rect);
            DrawDebugRect(fps144Rect);
            DrawDebugRect(fpsUnlimitedRect);
        }
        DrawDebugRect(applyRect);
    }
    else if (currentTab == SettingsTab::Controls)
    {
        for (const auto& btn : keyBindButtons)
        {
            DrawDebugRect(btn.rect);
        }
    }
    else if (currentTab == SettingsTab::Sound)
    {
        for (int i = 0; i < 10; ++i)
        {
            DrawDebugRect(bgmBarRects[i]);
            DrawDebugRect(sfxBarRects[i]);
        }
    }
}

void MainMenu::DrawControlsTab()
{
    // Render Window Mode options
    for (const auto& btn : keyBindButtons)
    {
        if (btn.nameTexture)
        {
            btn.nameTexture->Draw(Math::TranslationMatrix(Math::vec2{ btn.rect.Left(), btn.rect.Bottom() }), CS200::WHITE);
        }

        // Show "Press Any Key..." if this specific action is being rebound
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
            // Show the currently bound key
            if (btn.keyTexture)
            {
                double x = btn.rect.Right() - btn.keyTexture->GetSize().x;
                btn.keyTexture->Draw(Math::TranslationMatrix(Math::vec2{ x, btn.rect.Bottom() }), CS200::YELLOW);
            }
        }
    }
}

void MainMenu::DrawSoundTab()
{
    auto&       renderer = Engine::GetRenderer2D();
    Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    double      centerX  = winSize.x * 0.5;
    double      centerY  = winSize.y * 0.5;

    // BGM Rendering
    if (labelBGM)
        labelBGM->Draw(Math::TranslationMatrix(Math::vec2{ centerX - labelBGM->GetSize().x / 2.0, centerY + 120.0 }), CS200::WHITE);

    for (int i = 0; i < 10; ++i)
    {
        double width       = bgmBarRects[i].Right() - bgmBarRects[i].Left();
        double height      = bgmBarRects[i].Top() - bgmBarRects[i].Bottom();
        double rectCenterX = bgmBarRects[i].Left() + width / 2.0;
        double rectCenterY = bgmBarRects[i].Bottom() + height / 2.0;

        Math::TransformationMatrix transform = Math::TranslationMatrix(Math::vec2{ rectCenterX, rectCenterY }) * Math::ScaleMatrix(Math::vec2{ width, height });

        if (i < bgmLevel)
            renderer.DrawRectangle(transform, CS200::GREEN, CS200::WHITE, 2.0);
        else
            renderer.DrawRectangle(transform, CS200::CLEAR, CS200::GREY, 2.0);
    }

    if (valBGMTexture)
    {
        double scaledWidth = valBGMTexture->GetSize().x * 0.5;
        valBGMTexture->Draw(Math::TranslationMatrix(Math::vec2{ centerX - scaledWidth / 2.0, centerY - 10 }) * Math::ScaleMatrix(Math::vec2{ 0.8, 0.8 }), CS200::YELLOW);
    }

    // SFX Rendering
    if (labelSFX)
        labelSFX->Draw(Math::TranslationMatrix(Math::vec2{ centerX - labelSFX->GetSize().x / 2.0, centerY - 100.0 }), CS200::WHITE);

    for (int i = 0; i < 10; ++i)
    {
        double width       = sfxBarRects[i].Right() - sfxBarRects[i].Left();
        double height      = sfxBarRects[i].Top() - sfxBarRects[i].Bottom();
        double rectCenterX = sfxBarRects[i].Left() + width / 2.0;
        double rectCenterY = sfxBarRects[i].Bottom() + height / 2.0;

        Math::TransformationMatrix transform = Math::TranslationMatrix(Math::vec2{ rectCenterX, rectCenterY }) * Math::ScaleMatrix(Math::vec2{ width, height });

        if (i < sfxLevel)
            renderer.DrawRectangle(transform, CS200::GREEN, CS200::WHITE, 2.0);
        else
            renderer.DrawRectangle(transform, CS200::CLEAR, CS200::GREY, 2.0);
    }

    if (valSFXTexture)
    {
        double scaledWidth = valSFXTexture->GetSize().x * 0.5;
        valSFXTexture->Draw(Math::TranslationMatrix(Math::vec2{ centerX - scaledWidth / 2.0, centerY - 230.0 }) * Math::ScaleMatrix(Math::vec2{ 0.8, 0.8 }), CS200::YELLOW);
    }
}

void MainMenu::DrawImGui()
{
    ImGui::Begin("Main Menu Debug");

    ImGui::Text("Menu State: %s", currentState == MenuState::Main ? "Main" : "Settings");

    if (currentState == MenuState::Settings)
    {
        ImGui::Separator();
        const char* activeTabName = "Display";
        if (currentTab == SettingsTab::Controls)
            activeTabName = "Controls";
        else if (currentTab == SettingsTab::Sound)
            activeTabName = "Sound";

        ImGui::Text("Active Tab: %s", activeTabName);

        if (currentTab == SettingsTab::Display)
        {
            const char* resolutionNames[] = { "3840x2160", "2560x1440", "1920x1080", "1600x900", "1280x720" };
            const char* frameLimitNames[] = { "30 FPS", "60 FPS", "120 FPS", "144 FPS", "Unlimited" };
            ImGui::Text("Selected Resolution: %s", resolutionNames[pendingResIndex]);
            const char* modes[] = { "Windowed", "Borderless", "Fullscreen" };
            ImGui::Text("Selected Mode: %s", modes[static_cast<int>(pendingMode)]);
            ImGui::Text("Selected Frame Limit: %s", frameLimitNames[pendingFrameLimitIndex]);
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

    // Visual debugging of UI hitboxes
    ImGui::Checkbox("Show UI Rects", &showDebugRects);

    ImGui::End();
}

void MainMenu::Unload()
{
    Engine::GetLogger().LogEvent("Main Menu Unloaded");

    // Explicitly release texture memory
    titleTexture.reset();
    startTexture.reset();
    settingsTexture.reset();
    exitTexture.reset();
    tabDisplayTexture.reset();
    tabControlsTexture.reset();
    tabSoundTexture.reset();
    backTexture.reset();
    waitingTexture.reset();
    res3840Texture.reset();
    res2560Texture.reset();
    res1920Texture.reset();
    res1600Texture.reset();
    res1280Texture.reset();
    windowedTexture.reset();
    borderlessTexture.reset();
    fullscreenTexture.reset();
    fps30Texture.reset();
    fps60Texture.reset();
    fps120Texture.reset();
    fps144Texture.reset();
    fpsUnlimitedTexture.reset();
    applyTexture.reset();
    labelResolution.reset();
    labelMode.reset();
    labelFrameLimit.reset();
    labelBGM.reset();
    labelSFX.reset();
    valBGMTexture.reset();
    valSFXTexture.reset();

    keyBindButtons.clear();
}
