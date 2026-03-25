/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Input.hpp"
#include "Engine.hpp"
#include "Logger.hpp"
#include <SDL.h>

namespace
{
    SDL_Scancode convert_cs230_to_sdl(CS230::Input::Keys cs230_key)
    {
        switch (cs230_key)
        {
            case CS230::Input::Keys::A: return SDL_SCANCODE_A;
            case CS230::Input::Keys::B: return SDL_SCANCODE_B;
            case CS230::Input::Keys::C: return SDL_SCANCODE_C;
            case CS230::Input::Keys::D: return SDL_SCANCODE_D;
            case CS230::Input::Keys::E: return SDL_SCANCODE_E;
            case CS230::Input::Keys::F: return SDL_SCANCODE_F;
            case CS230::Input::Keys::G: return SDL_SCANCODE_G;
            case CS230::Input::Keys::H: return SDL_SCANCODE_H;
            case CS230::Input::Keys::I: return SDL_SCANCODE_I;
            case CS230::Input::Keys::J: return SDL_SCANCODE_J;
            case CS230::Input::Keys::K: return SDL_SCANCODE_K;
            case CS230::Input::Keys::L: return SDL_SCANCODE_L;
            case CS230::Input::Keys::M: return SDL_SCANCODE_M;
            case CS230::Input::Keys::N: return SDL_SCANCODE_N;
            case CS230::Input::Keys::O: return SDL_SCANCODE_O;
            case CS230::Input::Keys::P: return SDL_SCANCODE_P;
            case CS230::Input::Keys::Q: return SDL_SCANCODE_Q;
            case CS230::Input::Keys::R: return SDL_SCANCODE_R;
            case CS230::Input::Keys::S: return SDL_SCANCODE_S;
            case CS230::Input::Keys::T: return SDL_SCANCODE_T;
            case CS230::Input::Keys::U: return SDL_SCANCODE_U;
            case CS230::Input::Keys::V: return SDL_SCANCODE_V;
            case CS230::Input::Keys::W: return SDL_SCANCODE_W;
            case CS230::Input::Keys::X: return SDL_SCANCODE_X;
            case CS230::Input::Keys::Y: return SDL_SCANCODE_Y;
            case CS230::Input::Keys::Z: return SDL_SCANCODE_Z;
            case CS230::Input::Keys::Space: return SDL_SCANCODE_SPACE;
            case CS230::Input::Keys::Enter: return SDL_SCANCODE_RETURN;
            case CS230::Input::Keys::Left: return SDL_SCANCODE_LEFT;
            case CS230::Input::Keys::Up: return SDL_SCANCODE_UP;
            case CS230::Input::Keys::Right: return SDL_SCANCODE_RIGHT;
            case CS230::Input::Keys::Down: return SDL_SCANCODE_DOWN;
            case CS230::Input::Keys::Escape: return SDL_SCANCODE_ESCAPE;
            case CS230::Input::Keys::Tab: return SDL_SCANCODE_TAB;
            case CS230::Input::Keys::LShift: return SDL_SCANCODE_LSHIFT;
            default: return SDL_SCANCODE_UNKNOWN;
        }
    }
}

namespace CS230
{
    Input::Input()
    {
        const int key_count = static_cast<int>(Keys::Count);
        keys_down.resize(key_count, false);
        previous_keys_down.resize(key_count, false);

        const int mouse_btn_count = static_cast<int>(MouseButton::Count);
        mouse_buttons_down.resize(mouse_btn_count, false);
        previous_mouse_buttons_down.resize(mouse_btn_count, false);
        mouse_position = { 0.0, 0.0 };
    }

    void Input::Update()
    {
        previous_keys_down = keys_down;

        const Uint8* SDL_keyboard_state = SDL_GetKeyboardState(nullptr);

        for (Keys key = Keys::A; key < Keys::Count; ++key)
        {
            SDL_Scancode scancode = convert_cs230_to_sdl(key);

            if (scancode != SDL_SCANCODE_UNKNOWN)
            {
                SetKeyDown(key, SDL_keyboard_state[scancode]);
            }
        }

        previous_mouse_buttons_down = mouse_buttons_down;

        int    x, y;
        Uint32 mouse_state = SDL_GetMouseState(&x, &y);
        mouse_position     = { static_cast<double>(x), static_cast<double>(y) };

        SetMouseButtonDown(MouseButton::Left, (mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0);
        SetMouseButtonDown(MouseButton::Middle, (mouse_state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0);
        SetMouseButtonDown(MouseButton::Right, (mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0);
    }

    // KeyBoard
    bool Input::KeyDown(Keys key) const
    {
        return keys_down[static_cast<size_t>(key)];
    }

    bool Input::KeyJustReleased(Keys key) const
    {
        const size_t index = static_cast<size_t>(key);
        return !keys_down[index] && previous_keys_down[index];
    }

    bool Input::KeyJustPressed(Keys key) const
    {
        const size_t index = static_cast<size_t>(key);
        return keys_down[index] && !previous_keys_down[index];
    }

    void Input::SetKeyDown(Keys key, bool value)
    {
        keys_down[static_cast<size_t>(key)] = value;
    }

    // Mouse
    bool Input::MouseButtonDown(MouseButton button) const
    {
        return mouse_buttons_down[static_cast<size_t>(button)];
    }

    bool Input::MouseButtonJustPressed(MouseButton button) const
    {
        const size_t index = static_cast<size_t>(button);
        return mouse_buttons_down[index] && !previous_mouse_buttons_down[index];
    }

    bool Input::MouseButtonJustReleased(MouseButton button) const
    {
        const size_t index = static_cast<size_t>(button);
        return !mouse_buttons_down[index] && previous_mouse_buttons_down[index];
    }

    Math::vec2 Input::GetMousePosition() const
    {
        return mouse_position;
    }

    void Input::SetMouseButtonDown(MouseButton button, bool value)
    {
        mouse_buttons_down[static_cast<size_t>(button)] = value;
    }
}