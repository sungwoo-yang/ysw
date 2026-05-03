#include "ObjectFactory.hpp"

#include "Bonfire.hpp"
#include "Door.hpp"
#include "Gate.hpp"
#include "LaserStar.hpp"
#include "Mirror.hpp"
#include "Player.hpp"
#include "PushableMirror.hpp"
#include "Sign.hpp"
#include "TargetStar.hpp"

#include "Engine/BackgroundElement.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string NormalizeColor(std::string color)
    {
        std::transform(color.begin(), color.end(), color.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        return color;
    }

    bool StartsWith(const std::string& text, const std::string& prefix)
    {
        return text.rfind(prefix, 0) == 0;
    }

    std::vector<std::string> SplitID(const std::string& s, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string              token;
        std::istringstream       tokenStream(s);

        while (std::getline(tokenStream, token, delimiter))
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    // Star
    LaserStar::LaserType LaserTypeFromIDToken(const std::string& token)
    {
        if (token == "Y")
        {
            return LaserStar::LaserType::Yellow;
        }

        if (token == "R")
        {
            return LaserStar::LaserType::Red;
        }

        if (token == "W")
        {
            return LaserStar::LaserType::White;
        }

        Engine::GetLogger().LogError("Invalid LaserStar LaserType token: " + token);
        return LaserStar::LaserType::White;
    }

    LaserStar::FireMode LaserFireModeFromIDToken(const std::string& token)
    {
        if (token == "CONT")
        {
            return LaserStar::FireMode::Continuous;
        }

        if (token == "SHOT")
        {
            return LaserStar::FireMode::WarningShot;
        }

        Engine::GetLogger().LogError("Invalid LaserStar FireMode token: " + token);
        return LaserStar::FireMode::Continuous;
    }

    LaserStar::Pattern LaserPatternFromIDToken(const std::string& token)
    {
        if (token == "STATIC")
        {
            return LaserStar::Pattern::Static;
        }

        if (token == "ROTATE")
        {
            return LaserStar::Pattern::Rotating;
        }

        if (token == "BLINK")
        {
            return LaserStar::Pattern::Blink;
        }

        if (token == "TRACK")
        {
            return LaserStar::Pattern::Tracking;
        }

        Engine::GetLogger().LogError("Invalid LaserStar Pattern token: " + token);
        return LaserStar::Pattern::Static;
    }

    Math::vec2 DirectionFromIDToken(const std::string& token)
    {
        if (token == "N")
        {
            return { 0.0, 1.0 };
        }
        if (token == "S")
        {
            return { 0.0, -1.0 };
        }
        if (token == "E")
        {
            return { 1.0, 0.0 };
        }
        if (token == "W")
        {
            return { -1.0, 0.0 };
        }
        if (token == "NE")
        {
            return { 0.707, 0.707 };
        }
        if (token == "NW")
        {
            return { -0.707, 0.707 };
        }
        if (token == "SE")
        {
            return { 0.707, -0.707 };
        }
        if (token == "SW")
        {
            return { -0.707, -0.707 };
        }

        return { 0.0, -1.0 };
    }

    LaserStar::MoveMode LaserMoveModeFromIDToken(const std::string& token)
    {
        if (token == "NONE")
        {
            return LaserStar::MoveMode::None;
        }

        if (token == "LINEAR")
        {
            return LaserStar::MoveMode::Linear;
        }

        Engine::GetLogger().LogError("Invalid LaserStar MoveMode token: " + token);
        return LaserStar::MoveMode::None;
    }

    CS230::GameObject* CreateLaserStar(Math::vec2 pos, Player* player, const std::string& id)
    {
        // Static format:
        // LS_(LaserType)_(FireMode)_(Pattern)_(LaserDirection)_(Name)
        //
        // Example:
        // LS_R_CONT_STATIC_E_P1A
        // LS_Y_SHOT_TRACK_E_ARIESMAIN
        //
        // Moving format:
        // LS_(LaserType)_(FireMode)_(Pattern)_(LaserDirection)_(MoveMode)_(MoveDirection)_(Speed)_(Distance)_(Name)
        //
        // Example:
        // LS_R_CONT_STATIC_S_LINEAR_E_80_3000_P1CHASE

        auto parts = SplitID(id, '_');

        if (parts.size() != 6 && parts.size() != 10)
        {
            Engine::GetLogger().LogError("Invalid LaserStar id: " + id);
            Engine::GetLogger().LogError("Expected static: LS_(LaserType)_(FireMode)_(Pattern)_(LaserDirection)_(Name)");
            Engine::GetLogger().LogError("Expected moving: LS_(LaserType)_(FireMode)_(Pattern)_(LaserDirection)_(MoveMode)_(MoveDirection)_(Speed)_(Distance)_(Name)");
            return nullptr;
        }

        if (parts[0] != "LS")
        {
            Engine::GetLogger().LogError("Invalid LaserStar prefix: " + id);
            return nullptr;
        }

        LaserStar::LaserType type     = LaserTypeFromIDToken(parts[1]);
        LaserStar::FireMode  mode     = LaserFireModeFromIDToken(parts[2]);
        LaserStar::Pattern   pattern  = LaserPatternFromIDToken(parts[3]);
        Math::vec2           laserDir = DirectionFromIDToken(parts[4]);

        LaserStar::MoveMode moveMode = LaserStar::MoveMode::None;
        Math::vec2          moveDir  = { 0.0, 0.0 };
        double              speed    = 0.0;
        double              distance = 0.0;

        if (parts.size() == 10)
        {
            moveMode = LaserMoveModeFromIDToken(parts[5]);
            moveDir  = DirectionFromIDToken(parts[6]);

            try
            {
                speed    = std::stod(parts[7]);
                distance = std::stod(parts[8]);
            }
            catch (const std::exception& e)
            {
                Engine::GetLogger().LogError("Invalid LaserStar movement number in id: " + id);
                Engine::GetLogger().LogError(e.what());
                return nullptr;
            }
        }

        auto* star = new LaserStar(pos, player, type, pattern, laserDir, mode, moveMode, moveDir, speed, distance);
        star->SetName(id);

        return star;
    }

    CS230::GameObject* CreateSign(Math::vec2 pos, const std::string& id, const ObjectFactory::SignTextMap& signTexts)
    {
        std::string message = "Test Message";

        auto it = signTexts.find(id);
        if (it != signTexts.end())
        {
            message = it->second;
        }

        auto* sign = new Sign(pos, { 100.0, 100.0 }, message);
        sign->SetName(id);

        return sign;
    }

    // Door
    Door::Destination DoorDestinationFromIDToken(const std::string& token)
    {
        if (token == "MODE1")
        {
            return Door::Destination::Mode1;
        }

        if (token == "MODE3")
        {
            return Door::Destination::Mode3;
        }

        if (token == "BOSS1")
        {
            return Door::Destination::Boss1;
        }

        Engine::GetLogger().LogError("Invalid Door destination token: " + token);
        return Door::Destination::None;
    }

    Door::Event DoorEventFromIDToken(const std::string& token)
    {
        if (token == "BOSSREFLECT")
        {
            return Door::Event::BossStartReflect;
        }

        return Door::Event::None;
    }

    Door::Config DoorConfigFromID(const std::string& id)
    {
        Door::Config config;

        auto parts = SplitID(id, '_');

        if (parts.empty() || parts[0] != "DOOR")
        {
            Engine::GetLogger().LogError("Invalid Door id: " + id);
            return config;
        }

        if (parts.size() >= 3 && parts[1] == "STATE")
        {
            config.actionType  = Door::ActionType::ChangeState;
            config.destination = DoorDestinationFromIDToken(parts[2]);
            return config;
        }

        if (parts.size() >= 3 && parts[1] == "CUTSCENE")
        {
            config.actionType  = Door::ActionType::CutsceneThenChangeState;
            config.destination = DoorDestinationFromIDToken(parts[2]);
            return config;
        }

        if (parts.size() >= 4 && parts[1] == "TELEPORT")
        {
            config.actionType = Door::ActionType::Teleport;

            try
            {
                config.teleportPosition = { std::stod(parts[2]), std::stod(parts[3]) };
            }
            catch (const std::exception& e)
            {
                Engine::GetLogger().LogError("Invalid Door teleport position in id: " + id);
                Engine::GetLogger().LogError(e.what());

                config.actionType = Door::ActionType::None;
                return config;
            }

            if (parts.size() >= 5)
            {
                config.event = DoorEventFromIDToken(parts[4]);
            }

            return config;
        }

        Engine::GetLogger().LogError("Unknown Door id format: " + id);
        return config;
    }

    CS230::GameObject* CreateDoor(Math::vec2 pos, const std::string& id)
    {
        auto* door = new Door(pos, { 100.0, 100.0 });

        door->SetName(id);
        door->SetConfig(DoorConfigFromID(id));

        return door;
    }
}

namespace ObjectFactory
{
    CS230::MapManager::GameObjectFactory Create(Player* player, SignTextMap signTexts)
    {
        return [player, signTexts](GameObjectTypes /*type*/, Math::vec2 pos, const std::string& rawColor, const std::string& id) -> CS230::GameObject*
        {
            if (player == nullptr)
            {
                return nullptr;
            }

            const std::string color = NormalizeColor(rawColor);

            /*
            ================================
            Current color table
            ================================

            #00ffff = Platform / Floor
                      MapManager already creates floor geometry automatically.
                      This factory should return nullptr for that color.

            #786721 = Door
            #808080 = Pillar / background decoration
            #00aaff = Sign
            #ff0000 = Bonfire
            #0000ff = Gate
            #aa00ff = Static Mirror
            #8844ff = Pushable Mirror

            #ffffff = TargetStar
            #00ff00 = LaserStar
            */

            if (color == "#786721" || StartsWith(id, "DOOR_"))
            {
                return CreateDoor(pos, id);
            }

            if (color == "#808080")
            {
                Engine::GetLogger().LogEvent("Pillar created at: " + std::to_string(pos.x) + ", " + std::to_string(pos.y));

                auto* pillar = new CS230::BackgroundElement(pos, "Assets/textures/pillar.png");
                pillar->SetName(id);
                return pillar;
            }

            if (color == "#00aaff" || StartsWith(id, "sign"))
            {
                return CreateSign(pos, id, signTexts);
            }

            if (color == "#ff0000")
            {
                auto* bonfire = new Bonfire(pos, { 100.0, 100.0 });
                bonfire->SetName(id);
                return bonfire;
            }

            if (color == "#0000ff")
            {
                auto* gate = new Gate(pos, { 100.0, 100.0 });
                gate->SetName(id);
                return gate;
            }

            if (color == "#aa00ff")
            {
                auto* mirror = new Mirror(pos, { 100.0, 100.0 });
                mirror->SetName(id);
                return mirror;
            }

            if (color == "#8844ff")
            {
                auto* mirror = new PushableMirror(pos, { 100.0, 100.0 });
                mirror->SetName(id);
                return mirror;
            }

            if (color == "#ffffff")
            {
                auto* target = new TargetStar(pos);
                target->SetName(id);
                return target;
            }

            if (color == "#00ff00")
            {
                return CreateLaserStar(pos, player, id);
            }

            return nullptr;
        };
    }
}