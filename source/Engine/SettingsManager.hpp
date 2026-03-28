#pragma once
#include "Window.hpp"
#include <filesystem>
#include <string>

namespace CS230
{
    // Game Settings Data
    struct GameSettings
    {
        int         resolutionX  = 1280;
        int         resolutionY  = 720;
        bool        fullscreen   = false;
        bool        borderless   = false;
        float       masterVolume = 1.0f;
        float       bgmVolume    = 1.0f;
        float       sfxVolume    = 1.0f;
        bool        showFPS      = false;
        std::string language     = "English";
    };

    class SettingsManager
    {
    public:
        // Get Singleton Instance
        static SettingsManager& Instance();

        // Load/Save configuration
        void LoadSettings(const std::filesystem::path& filepath = "settings.cfg");
        void SaveSettings(const std::filesystem::path& filepath = "settings.cfg");

        // Getters
        [[nodiscard]] const GameSettings& GetSettings() const;
        [[nodiscard]] int                 GetResolutionWidth() const;
        [[nodiscard]] int                 GetResolutionHeight() const;
        [[nodiscard]] float               GetMasterVolume() const;
        [[nodiscard]] float               GetBGMVolume() const;
        [[nodiscard]] float               GetSFXVolume() const;

        // Setters
        void SetResolution(int width, int height);
        void SetWindowMode(bool fullscreen, bool borderless);
        void SetMasterVolume(float volume);
        void SetBGMVolume(float volume);
        void SetSFXVolume(float volume);
        void SetShowFPS(bool show);

        // Apply all settings to engine
        void ApplyAllSettings();

    private:
        // Optimization: Use default constructor
        SettingsManager();
        ~SettingsManager() = default;

        GameSettings          currentSettings{};
        std::filesystem::path configPath;
    };
}