#include "ObjectFactory.hpp"

#include "Bonfire.hpp"
#include "BossStar.hpp"
#include "Door.hpp"
#include "Gate.hpp"
#include "HostileStar.hpp"
#include "Mirror.hpp"
#include "Player.hpp"
#include "PushableMirror.hpp"
#include "PuzzleStar.hpp"
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

    PuzzleStar::LaserType PuzzleLaserTypeFromIDToken(const std::string& token)
    {
        if (token == "Y")
        {
            return PuzzleStar::LaserType::Yellow;
        }
        if (token == "R")
        {
            return PuzzleStar::LaserType::Red;
        }

        return PuzzleStar::LaserType::White;
    }

    PuzzleStar::Pattern PuzzlePatternFromIDToken(const std::string& token)
    {
        if (token == "R")
        {
            return PuzzleStar::Pattern::Rotating;
        }
        if (token == "B")
        {
            return PuzzleStar::Pattern::Blink;
        }

        return PuzzleStar::Pattern::Static;
    }

    CS230::GameObject* CreatePuzzleStar(Math::vec2 pos, Player* player, const std::string& id)
    {
        // ID format:
        // P_(LaserType)_(Pattern)_(Direction)_(Num)
        //
        // Example:
        // P_Y_S_E_01
        // P_R_B_N_02
        // P_W_R_SW_03

        auto parts = SplitID(id, '_');

        if (parts.size() < 5 || parts[0] != "P")
        {
            Engine::GetLogger().LogError("Invalid PuzzleStar id: " + id);
            return nullptr;
        }

        PuzzleStar::LaserType type    = PuzzleLaserTypeFromIDToken(parts[1]);
        PuzzleStar::Pattern   pattern = PuzzlePatternFromIDToken(parts[2]);
        Math::vec2            dir     = DirectionFromIDToken(parts[3]);

        auto* star = new PuzzleStar(pos, player, type, pattern, dir);
        star->SetName(id);

        return star;
    }

    CS230::GameObject* CreateHostileStar(Math::vec2 pos, Player* player, const std::string& id)
    {
        // ID format:
        // H_Y_01
        // H_R_01

        HostileStar::StarType type = HostileStar::StarType::Yellow;

        if (id.find("_R_") != std::string::npos)
        {
            type = HostileStar::StarType::Red;
        }

        auto* star = new HostileStar(pos, player, type);
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
            #00ff00 = PuzzleStar
            #ffff00 = HostileStar
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
                return CreatePuzzleStar(pos, player, id);
            }

            if (color == "#ffff00")
            {
                return CreateHostileStar(pos, player, id);
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