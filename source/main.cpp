/**
 * \file
 * \author Rudy Castan
 * \date 2024 Fall
 * \copyright DigiPen Institute of Technology
 */

#include "Engine/Engine.hpp"
#include "Engine/Error.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Window.hpp"
#include "Game/MainMenu.hpp"
#include "Game/Mode1.hpp"
#include "Game/Boss1.hpp"
#include "Engine/SettingsManager.hpp"
#include "Game/LevelEditor.hpp"
#include "Game/OriMode.hpp"
#include "Game/PauseMenu.hpp"
#include "Game/Splash.hpp"
#include <iostream>

namespace
{
    [[maybe_unused]] int  gWindowWidth  = 400;
    [[maybe_unused]] int  gWindowHeight = 400;
    [[maybe_unused]] bool gNeedResize   = false;
}

#if defined(__EMSCRIPTEN__)
#    include <emscripten.h>
#    include <emscripten/bind.h>
#    include <emscripten/em_asm.h>

void main_loop()
{
    Engine& engine = Engine::Instance();
    if (gNeedResize)
    {
        Engine::GetWindow().ForceResize(gWindowWidth, gWindowHeight);
        gNeedResize = false;
    }

    engine.Update();

    if (engine.HasGameEnded())
    {
        emscripten_cancel_main_loop();
        engine.Stop();
    }
}

EMSCRIPTEN_BINDINGS(main_window)
{
    emscripten::function(
        "setWindowSize", emscripten::optional_override(
                             [](int sizeX, int sizeY)
                             {
                                 sizeX                  = (sizeX < 400) ? 400 : sizeX;
                                 sizeY                  = (sizeY < 400) ? 400 : sizeY;
                                 const auto window_size = Engine::GetWindow().GetSize();
                                 if (sizeX != window_size.x || sizeY != window_size.y)
                                 {
                                     gNeedResize   = true;
                                     gWindowWidth  = sizeX;
                                     gWindowHeight = sizeY;
                                 }
                             }));
}
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    try
    {
        Engine& engine = Engine::Instance();
        engine.Start("OLLIM");

        engine.AddFont("Assets/fonts/Elara-Bold.png");
        engine.AddFont("Assets/fonts/Elara-Bold.png"); // index 1 kept for any GetFont(1) callers

        CS230::SettingsManager::Instance().LoadSettings();  // applies volume from settings.cfg

        engine.GetGameStateManager().SetPauseState<PauseMenu>();

#ifdef DEVELOPER_VERSION
        // Dev mode: jump straight into the Level Editor
        engine.GetGameStateManager().PushState<LevelEditor>();
#else
        // Release mode: start at the main menu
        engine.GetGameStateManager().PushState<MainMenu>();
#endif

#if !defined(__EMSCRIPTEN__)
        while (engine.HasGameEnded() == false)
        {
            engine.Update();
        }
        engine.Stop();
#else
        // https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop
        constexpr bool simulate_infinite_loop  = true;
        constexpr int  match_browser_framerate = -1;
        emscripten_set_main_loop(main_loop, match_browser_framerate, simulate_infinite_loop);
#endif
        return 0;
    }

    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return -1;
    }
}
