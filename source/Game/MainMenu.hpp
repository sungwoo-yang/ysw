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

    gsl::czstring GetName() const override
    {
        return "MainMenu";
    }

private:
    enum class MenuState
    {
        Main,
        Settings
    };

    enum class SettingsTab
    {
        Display,
        Controls
    };

    void UpdateMainMenu(double dt);
    void DrawMainMenu();

    std::shared_ptr<CS230::Texture> titleTexture;
    std::shared_ptr<CS230::Texture> startTexture;
    std::shared_ptr<CS230::Texture> settingsTexture;
    std::shared_ptr<CS230::Texture> exitTexture;

    Math::rect startButtonRect;
    Math::rect settingsButtonRect;
    Math::rect exitButtonRect;

    bool isStartHovered    = false;
    bool isSettingsHovered = false;
    bool isExitHovered     = false;

    void UpdateSettingsMenu(double dt);
    void DrawSettingsMenu();
    void SetupButtons();

    std::shared_ptr<CS230::Texture> tabDisplayTexture;
    std::shared_ptr<CS230::Texture> tabControlsTexture;
    std::shared_ptr<CS230::Texture> backTexture;

    Math::rect tabDisplayRect;
    Math::rect tabControlsRect;
    Math::rect backRect;

    void UpdateDisplayTab();
    void DrawDisplayTab();

    std::shared_ptr<CS230::Texture> res1600Texture;
    std::shared_ptr<CS230::Texture> res1280Texture;
    std::shared_ptr<CS230::Texture> windowedTexture;
    std::shared_ptr<CS230::Texture> borderlessTexture;
    std::shared_ptr<CS230::Texture> fullscreenTexture;
    std::shared_ptr<CS230::Texture> applyTexture;
    std::shared_ptr<CS230::Texture> labelResolution;
    std::shared_ptr<CS230::Texture> labelMode;

    Math::rect res1600Rect;
    Math::rect res1280Rect;
    Math::rect windowedRect;
    Math::rect borderlessRect;
    Math::rect fullscreenRect;
    Math::rect applyRect;

    enum class WindowMode
    {
        Windowed,
        Borderless,
        Fullscreen
    };
    int               pendingResIndex = 1;
    WindowMode        pendingMode     = WindowMode::Windowed;
    const Math::ivec2 resolutions[2]  = {
        { 1600, 900 },
        { 1280, 720 }
    };

    void UpdateControlsTab();
    void DrawControlsTab();

    struct KeyBindButton
    {
        CS230::GameAction               action;
        Math::rect                      rect;
        std::string                     actionName;
        std::shared_ptr<CS230::Texture> nameTexture;
        std::shared_ptr<CS230::Texture> keyTexture;
    };

    std::vector<KeyBindButton> keyBindButtons;

    CS230::GameAction               rebindAction    = CS230::GameAction::Count;
    bool                            isWaitingForKey = false;
    std::shared_ptr<CS230::Texture> waitingTexture;

    MenuState   currentState = MenuState::Main;
    SettingsTab currentTab   = SettingsTab::Display;
};