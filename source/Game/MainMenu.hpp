#pragma once
#include "Engine/GameState.hpp"
#include "Engine/Input.hpp"
#include "Engine/InputMapper.hpp"
#include "Engine/Rect.hpp"
#include "Engine/Texture.hpp"
#include <memory>
#include <vector>

class MainMenu : public CS230::GameState
{
public:
    MainMenu() = default;
    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    [[nodiscard]] gsl::czstring GetName() const override
    {
        return "MainMenu";
    }

private:
    // UI states for switching between the title screen and settings
    enum class MenuState
    {
        Main,
        Settings
    };

    enum class SettingsTab
    {
        Display,
        Controls,
        Sound
    };

    void UpdateMainMenu(double dt);
    void DrawMainMenu();

    // Visual assets for menu text
    std::shared_ptr<CS230::Texture> titleTexture;
    std::shared_ptr<CS230::Texture> startTexture;
    std::shared_ptr<CS230::Texture> settingsTexture;
    std::shared_ptr<CS230::Texture> exitTexture;

    // Interactive button regions for mouse detection
    Math::rect startButtonRect;
    Math::rect settingsButtonRect;
    Math::rect exitButtonRect;

    bool isStartHovered    = false;
    bool isSettingsHovered = false;
    bool isExitHovered     = false;

    void UpdateSettingsMenu(double dt);
    void DrawSettingsMenu();
    void SetupButtons();
    void DrawDebugRect(const Math::rect& rect);
    void DrawDebugUIRects();

    // Tab textures and their screen areas
    std::shared_ptr<CS230::Texture> tabDisplayTexture;
    std::shared_ptr<CS230::Texture> tabControlsTexture;
    std::shared_ptr<CS230::Texture> tabSoundTexture;
    std::shared_ptr<CS230::Texture> backTexture;

    Math::rect tabDisplayRect;
    Math::rect tabControlsRect;
    Math::rect tabSoundRect;
    Math::rect backRect;

    void UpdateDisplayTab();
    void DrawDisplayTab();

    // Display-specific textures (Resolutions, Modes)
    std::shared_ptr<CS230::Texture> res3840Texture;
    std::shared_ptr<CS230::Texture> res2560Texture;
    std::shared_ptr<CS230::Texture> res1920Texture;
    std::shared_ptr<CS230::Texture> res1600Texture;
    std::shared_ptr<CS230::Texture> res1280Texture;
    std::shared_ptr<CS230::Texture> windowedTexture;
    std::shared_ptr<CS230::Texture> borderlessTexture;
    std::shared_ptr<CS230::Texture> fullscreenTexture;
    std::shared_ptr<CS230::Texture> fps30Texture;
    std::shared_ptr<CS230::Texture> fps60Texture;
    std::shared_ptr<CS230::Texture> fps120Texture;
    std::shared_ptr<CS230::Texture> fps144Texture;
    std::shared_ptr<CS230::Texture> fpsUnlimitedTexture;
    std::shared_ptr<CS230::Texture> applyTexture;
    std::shared_ptr<CS230::Texture> labelResolution;
    std::shared_ptr<CS230::Texture> labelMode;
    std::shared_ptr<CS230::Texture> labelFrameLimit;

    Math::rect res3840Rect;
    Math::rect res2560Rect;
    Math::rect res1920Rect;
    Math::rect res1600Rect;
    Math::rect res1280Rect;
    Math::rect resolutionDropdownRect;
    Math::rect modeDropdownRect;
    Math::rect frameLimitDropdownRect;
    Math::rect windowedRect;
    Math::rect borderlessRect;
    Math::rect fullscreenRect;
    Math::rect fps30Rect;
    Math::rect fps60Rect;
    Math::rect fps120Rect;
    Math::rect fps144Rect;
    Math::rect fpsUnlimitedRect;
    Math::rect applyRect;

    enum class WindowMode
    {
        Windowed,
        Borderless,
        Fullscreen
    };
    static constexpr int ResolutionOptionCount = 5;
    static constexpr int FrameLimitOptionCount = 5;
    static constexpr int WindowModeOptionCount = 3;

    int               pendingResIndex        = 4;
    int               pendingFrameLimitIndex = 1;
    WindowMode        pendingMode            = WindowMode::Windowed;
    bool              resolutionDropdownOpen = false;
    bool              modeDropdownOpen       = false;
    bool              frameLimitDropdownOpen = false;
    const Math::ivec2 resolutions[ResolutionOptionCount] = {
        { 3840, 2160 },
        { 2560, 1440 },
        { 1920, 1080 },
        { 1600, 900  },
        { 1280, 720  }
    };
    const int frameLimits[FrameLimitOptionCount] = { 30, 60, 120, 144, 0 };

    void UpdateControlsTab();
    void DrawControlsTab();

    // Sound Settings
    void UpdateSoundTab();
    void DrawSoundTab();
    void UpdateVolumeTextures();

    std::shared_ptr<CS230::Texture> labelBGM;
    std::shared_ptr<CS230::Texture> labelSFX;
    std::shared_ptr<CS230::Texture> valBGMTexture;
    std::shared_ptr<CS230::Texture> valSFXTexture;

    Math::rect bgmBarRects[10];
    Math::rect sfxBarRects[10];

    int bgmLevel = 10;
    int sfxLevel = 10;
    
    // Internal data structure for rendering the key binding list
    struct KeyBindButton
    {
        CS230::GameAction               action;
        Math::rect                      rect;
        std::string                     actionName;
        std::shared_ptr<CS230::Texture> nameTexture;
        std::shared_ptr<CS230::Texture> keyTexture;
    };

    std::vector<KeyBindButton> keyBindButtons;

    // State variables for the input remapping process
    CS230::GameAction               rebindAction    = CS230::GameAction::Count;
    bool                            isWaitingForKey = false;
    std::shared_ptr<CS230::Texture> waitingTexture;

    MenuState   currentState = MenuState::Main;
    SettingsTab currentTab   = SettingsTab::Display;
    bool        showDebugRects = false;
    Math::ivec2 lastLayoutWindowSize{ 0, 0 };
};
