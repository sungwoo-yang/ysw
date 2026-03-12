#pragma once
#include "AudioTypes.hpp"
#include <SDL2/SDL_mixer.h>
#include <string>

class Audio
{
public:
    Audio(const std::string& filePath, AudioTypes audioType);
    ~Audio();

    AudioTypes GetType() const
    {
        return type;
    }

    Mix_Chunk* GetSFX() const
    {
        return sfx;
    }

    Mix_Music* GetBGM() const
    {
        return bgm;
    }

private:
    std::string filepath;
    AudioTypes  type;

    Mix_Chunk* sfx;
    Mix_Music* bgm;
};