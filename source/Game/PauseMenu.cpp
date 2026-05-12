#include "PauseMenu.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "CS200/RenderingAPI.hpp"
#include "Engine/AudioManager.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/SettingsManager.hpp"
#include "Engine/Window.hpp"
#include "MainMenu.hpp"
#include <algorithm>
#include <imgui.h>
#include <initializer_list>

namespace
{
    Math::rect MakeRect(Math::vec2 bottomLeft, Math::ivec2 size)
    {
        return {
            bottomLeft,
            { bottomLeft.x + static_cast<double>(size.x), bottomLeft.y + static_cast<double>(size.y) }
        };
    }

    Math::rect MakeSizedRect(Math::vec2 bottomLeft, Math::vec2 size)
    {
        return {
            bottomLeft,
            { bottomLeft.x + size.x, bottomLeft.y + size.y }
        };
    }

    Math::rect CenteredRect(double centerX, double bottomY, Math::ivec2 size)
    {
        return MakeRect({ centerX - static_cast<double>(size.x) * 0.5, bottomY }, size);
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

    int VolumeToLevel(float volume)
    {
        return std::clamp(static_cast<int>(volume * 10.0f), 0, 10);
    }
}

void PauseMenu::Load()
{
    CS230::Font& titleFont = Engine::GetFont(0);
    CS230::Font& menuFont  = Engine::GetFont(1);

    titleTexture    = titleFont.PrintToTexture("Paused", CS200::CYAN);
    settingsTexture = menuFont.PrintToTexture("Settings", CS200::WHITE);
    mainMenuTexture = menuFont.PrintToTexture("Main Menu", CS200::WHITE);
    exitTexture     = menuFont.PrintToTexture("Exit Game", CS200::WHITE);
    backTexture     = menuFont.PrintToTexture("Back", CS200::WHITE);

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
    labelBGM            = menuFont.PrintToTexture("BGM Volume", CS200::GREY);
    labelSFX            = menuFont.PrintToTexture("SFX Volume", CS200::GREY);

    SyncPendingSettings();
    UpdateVolumeTextures();
    SetupButtons();
}

void PauseMenu::SyncPendingSettings()
{
    const auto& settings = CS230::SettingsManager::Instance().GetSettings();

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

    bgmLevel = VolumeToLevel(settings.bgmVolume);
    sfxLevel = VolumeToLevel(settings.sfxVolume);
}

void PauseMenu::SetupButtons()
{
    const Math::ivec2 winSize = Engine::GetWindow().GetSize();
    lastLayoutWindowSize     = winSize;

    const double centerX = static_cast<double>(winSize.x) * 0.5;
    const double centerY = static_cast<double>(winSize.y) * 0.5;

    const double homeY = centerY + 40.0;
    const double gap   = 70.0;

    if (settingsTexture)
        settingsRect = CenteredRect(centerX, homeY, settingsTexture->GetSize());
    if (mainMenuTexture)
        mainMenuRect = CenteredRect(centerX, homeY - gap, mainMenuTexture->GetSize());
    if (exitTexture)
        exitRect = CenteredRect(centerX, homeY - gap * 2.0, exitTexture->GetSize());
    if (backTexture)
        backRect = MakeRect({ 30.0, 30.0 }, backTexture->GetSize());

    const double     optionY                = centerY + 135.0;
    const Math::vec2 resolutionDropdownSize = DropdownSize({ res3840Texture, res2560Texture, res1920Texture, res1600Texture, res1280Texture });
    const Math::vec2 modeDropdownSize       = DropdownSize({ windowedTexture, borderlessTexture, fullscreenTexture });
    const Math::vec2 frameDropdownSize      = DropdownSize({ fps30Texture, fps60Texture, fps120Texture, fps144Texture, fpsUnlimitedTexture });
    const double     resolutionGap          = resolutionDropdownSize.y + 4.0;
    const double     modeGap                = modeDropdownSize.y + 4.0;
    const double     frameGap               = frameDropdownSize.y + 4.0;
    const double     resolutionX            = centerX - 470.0;
    const double     modeX                  = centerX - 120.0;
    const double     frameX                 = centerX + 230.0;
    const double volumeCenter = centerX;

    resolutionDropdownRect = MakeSizedRect({ resolutionX, optionY }, resolutionDropdownSize);
    modeDropdownRect       = MakeSizedRect({ modeX, optionY }, modeDropdownSize);
    frameLimitDropdownRect = MakeSizedRect({ frameX, optionY }, frameDropdownSize);

    res3840Rect = MakeSizedRect({ resolutionX, optionY - resolutionGap }, resolutionDropdownSize);
    res2560Rect = MakeSizedRect({ resolutionX, optionY - resolutionGap * 2.0 }, resolutionDropdownSize);
    res1920Rect = MakeSizedRect({ resolutionX, optionY - resolutionGap * 3.0 }, resolutionDropdownSize);
    res1600Rect = MakeSizedRect({ resolutionX, optionY - resolutionGap * 4.0 }, resolutionDropdownSize);
    res1280Rect = MakeSizedRect({ resolutionX, optionY - resolutionGap * 5.0 }, resolutionDropdownSize);

    windowedRect   = MakeSizedRect({ modeX, optionY - modeGap }, modeDropdownSize);
    borderlessRect = MakeSizedRect({ modeX, optionY - modeGap * 2.0 }, modeDropdownSize);
    fullscreenRect = MakeSizedRect({ modeX, optionY - modeGap * 3.0 }, modeDropdownSize);

    fps30Rect        = MakeSizedRect({ frameX, optionY - frameGap }, frameDropdownSize);
    fps60Rect        = MakeSizedRect({ frameX, optionY - frameGap * 2.0 }, frameDropdownSize);
    fps120Rect       = MakeSizedRect({ frameX, optionY - frameGap * 3.0 }, frameDropdownSize);
    fps144Rect       = MakeSizedRect({ frameX, optionY - frameGap * 4.0 }, frameDropdownSize);
    fpsUnlimitedRect = MakeSizedRect({ frameX, optionY - frameGap * 5.0 }, frameDropdownSize);
    if (applyTexture)
        applyRect = CenteredRect(centerX, 70.0, applyTexture->GetSize());

    const double bgmBarY    = centerY - 90.0;
    const double sfxBarY    = centerY - 210.0;
    const double blockGap   = 48.0;
    const double totalWidth = 10.0 * blockGap;
    const double startX     = volumeCenter - (totalWidth * 0.5) + (blockGap * 0.5);

    for (int i = 0; i < 10; ++i)
    {
        bgmBarRects[i] = {
            { startX + i * blockGap - 18.0,        bgmBarY },
            { startX + i * blockGap + 18.0, bgmBarY + 26.0 }
        };
        sfxBarRects[i] = {
            { startX + i * blockGap - 18.0,        sfxBarY },
            { startX + i * blockGap + 18.0, sfxBarY + 26.0 }
        };
    }
}

void PauseMenu::Update([[maybe_unused]] double dt)
{
    if (Engine::GetWindow().GetSize() != lastLayoutWindowSize)
    {
        SetupButtons();
    }

    auto& input = Engine::GetInput();
    if (input.KeyJustPressed(CS230::Input::Keys::Escape))
    {
        CS230::SettingsManager::Instance().SaveSettings();
        Engine::GetGameStateManager().PopState();
        return;
    }

    if (page == Page::Home)
        UpdateHome();
    else
        UpdateSettings();
}

void PauseMenu::UpdateHome()
{
    if (CheckClick(settingsRect))
    {
        page = Page::Settings;
        resolutionDropdownOpen = false;
        modeDropdownOpen       = false;
        frameLimitDropdownOpen = false;
        SetupButtons();
    }
    else if (CheckClick(mainMenuRect))
    {
        CS230::SettingsManager::Instance().SaveSettings();
        Engine::GetGameStateManager().ChangeStateWithFade<MainMenu>();
    }
    else if (CheckClick(exitRect))
    {
        CS230::SettingsManager::Instance().SaveSettings();
        Engine::Instance().Stop();
    }
}

void PauseMenu::UpdateSettings()
{
    if (CheckClick(backRect))
    {
        page = Page::Home;
        resolutionDropdownOpen = false;
        modeDropdownOpen       = false;
        frameLimitDropdownOpen = false;
        SetupButtons();
        return;
    }

    auto& input = Engine::GetInput();
    if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        const Math::ivec2 winSize  = Engine::GetWindow().GetSize();
        Math::vec2        mousePos = input.GetMousePosition();
        mousePos.y                 = static_cast<double>(winSize.y) - mousePos.y;

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
    }

