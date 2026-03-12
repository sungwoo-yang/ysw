#include "Audio.hpp"
#include "Path.hpp"
#include <iostream>

Audio::Audio(const std::filesystem::path& filePath, AudioTypes audioType) : filepath(filePath.string()), type(audioType), sfx(nullptr), bgm(nullptr)
{
    try
    {
        this->filepath = assets::locate_asset(filePath).string();
    }
    catch (const std::exception& e)
    {
        this->filepath = filePath.string();
    }

    if (type == AudioTypes::SFX)
    {
        // Load sound effect (support .wav, .ogg, etc.)
        sfx = Mix_LoadWAV(filepath.c_str());
        if (!sfx)
        {
            std::cerr << "[Audio Error] Failed to load SFX: " << filepath << " | SDL_mixer Error: " << Mix_GetError() << std::endl;
        }
    }
    else
    {
        // Load background music
        bgm = Mix_LoadMUS(filepath.c_str());
        if (!bgm)
        {
            std::cerr << "[Audio Error] Failed to load BGM: " << filepath << " | SDL_mixer Error: " << Mix_GetError() << std::endl;
        }
    }
}

Audio::~Audio()
{
    // Free audio memory to prevent memory leaks
    if (sfx)
    {
        Mix_FreeChunk(sfx);
        sfx = nullptr;
    }
    if (bgm)
    {
        Mix_FreeMusic(bgm);
        bgm = nullptr;
    }
}