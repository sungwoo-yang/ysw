/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Vec2.hpp"
#include <gsl/gsl>
#include <memory>
#include <string_view>
#include <filesystem>

namespace CS230
{
    class Logger;
    class Window;
    class Input;
    class GameState;
    class GameStateManager;
    class TextureManager;
    class Font;
}

namespace CS200
{
    class IRenderer2D;
}

struct WindowEnvironment
{
    int        FPS         = 0;
    uint64_t   FrameCount  = 0;
    double     DeltaTime   = 0.0;
    double     ElapsedTime = 0.0;
    Math::vec2 DisplaySize{};
};

class Engine
{
public:
    static Engine& Instance();
    static CS230::Logger& GetLogger();
    static CS230::Window& GetWindow();
    static CS230::Input& GetInput();
    static const WindowEnvironment& GetWindowEnvironment();
    static CS230::GameStateManager& GetGameStateManager();
    static CS200::IRenderer2D& GetRenderer2D();
    static CS230::TextureManager& GetTextureManager();
    static CS230::Font& GetFont(int index);
    void AddFont(const std::filesystem::path& file_name);

public:
    void Start(std::string_view window_title);
    void Stop();
    void Update();
    bool HasGameEnded();

private:
    class Impl;
    gsl::owner<Impl*> impl = nullptr;

private:
    Engine();
    ~Engine();

    void updateEnvironment();
};
