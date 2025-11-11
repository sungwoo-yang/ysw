/**
 * \file
 * \author Rudy Castan
 * \date 2024 Fall
 * \copyright DigiPen Institute of Technology
 */


#include "ImGuiHelper.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <gsl/gsl>
#include <iostream>

gsl::owner<SDL_Window*>   gWindow  = nullptr;
gsl::owner<SDL_GLContext> gContext = nullptr;
bool                      gIsDone  = false;
int                       gWidth   = 800;
int                       gHeight  = 600;

#if defined(__EMSCRIPTEN__)
#    include <emscripten.h>
#    include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(main_window)
{
    emscripten::function("setWindowSize", emscripten::optional_override([](int sizeX, int sizeY) { SDL_SetWindowSize(gWindow, sizeX, sizeY); }));
}

#endif


void demo_setup();
void demo_draw();
void demo_imgui();
void demo_shutdown();

void setup()
{
    ImGuiHelper::Initialize(gWindow, gContext);
    demo_setup();
}

void main_loop();

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
#ifdef DEVELOPER_VERSION
    std::cout << "Developer Version\n";
#endif

#ifdef IS_WEBGL2
    std::cout << "WebGL 2 Version\n";
#else
    std::cout << "OpenGL Desktop Version\n";
#endif

    std::cout << "Hello World!\nRudy is here\n";


    SDL_Init(SDL_INIT_VIDEO);
#ifdef IS_WEBGL2
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    // double buffer, bits for color, depth, stencil, multisampling

    gWindow = SDL_CreateWindow("CS200 Fun", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    gContext = SDL_GL_CreateContext(gWindow);

    SDL_GL_MakeCurrent(gWindow, gContext);

    glewInit();

    // Vsync
    constexpr int ADAPTIVE_SYNC = -1;
    constexpr int VSYNC         = 1;
    if (const auto result = SDL_GL_SetSwapInterval(ADAPTIVE_SYNC); result != 0)
    {
        SDL_GL_SetSwapInterval(VSYNC);
    }

    setup();

#if !defined(__EMSCRIPTEN__)
    while (!gIsDone)
    {
        main_loop();
    }
#else
    const bool simulate_infinte_loop   = true;
    const int  match_browser_framerate = -1;
    emscripten_set_main_loop(main_loop, match_browser_framerate, simulate_infinte_loop);
#endif

    demo_shutdown();
    SDL_GL_DeleteContext(gContext);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();

    return 0;
}

void main_loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
    {
        ImGuiHelper::FeedEvent(event);
        switch (event.type)
        {
            case SDL_WINDOWEVENT:
                if (event.window.windowID == SDL_GetWindowID(gWindow))
                {
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        gIsDone = true;
                    }
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        gWidth  = event.window.data1;
                        gHeight = event.window.data2;
                    }
                }
                break;
            case SDL_QUIT: gIsDone = true; break;

            default: break;
        }
        //
    }

    // game logic
    glViewport(0, 0, gWidth, gHeight);
    demo_draw();


    [[maybe_unused]] const auto viewport = ImGuiHelper::Begin();
    demo_imgui();
    ImGuiHelper::End();

    // swap framebuffers
    SDL_GL_SwapWindow(gWindow);
}
