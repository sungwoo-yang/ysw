#pragma once
#include "Engine/GameState.hpp"
#include "Engine/Rect.hpp"
#include "Engine/Texture.hpp"
#include <memory>

class MainMenu : public CS230::GameState
{
public:
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
    MenuState currentState = MenuState::Main;

    enum class WindowMode
    {
        Windowed,
        Borderless,
        Fullscreen
    };

    std::shared_ptr<CS230::Texture> titleTexture;
    std::shared_ptr<CS230::Texture> startTexture;
    std::shared_ptr<CS230::Texture> settingsTexture;
    std::shared_ptr<CS230::Texture> exitTexture;

    // settings
    std::shared_ptr<CS230::Texture> res1600Texture;
    std::shared_ptr<CS230::Texture> res1280Texture;

    std::shared_ptr<CS230::Texture> windowedTexture;
    std::shared_ptr<CS230::Texture> borderlessTexture;
    std::shared_ptr<CS230::Texture> fullscreenTexture;

    std::shared_ptr<CS230::Texture> defaultTexture;
    std::shared_ptr<CS230::Texture> applyTexture;
    std::shared_ptr<CS230::Texture> backTexture;

    std::shared_ptr<CS230::Texture> labelResolution;
    std::shared_ptr<CS230::Texture> labelMode;

    Math::rect startButtonRect;
    Math::rect settingsButtonRect;
    Math::rect exitButtonRect;

    Math::rect res1600Rect;
    Math::rect res1280Rect;

    Math::rect windowedRect;
    Math::rect borderlessRect;
    Math::rect fullscreenRect;

    Math::rect defaultRect;
    Math::rect applyRect;
    Math::rect backRect;

    bool isStartHovered    = false;
    bool isSettingsHovered = false;
    bool isExitHovered     = false;

    int        pendingResIndex = 1;
    WindowMode pendingMode     = WindowMode::Windowed;

    const std::vector<Math::ivec2> resolutions = {
        { 1600, 900 },
        { 1280, 720 }
    };

    void SetupButtons();
    void DrawMainMenu();
    void DrawSettingsMenu();
    void UpdateMainMenu(double dt);
    void UpdateSettingsMenu(double dt);
};