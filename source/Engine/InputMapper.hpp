#pragma once
#include "Input.hpp"
#include <map>
#include <string>
#include <filesystem>

namespace CS230
{
    enum class GameAction
    {
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        Jump,
        Attack,
        Dash,
        Interact,
        Pause,
        MenuSelect,
        MenuBack,
        Count
    };

    class InputMapper
    {
    public:
        static InputMapper& Instance();

        void Init();
        
        bool IsPressed(GameAction action);
        bool IsJustPressed(GameAction action);
        bool IsJustReleased(GameAction action);

        void SetBinding(GameAction action, Input::Keys key);
        Input::Keys GetBinding(GameAction action) const;
        std::string GetBindingName(GameAction action) const;

        void LoadBindings(const std::filesystem::path& filepath = "keybindings.cfg");
        void SaveBindings(const std::filesystem::path& filepath = "keybindings.cfg");

        std::string ActionToString(GameAction action) const;

    private:
        InputMapper();
        ~InputMapper() = default;

        void SetDefaultBindings();
        Input::Keys StringToKey(const std::string& str);

        std::map<GameAction, Input::Keys> bindings;
    };
}