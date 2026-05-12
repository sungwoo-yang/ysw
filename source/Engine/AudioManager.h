// #pragma once

// #include <SDL_mixer.h>
// #include <string>
// #include <unordered_map>
// #include <filesystem>
// #include <memory>

// namespace CS230
// {
//     struct SoundDeleter { void operator()(Mix_Chunk* c) const { Mix_FreeChunk(c); } };
//     struct MusicDeleter { void operator()(Mix_Music* m) const { Mix_FreeMusic(m); } };

//     class AudioManager
//     {
//     public:
//         AudioManager();
//         ~AudioManager();

//         void Init();

//         void Shutdown();


//         Mix_Chunk* LoadSound(const std::filesystem::path& file_name);

//         Mix_Music* LoadMusic(const std::filesystem::path& file_name);

        
//         int PlaySound(Mix_Chunk* sound, int loops = 0);


//         int PlayMusic(Mix_Music* music, int loops = -1);

  

//         void Unload();

//     private:

//         std::unordered_map<std::string, std::unique_ptr<Mix_Chunk, SoundDeleter>> sound_cache;
//         std::unordered_map<std::string, std::unique_ptr<Mix_Music, MusicDeleter>> music_cache;
//     };
// }