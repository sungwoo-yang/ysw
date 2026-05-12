#pragma once

#include "CS200/RGBA.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Rect.hpp"
#include "Engine/Texture.hpp"
#include <memory>

class PauseMenu : public CS230::GameState
{
public:
    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    [[nodiscard]] gsl::czstring GetName() const override
    {
        return "PauseMenu";
    }

private:
    enum class Page
    {
        Home,
        Settings
    };

    enum class WindowMode
    {
        Windowed,
        Borderless,
        Fullscreen
    };

    void SetupButtons();
    void SyncPendingSettings();
    void UpdateHome();
    void UpdateSettings();
    void DrawHome();
    void DrawSettings();
    void UpdateVolumeTextures();

    bool CheckClick(const Math::rect& rect) const;
    void DrawText(std::shared_ptr<CS230::Texture> texture, Math::rect rect, CS200::RGBA color);
    void DrawButton(std::shared_ptr<CS230::Texture> texture, Math::rect rect, bool selected = false);

    Page page = Page::Home;

    std::shared_ptr<CS230::Texture> titleTexture;
    std::shared_ptr<CS230::Texture> settingsTexture;
    std::shared_ptr<CS230::Texture> mainMenuTexture;
    std::shared_ptr<CS230::Texture> exitTexture;
    std::shared_ptr<CS230::Texture> backTexture;

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
    std::shared_ptr<CS230::Texture> labelBGM;
    std::shared_ptr<CS230::Texture> labelSFX;
    std::shared_ptr<CS230::Texture> valBGMTexture;
    std::shared_ptr<CS230::Texture> valSFXTexture;

    Math::rect settingsRect;
    Math::rect mainMenuRect;
    Math::rect exitRect;
    Math::rect backRect;

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
    Math::rect bgmBarRects[10];
    Math::rect sfxBarRects[10];

    static constexpr int ResolutionOptionCount = 5;
    static constexpr int FrameLimitOptionCount = 5;
    static constexpr int WindowModeOptionCount = 3;

    int        pendingResIndex        = 4;
    int        pendingFrameLimitIndex = 1;
    WindowMode pendingMode            = WindowMode::Windowed;
    int        bgmLevel               = 10;
    int        sfxLevel               = 10;
    bool       resolutionDropdownOpen = false;
    bool       modeDropdownOpen       = false;
    bool       frameLimitDropdownOpen = false;

    const Math::ivec2 resolutions[ResolutionOptionCount] = {
        { 3840, 2160 },
        { 2560, 1440 },
        { 1920, 1080 },
        { 1600, 900  },
        { 1280, 720  }
    };
    const int frameLimits[FrameLimitOptionCount] = { 30, 60, 120, 144, 0 };

    Math::ivec2 lastLayoutWindowSize{ 0, 0 };
};
;