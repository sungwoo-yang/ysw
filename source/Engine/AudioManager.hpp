#pragma once
#include "Audio.hpp"
#include <string>
#include <unordered_map>

class AudioManager
{
public:
    static void Initialize();
    static void Shutdown();

    // Load and caches an audio file into the manager
    static void LoadSound(const std::string& name, const std::string& filePath, AudioTypes audioType);

    // Plays the loaded sound by its assigned name
    static void Play(const std::string& name);
    static void StopBGM();

    // Adjusts the volume for all sounds and music
    static void SetVolume(int volume);

private:
    inline static std::unordered_map<std::string, Audio*> audioMap;
};