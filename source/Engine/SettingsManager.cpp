#include "SettingsManager.hpp"
#include "Engine.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace CS230
{
    SettingsManager& SettingsManager::Instance()
    {
        static SettingsManager instance;
        return instance;
    }

    SettingsManager::SettingsManager()
    {
        currentSettings = GameSettings{};
    }

    const GameSettings& SettingsManager::GetSettings() const
    {
        return currentSettings;
    }

    int SettingsManager::GetResolutionWidth() const
    {
        return currentSettings.resolutionX;
    }

    int SettingsManager::GetResolutionHeight() const
    {
        return currentSettings.resolutionY;
    }

    float SettingsManager::GetMasterVolume() const
    {
        return currentSettings.masterVolume;
    }

    void SettingsManager::SetResolution(int width, int height)
    {
        currentSettings.resolutionX = width;
        currentSettings.resolutionY = height;
        Engine::GetWindow().ForceResize(width, height);
    }

    void SettingsManager::SetWindowMode(bool fullscreen, bool borderless)
    {
        currentSettings.fullscreen = fullscreen;
        currentSettings.borderless = borderless;

        auto& window = Engine::GetWindow();
        window.SetFullscreen(fullscreen);
        window.SetBordered(!borderless);
    }

    void SettingsManager::SetMasterVolume(float volume)
    {
        currentSettings.masterVolume = std::clamp(volume, 0.0f, 1.0f);
    }

    void SettingsManager::SetShowFPS(bool show)
    {
        currentSettings.showFPS = show;
    }

    void SettingsManager::ApplyAllSettings()
    {
        auto& window = Engine::GetWindow();
        window.SetFullscreen(false);
        window.ForceResize(currentSettings.resolutionX, currentSettings.resolutionY);
        window.SetBordered(!currentSettings.borderless);
        if (currentSettings.fullscreen)
        {
            window.SetFullscreen(true);
        }
        Engine::GetLogger().LogEvent("Settings Applied Successfully.");
    }

    void SettingsManager::LoadSettings(const std::filesystem::path& filepath)
    {
        configPath = filepath;
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            Engine::GetLogger().LogEvent("Config file not found. Using defaults.");
            return;
        }

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string        key;
            if (std::getline(iss, key, '='))
            {
                std::string value;
                if (std::getline(iss, value))
                {
                    if (key == "ResolutionX")
                        currentSettings.resolutionX = std::stoi(value);
                    else if (key == "ResolutionY")
                        currentSettings.resolutionY = std::stoi(value);
                    else if (key == "Fullscreen")
                        currentSettings.fullscreen = (value == "1" || value == "true");
                    else if (key == "Borderless")
                        currentSettings.borderless = (value == "1" || value == "true");
                    else if (key == "MasterVolume")
                        currentSettings.masterVolume = std::stof(value);
                    else if (key == "ShowFPS")
                        currentSettings.showFPS = (value == "1" || value == "true");
                }
            }
        }
        Engine::GetLogger().LogEvent("Settings Loaded.");
        ApplyAllSettings();
    }

    void SettingsManager::SaveSettings(const std::filesystem::path& filepath)
    {
        std::ofstream file(filepath);
        if (!file.is_open())
            return;

        file << "# Game Configuration File\n";
        file << "ResolutionX=" << currentSettings.resolutionX << "\n";
        file << "ResolutionY=" << currentSettings.resolutionY << "\n";
        file << "Fullscreen=" << (currentSettings.fullscreen ? "1" : "0") << "\n";
        file << "Borderless=" << (currentSettings.borderless ? "1" : "0") << "\n";
        file << "MasterVolume=" << currentSettings.masterVolume << "\n";
        file << "ShowFPS=" << (currentSettings.showFPS ? "1" : "0") << "\n";

        Engine::GetLogger().LogEvent("Settings Saved to " + filepath.string());
    }
}