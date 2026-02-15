#include "InputMapper.hpp"
#include "Engine.hpp"
#include "Logger.hpp"
#include <fstream>
#include <sstream>

namespace CS230
{
    // Lazy initialization of the singleton instance
    InputMapper& InputMapper::Instance()
    {
        static InputMapper instance;
        return instance;
    }

    InputMapper::InputMapper()
    {
        // Ensure standard controls are available immediately upon creation
        SetDefaultBindings();
    }

    void InputMapper::Init()
    {
        // Attempt to override defaults with user preferences on startup
        LoadBindings();
    }

    void InputMapper::SetDefaultBindings()
    {
        bindings.clear();
        
        // Standard PC control scheme
        bindings[GameAction::MoveLeft]  = Input::Keys::A;
        bindings[GameAction::MoveRight] = Input::Keys::D;
        bindings[GameAction::MoveUp]    = Input::Keys::W;
        bindings[GameAction::MoveDown]  = Input::Keys::S;

        // Action bindings
        bindings[GameAction::Jump]      = Input::Keys::Space;
        bindings[GameAction::Attack]    = Input::Keys::J;
        bindings[GameAction::Dash]      = Input::Keys::LShift;
        bindings[GameAction::Interact]  = Input::Keys::F;

        // System / UI bindings
        bindings[GameAction::Pause]     = Input::Keys::Escape;
        bindings[GameAction::MenuSelect]= Input::Keys::Enter;
        bindings[GameAction::MenuBack]  = Input::Keys::Escape;
    }

    bool InputMapper::IsPressed(GameAction action)
    {
        // Find the physical key associated with the action and poll the core input system
        auto it = bindings.find(action);
        if (it != bindings.end())
        {
            return Engine::GetInput().KeyDown(it->second);
        }
        return false;
    }

    bool InputMapper::IsJustPressed(GameAction action)
    {
        auto it = bindings.find(action);
        if (it != bindings.end())
        {
            return Engine::GetInput().KeyJustPressed(it->second);
        }
        return false;
    }

    bool InputMapper::IsJustReleased(GameAction action)
    {
        auto it = bindings.find(action);
        if (it != bindings.end())
        {
            return Engine::GetInput().KeyJustReleased(it->second);
        }
        return false;
    }

    void InputMapper::SetBinding(GameAction action, Input::Keys key)
    {
        bindings[action] = key;
    }

    Input::Keys InputMapper::GetBinding(GameAction action) const
    {
        auto it = bindings.find(action);
        if (it != bindings.end())
        {
            return it->second;
        }
        return Input::Keys::Count;  // Returns invalid key if not mapped
    }

    std::string InputMapper::GetBindingName(GameAction action) const
    {
        Input::Keys key = GetBinding(action);
        return to_string(key);  // Relies on an external to_string utility for keys
    }

    std::string InputMapper::ActionToString(GameAction action) const
    {
        switch (action)
        {
            case GameAction::MoveLeft:  return "Move Left";
            case GameAction::MoveRight: return "Move Right";
            case GameAction::MoveUp:    return "Move Up";
            case GameAction::MoveDown:  return "Move Down";
            case GameAction::Jump:      return "Jump";
            case GameAction::Attack:    return "Attack";
            case GameAction::Dash:      return "Dash";
            case GameAction::Interact:  return "Interact";
            case GameAction::Pause:     return "Pause";
            case GameAction::MenuSelect:return "Select";
            case GameAction::MenuBack:  return "Back";
            default: return "Unknown";
        }
    }

    Input::Keys InputMapper::StringToKey(const std::string& str)
    {
        // Iterate through all possible keys to find a string match
        for (int i = 0; i < static_cast<int>(Input::Keys::Count); ++i)
        {
            Input::Keys key = static_cast<Input::Keys>(i);
            if (str == to_string(key))
            {
                return key;
            }
        }
        return Input::Keys::Count;
    }

    void InputMapper::SaveBindings(const std::filesystem::path& filepath)
    {
        std::ofstream file(filepath);
        if (!file.is_open()) return;

        file << "# Key Bindings\n";

        // Serialize the action-to-key mapping into a simple "Action=Key" text format
        for (const auto& [action, key] : bindings)
        {
            file << ActionToString(action) << "=" << to_string(key) << "\n";
        }
        Engine::GetLogger().LogEvent("Key Bindings Saved.");
    }

    void InputMapper::LoadBindings(const std::filesystem::path& filepath)
    {
        std::ifstream file(filepath);

        // If no config file exists, fall back to default controls
        if (!file.is_open())
        {
            Engine::GetLogger().LogEvent("No binding file found. Using defaults.");
            SetDefaultBindings();
            return;
        }

        std::string line;
        while (std::getline(file, line))
        {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string actionStr, keyStr;

            // Parse lines formatted as "Action=Key"
            if (std::getline(iss, actionStr, '=') && std::getline(iss, keyStr))
            {
                // Match the string representation of the action to the GameAction enum
                for (int i = 0; i < static_cast<int>(GameAction::Count); ++i)
                {
                    GameAction act = static_cast<GameAction>(i);
                    if (ActionToString(act) == actionStr)
                    {
                        Input::Keys key = StringToKey(keyStr);
                        if (key != Input::Keys::Count)
                        {
                            bindings[act] = key;    // Apply the parsed binding
                        }
                        break;
                    }
                }
            }
        }
        Engine::GetLogger().LogEvent("Key Bindings Loaded.");
    }
}