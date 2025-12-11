/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include "Vec2.hpp"
#include <gsl/gsl>
#include <vector>

namespace CS230
{
    class Input
    {
    public:
        enum class MouseButton
        {
            Left,
            Middle,
            Right,
            Count
        };

        enum class Keys
        {
            A,
            B,
            C,
            D,
            E,
            F,
            G,
            H,
            I,
            J,
            K,
            L,
            M,
            N,
            O,
            P,
            Q,
            R,
            S,
            T,
            U,
            V,
            W,
            X,
            Y,
            Z,
            Space,
            Enter,
            Left,
            Up,
            Right,
            Down,
            Escape,
            Tab,
            LShift,
            Count
        };

        Input();
        void Update();

        bool KeyDown(Keys key) const;
        bool KeyJustReleased(Keys key) const;
        bool KeyJustPressed(Keys key) const;

        bool       MouseButtonDown(MouseButton button) const;
        bool       MouseButtonJustPressed(MouseButton button) const;
        bool       MouseButtonJustReleased(MouseButton button) const;
        Math::vec2 GetMousePosition() const;

    private:
        std::vector<bool> keys_down;
        std::vector<bool> previous_keys_down;

        std::vector<bool> mouse_buttons_down;
        std::vector<bool> previous_mouse_buttons_down;
        Math::vec2        mouse_position;

    private:
        void SetKeyDown(Keys key, bool value);
        void SetMouseButtonDown(MouseButton button, bool value);
    };

    constexpr Input::Keys& operator++(Input::Keys& the_key) noexcept
    {
        the_key = static_cast<Input::Keys>(static_cast<unsigned>(the_key) + 1);
        return the_key;
    }

    constexpr gsl::czstring to_string(Input::Keys key) noexcept
    {
        switch (key)
        {
            case Input::Keys::A: return "A";
            case Input::Keys::B: return "B";
            case Input::Keys::C: return "C";
            case Input::Keys::D: return "D";
            case Input::Keys::E: return "E";
            case Input::Keys::F: return "F";
            case Input::Keys::G: return "G";
            case Input::Keys::H: return "H";
            case Input::Keys::I: return "I";
            case Input::Keys::J: return "J";
            case Input::Keys::K: return "K";
            case Input::Keys::L: return "L";
            case Input::Keys::M: return "M";
            case Input::Keys::N: return "N";
            case Input::Keys::O: return "O";
            case Input::Keys::P: return "P";
            case Input::Keys::Q: return "Q";
            case Input::Keys::R: return "R";
            case Input::Keys::S: return "S";
            case Input::Keys::T: return "T";
            case Input::Keys::U: return "U";
            case Input::Keys::V: return "V";
            case Input::Keys::W: return "W";
            case Input::Keys::X: return "X";
            case Input::Keys::Y: return "Y";
            case Input::Keys::Z: return "Z";
            case Input::Keys::Space: return "Space";
            case Input::Keys::Enter: return "Enter";
            case Input::Keys::Left: return "Left";
            case Input::Keys::Up: return "Up";
            case Input::Keys::Right: return "Right";
            case Input::Keys::Down: return "Down";
            case Input::Keys::Escape: return "Escape";
            case Input::Keys::Tab: return "Tab";
            case Input::Keys::LShift: return "LShift";
            case Input::Keys::Count: return "Count";
        }
        return "Unknown";
    }
}