    bool volumeChanged = false;
    for (int i = 0; i < 10; ++i)
    {
        if (CheckClick(bgmBarRects[i]))
        {
            bgmLevel      = (i == 0 && bgmLevel == 1) ? 0 : i + 1;
            volumeChanged = true;
        }
        if (CheckClick(sfxBarRects[i]))
        {
            sfxLevel      = (i == 0 && sfxLevel == 1) ? 0 : i + 1;
            volumeChanged = true;
        }
    }

    if (volumeChanged)
    {
        UpdateVolumeTextures();

        const float bgmFloat = static_cast<float>(bgmLevel) / 10.0f;
        const float sfxFloat = static_cast<float>(sfxLevel) / 10.0f;
        CS230::SettingsManager::Instance().SetBGMVolume(bgmFloat);
        CS230::SettingsManager::Instance().SetSFXVolume(sfxFloat);
        AudioManager::SetBGMVolume(static_cast<int>(bgmFloat * 128.0f));
        AudioManager::SetSFXVolume(static_cast<int>(sfxFloat * 128.0f));
    }

    if (CheckClick(applyRect))
    {
        auto& settingsMgr = CS230::SettingsManager::Instance();
        settingsMgr.SetWindowMode(false, pendingMode == WindowMode::Borderless);
        settingsMgr.SetResolution(resolutions[pendingResIndex].x, resolutions[pendingResIndex].y);
        settingsMgr.SetWindowMode(pendingMode == WindowMode::Fullscreen, pendingMode == WindowMode::Borderless);
        settingsMgr.SetFrameLimit(frameLimits[pendingFrameLimitIndex]);
        settingsMgr.SaveSettings();
        resolutionDropdownOpen = false;
        modeDropdownOpen       = false;
        frameLimitDropdownOpen = false;
        SetupButtons();
    }
}

