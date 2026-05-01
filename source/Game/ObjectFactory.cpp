#include "ObjectFactory.hpp"

#include "Bonfire.hpp"
#include "BossStar.hpp"
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

    CS230::GameObject* CreateLaserStar(Math::vec2 pos, Player* player, const std::string& id)
    {
        // Required ID format:
        //
        // LS_(LaserType)_(FireMode)_(Pattern)_(Direction)_(Num)
        //
        // Examples:
        // LS_Y_CONT_STATIC_E_01
        // LS_R_CONT_ROTATE_N_02
        // LS_W_CONT_BLINK_E_03
        // LS_Y_SHOT_TRACK_E_04
        // LS_R_SHOT_TRACK_E_05

        auto parts = SplitID(id, '_');

        if (parts.size() != 6 || parts[0] != "LS")
        {
            Engine::GetLogger().LogError("Invalid LaserStar id: " + id);
            Engine::GetLogger().LogError("Expected: LS_(LaserType)_(FireMode)_(Pattern)_(Direction)_(Num)");
            return nullptr;
        }

        LaserStar::LaserType type    = LaserTypeFromIDToken(parts[1]);
        LaserStar::FireMode  mode    = LaserFireModeFromIDToken(parts[2]);
        LaserStar::Pattern   pattern = LaserPatternFromIDToken(parts[3]);
        Math::vec2           dir     = DirectionFromIDToken(parts[4]);

        auto* star = new LaserStar(pos, player, type, pattern, dir, mode);
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
            #ff00ff = BossStar
            */

            if (color == "#786721" || id.find("door") != std::string::npos)
            {
                auto* door = new Door(pos, { 100.0, 100.0 });
                door->SetName(id);
                return door;
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

            if (color == "#ff00ff")
            {
                auto* boss = new BossStar(pos, player);
                boss->SetName(id);
                return boss;
            }

            return nullptr;
        };
    }
}