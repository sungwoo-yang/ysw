#pragma once
#include "Window.hpp"
#include <string>
#include <filesystem>

namespace CS230
{
    struct GameSettings
    {
        int         resolutionX = 1280;
        int         resolutionY = 720;
        bool        fullscreen  = false;
        bool        borderless  = false;
        float       masterVolume = 1.0f;
        float       bgmVolume    = 1.0f;
        float       sfxVolume    = 1.0f;
        bool        showFPS      = false;
        std::string language     = "English";
    };

    class SettingsManager
    {
    public:
        static SettingsManager& Instance();

        void LoadSettings(const std::filesystem::path& filepath = "settings.cfg");
        void SaveSettings(const std::filesystem::path& filepath = "settings.cfg");

        const GameSettings& GetSettings() const;
        int   GetResolutionWidth() const;
        int   GetResolutionHeight() const;
        float GetMasterVolume() const;

        void SetResolution(int width, int height);
        void SetWindowMode(bool fullscreen, bool borderless);
        void SetMasterVolume(float volume);
        void SetShowFPS(bool show);

        void ApplyAllSettings();

    private:
        SettingsManager();
        ~SettingsManager() = default;

        GameSettings currentSettings;
        std::filesystem::path configPath;
    };
}