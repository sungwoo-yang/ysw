#pragma once
#include "Input.hpp"
#include <map>
#include <string>
#include <filesystem>

namespace CS230
{
    // Defines logical game actions abstracted from physical hardware keys.
    // This allows the game logic to check for "Jump" instead of specifically checking for the "Spacebar".
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
        Count   // Used to iterate over the enum values
    };

    // Singleton manager that handles the mapping between physical hardware input and logical game actions.
    // Supports rebindable controls and configuration serialization.
    class InputMapper
    {
    public:
        // Retrieves the singleton instance of the InputMapper
        static InputMapper& Instance();

        // Initializes the mapper by loading saved bindings from disk
        void Init();
        
        // Input state queries for specific logical actions
        bool IsPressed(GameAction action);
        bool IsJustPressed(GameAction action);
        bool IsJustReleased(GameAction action);

        // Binding management
        void SetBinding(GameAction action, Input::Keys key);
        Input::Keys GetBinding(GameAction action) const;
        std::string GetBindingName(GameAction action) const;

        // Serialization of key bindings to and from a configuration file
        void LoadBindings(const std::filesystem::path& filepath = "keybindings.cfg");
        void SaveBindings(const std::filesystem::path& filepath = "keybindings.cfg");

        // Converts a GameAction enum to a human-readable string for UI and saving
        std::string ActionToString(GameAction action) const;

    private:
        // Private constructor enforces the singleton pattern
        InputMapper();
        ~InputMapper() = default;

        // Establishes fallback mappings (e.g., WASD for movement)
        void SetDefaultBindings();

        // Helper to convert a string representation of a key back to the Input::Keys enum
        Input::Keys StringToKey(const std::string& str);

        // Stores the active mapping of logical actions to physical keys
        std::map<GameAction, Input::Keys> bindings;
    };
}