#include "AudioManager.hpp"
#include <SDL2/SDL_mixer.h>
#include <iostream>

void AudioManager::Initialize()
{
    // Initialize with 44100Hz, default format, stereo (2 channels), and 2048 byte chunk size
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        std::cerr << "[AudioManager Error] SDL_mixer could not initialize! Error: " << Mix_GetError() << std::endl;
    }
}

void AudioManager::Shutdown()
{
    // Free all loaded audio resources
    for (auto& pair : audioMap)
    {
        delete pair.second;
    }
    audioMap.clear();

    Mix_CloseAudio();
}

void AudioManager::LoadSound(const std::string& name, const std::string& filePath, AudioTypes audioType)
{
    // Prevent loading the same sound multiple times
    if (audioMap.find(name) != audioMap.end())
        return;

    Audio* newAudio = new Audio(filePath, audioType);
    audioMap[name]  = newAudio;
}

void AudioManager::Play(const std::string& name)
{
    auto it = audioMap.find(name);
    if (it == audioMap.end())
    {
        std::cerr << "[AudioManager Error] Sound not found: " << name << std::endl;
        return;
    }

    Audio* audio = it->second;

    if (audio->GetType() == AudioTypes::SFX)
    {
        // Play the sound effect once (0 loops) on the first available channel (-1)
        Mix_PlayChannel(-1, audio->GetSFX(), 0);
    }
    else
    {
        // Play the background music in an infinite loop (-1)
        Mix_PlayMusic(audio->GetBGM(), -1);
    }
}

void AudioManager::StopBGM()
{
    Mix_HaltMusic();
}

void AudioManager::SetVolume(int volume)
{
    // Volume ranges from 0 to MIX_MAX_VOLUME (normally 128)
    Mix_Volume(-1, volume);  // Set volume for all SFX channels
    Mix_VolumeMusic(volume); // Set volume for BGM
}