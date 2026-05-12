/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Window.hpp"
#include "CS200/RenderingAPI.hpp"
#include "Engine.hpp"
#include "Error.hpp"
#include "Logger.hpp"
#include "OpenGL/GL.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <algorithm>
#include <functional>
#include <sstream>

namespace
{
    void hint_gl(SDL_GLattr attr, int value)
    {
        // // https://wiki.libsdl.org/SDL2/SDL_GL_SetAttribute
        if (const auto success = SDL_GL_SetAttribute(attr, value); success != 0)
        {
            Engine::GetLogger().LogError(std::string{ "Failed to Set GL Attribute: " } + SDL_GetError());
        }
    }

    Math::ivec2 fit_size_to_current_display(SDL_Window* window, Math::ivec2 desired_size)
    {
        SDL_Rect display_bounds{};
        int      display_index = 0;

        if (window != nullptr)
        {
            display_index = SDL_GetWindowDisplayIndex(window);
            if (display_index < 0)
            {
                display_index = 0;
            }
        }

        if (SDL_GetDisplayUsableBounds(display_index, &display_bounds) != 0 || display_bounds.w <= 0 || display_bounds.h <= 0)
        {
            if (SDL_GetDisplayBounds(display_index, &display_bounds) != 0 || display_bounds.w <= 0 || display_bounds.h <= 0)
            {
                return desired_size;
            }
        }

        if (desired_size.x <= display_bounds.w && desired_size.y <= display_bounds.h)
        {
            return desired_size;
        }

        const double scale = std::min(
            static_cast<double>(display_bounds.w) / static_cast<double>(desired_size.x),
            static_cast<double>(display_bounds.h) / static_cast<double>(desired_size.y));

        Math::ivec2 fitted_size = {
            std::max(1, static_cast<int>(static_cast<double>(desired_size.x) * scale)),
            std::max(1, static_cast<int>(static_cast<double>(desired_size.y) * scale))
        };

        fitted_size.x = std::min(fitted_size.x, display_bounds.w);
        fitted_size.y = std::min(fitted_size.y, display_bounds.h);

        return fitted_size;
    }
}

namespace CS230
{
    void Window::Start(std::string_view title)
    {
        setupSDLWindow(title);
        setupOpenGL();

        SDL_GetWindowSize(sdlWindow, &size.x, &size.y);

        CS200::RenderingAPI::SetClearColor(default_background);

        is_closed = false;
    }

    void Window::Update()
    {
        SDL_GL_SwapWindow(sdlWindow);
        SDL_Event event{ 0 };
        while (SDL_PollEvent(&event) != 0)
        {
            eventCallback(event);

            switch (event.type)
            {
                case SDL_QUIT: is_closed = true; break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        is_closed = true;
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        size.x = event.window.data1;
                        size.y = event.window.data2;
                        GL::Viewport(0, 0, size.x, size.y);
                    }
                    break;
            }
        }
    }

    bool Window::IsClosed() const
    {
        return is_closed;
    }

    Math::ivec2 Window::GetSize() const
    {
        return size;
    }

    void Window::Clear(CS200::RGBA color)
    {
        CS200::RenderingAPI::SetClearColor(color);
        CS200::RenderingAPI::Clear();
    }

    void Window::ForceResize(int desired_width, int desired_height)
    {
        const Math::ivec2 fitted_size = fit_size_to_current_display(sdlWindow, { desired_width, desired_height });
        SDL_SetWindowSize(sdlWindow, fitted_size.x, fitted_size.y);
        SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_GetWindowSize(sdlWindow, &size.x, &size.y);
        GL::Viewport(0, 0, size.x, size.y);
    }

    void Window::SetFullscreen(bool fullscreen)
    {
        if (fullscreen)
        {
            // Use SDL_WINDOW_FULLSCREEN_DESKTOP to prevent desktop windows from rearranging
            SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
        else
        {
            SDL_SetWindowFullscreen(sdlWindow, 0);
        }
        SDL_GetWindowSize(sdlWindow, &size.x, &size.y);
        GL::Viewport(0, 0, size.x, size.y);
    }

    void Window::SetBordered(bool bordered)
    {
        SDL_SetWindowBordered(sdlWindow, bordered ? SDL_TRUE : SDL_FALSE);
    }

    void Window::SetVSync(bool enabled)
    {
        if (!enabled)
        {
            SDL_GL_SetSwapInterval(0);
            return;
        }

        constexpr int ADAPTIVE_VSYNC = -1;
        constexpr int VSYNC          = 1;
        if (SDL_GL_SetSwapInterval(ADAPTIVE_VSYNC) != 0)
        {
            SDL_GL_SetSwapInterval(VSYNC);
        }
    }

    SDL_Window* Window::GetSDLWindow() const
    {
        return sdlWindow;
    }

    SDL_GLContext Window::GetGLContext() const
    {
        return glContext;
    }

    void Window::SetEventCallback(WindowEventCallback callback)
    {
        eventCallback = callback;
    }

    Window::~Window()
    {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(sdlWindow);
        SDL_Quit();
    }

    void Window::setupSDLWindow(std::string_view title)
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            throw_error_message("Failed to init SDK error: ", SDL_GetError());
        }

#if defined(IS_WEBGL2)
        hint_gl(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        hint_gl(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        hint_gl(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
        hint_gl(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
        hint_gl(SDL_GL_DOUBLEBUFFER, true);
        hint_gl(SDL_GL_STENCIL_SIZE, 8);
        hint_gl(SDL_GL_DEPTH_SIZE, 24);
        hint_gl(SDL_GL_RED_SIZE, 8);
        hint_gl(SDL_GL_GREEN_SIZE, 8);
        hint_gl(SDL_GL_BLUE_SIZE, 8);
        hint_gl(SDL_GL_ALPHA_SIZE, 8);
        hint_gl(SDL_GL_MULTISAMPLEBUFFERS, 1);
        hint_gl(SDL_GL_MULTISAMPLESAMPLES, 4);

        sdlWindow = SDL_CreateWindow(title.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, default_width, default_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP);
        
        if (sdlWindow == nullptr)
    {
        throw_error_message("Failed to create window: ", SDL_GetError());
    }
    }

    void Window::setupOpenGL()
    {
        if (glContext = SDL_GL_CreateContext(sdlWindow); glContext == nullptr)
        {
            throw_error_message("Failed to create opengl context: ", SDL_GetError());
        }

        SDL_GL_MakeCurrent(sdlWindow, glContext);

        if (const auto result = glewInit(); GLEW_OK != result)
        {
            throw_error_message("Unable to initialize GLEW - error: ", glewGetErrorString(result));
        }

        SetVSync(true);

        CS200::RenderingAPI::Init();
    }
}