bool PauseMenu::CheckClick(const Math::rect& rect) const
{
    auto& input = Engine::GetInput();
    if (!input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        return false;
    }

    const Math::ivec2 winSize = Engine::GetWindow().GetSize();
    Math::vec2       mousePos = input.GetMousePosition();
    mousePos.y                = static_cast<double>(winSize.y) - mousePos.y;

    return mousePos.x >= rect.Left() && mousePos.x <= rect.Right() && mousePos.y >= rect.Bottom() && mousePos.y <= rect.Top();
}

void PauseMenu::Draw()
{
    auto&              renderer = Engine::GetRenderer2D();
    const Math::ivec2  winSize  = Engine::GetWindow().GetSize();
    const double       width    = static_cast<double>(winSize.x);
    const double       height   = static_cast<double>(winSize.y);
    const Math::vec2   center   = { width * 0.5, height * 0.5 };
    const auto         screen   = CS200::build_ndc_matrix(winSize);

    CS200::RenderingAPI::SetViewport(winSize);
    renderer.BeginScene(screen);

    const auto overlay = Math::TranslationMatrix(center) * Math::ScaleMatrix({ width, height });
    renderer.DrawRectangle(overlay, 0x000000B8, CS200::CLEAR, 0.0);

    if (page == Page::Home)
        DrawHome();
    else
        DrawSettings();

    renderer.EndScene();
}

void PauseMenu::DrawHome()
{
    const Math::ivec2 winSize = Engine::GetWindow().GetSize();
    const double      centerX = static_cast<double>(winSize.x) * 0.5;
    const double      centerY = static_cast<double>(winSize.y) * 0.5;

    if (titleTexture)
    {
        const Math::ivec2 size = titleTexture->GetSize();
        titleTexture->Draw(Math::TranslationMatrix(Math::vec2{ centerX - size.x * 1.5, centerY + 140.0 }) * Math::ScaleMatrix({ 3.0, 3.0 }), CS200::CYAN);
    }

    DrawButton(settingsTexture, settingsRect);
    DrawButton(mainMenuTexture, mainMenuRect);
    DrawButton(exitTexture, exitRect);
}

void PauseMenu::DrawSettings()
{
    auto&             renderer = Engine::GetRenderer2D();
    const Math::ivec2 winSize  = Engine::GetWindow().GetSize();
    const double      centerX  = static_cast<double>(winSize.x) * 0.5;
    const double      centerY  = static_cast<double>(winSize.y) * 0.5;

    DrawButton(backTexture, backRect);

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

    if (labelBGM)
        labelBGM->Draw(Math::TranslationMatrix(Math::vec2{ centerX - labelBGM->GetSize().x * 0.5, centerY - 50.0 }), CS200::WHITE);
    for (int i = 0; i < 10; ++i)
    {
        const Math::vec2 size   = bgmBarRects[i].Size();
        const Math::vec2 center = { bgmBarRects[i].Left() + size.x * 0.5, bgmBarRects[i].Bottom() + size.y * 0.5 };
        const auto       rect   = Math::TranslationMatrix(center) * Math::ScaleMatrix(size);
        renderer.DrawRectangle(rect, i < bgmLevel ? CS200::GREEN : CS200::CLEAR, i < bgmLevel ? CS200::WHITE : CS200::GREY, 2.0);
    }
    if (valBGMTexture)
        valBGMTexture->Draw(Math::TranslationMatrix(Math::vec2{ centerX + 270.0, centerY - 93.0 }) * Math::ScaleMatrix({ 0.8, 0.8 }), CS200::YELLOW);

    if (labelSFX)
        labelSFX->Draw(Math::TranslationMatrix(Math::vec2{ centerX - labelSFX->GetSize().x * 0.5, centerY - 170.0 }), CS200::WHITE);
    for (int i = 0; i < 10; ++i)
    {
        const Math::vec2 size   = sfxBarRects[i].Size();
        const Math::vec2 center = { sfxBarRects[i].Left() + size.x * 0.5, sfxBarRects[i].Bottom() + size.y * 0.5 };
        const auto       rect   = Math::TranslationMatrix(center) * Math::ScaleMatrix(size);
        renderer.DrawRectangle(rect, i < sfxLevel ? CS200::GREEN : CS200::CLEAR, i < sfxLevel ? CS200::WHITE : CS200::GREY, 2.0);
    }
    if (valSFXTexture)
        valSFXTexture->Draw(Math::TranslationMatrix(Math::vec2{ centerX + 270.0, centerY - 213.0 }) * Math::ScaleMatrix({ 0.8, 0.8 }), CS200::YELLOW);

    DrawButton(applyTexture, applyRect);

    DrawDropdown(labelResolution, resolutionDropdownRect, resolutionTextures[pendingResIndex], resolutionTextures, resolutionRects, ResolutionOptionCount, pendingResIndex, resolutionDropdownOpen);
    DrawDropdown(labelMode, modeDropdownRect, modeTextures[static_cast<int>(pendingMode)], modeTextures, modeRects, WindowModeOptionCount, static_cast<int>(pendingMode), modeDropdownOpen);
    DrawDropdown(labelFrameLimit, frameLimitDropdownRect, frameTextures[pendingFrameLimitIndex], frameTextures, frameRects, FrameLimitOptionCount, pendingFrameLimitIndex,
                 frameLimitDropdownOpen);
}

void PauseMenu::DrawText(std::shared_ptr<CS230::Texture> texture, Math::rect rect, CS200::RGBA color)
{
    if (texture)
    {
        texture->Draw(Math::TranslationMatrix(Math::vec2{ rect.Left(), rect.Bottom() }), color);
    }
}

void PauseMenu::DrawButton(std::shared_ptr<CS230::Texture> texture, Math::rect rect, bool selected)
{
    if (texture)
    {
        texture->Draw(Math::TranslationMatrix(Math::vec2{ rect.Left(), rect.Bottom() }), selected ? CS200::CYAN : CS200::WHITE);
    }
}

void PauseMenu::UpdateVolumeTextures()
{
    CS230::Font& menuFont = Engine::GetFont(1);
    valBGMTexture         = menuFont.PrintToTexture(std::to_string(bgmLevel), CS200::WHITE);
    valSFXTexture         = menuFont.PrintToTexture(std::to_string(sfxLevel), CS200::WHITE);
}

void PauseMenu::DrawImGui()
{
#ifdef DEVELOPER_VERSION
    ImGui::Begin("Pause Menu Debug");
    ImGui::Text("Page: %s", page == Page::Home ? "Home" : "Settings");
    ImGui::End();
#endif
}

void PauseMenu::Unload()
{
    CS230::SettingsManager::Instance().SaveSettings();
}
