#include "ObjectFactory.hpp"

#include "Bonfire.hpp"
#include "BreakableWall.hpp"
#include "Door.hpp"
#include "Elevator.hpp"
#include "FallingBlock.hpp"
#include "Gate.hpp"
#include "LaserCutRope.hpp"
#include "LaserStar.hpp"
#include "LaserTurret.hpp"
#include "Mirror.hpp"
#include "Player.hpp"
#include "PushableMirror.hpp"
#include "Ramp.hpp"
#include "Sign.hpp"
#include "Spike.hpp"
#include "Staircase.hpp"
#include "TargetStar.hpp"
#include "WaterZone.hpp"

#include "Engine/BackgroundElement.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapElement.h"
#include "Engine/MapManager.h"
#include "Engine/Polygon.h"

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

        if (id.find("BOSS") != std::string::npos)
        {
            // Boss room is about 2400 x 1360, diagonal is about 2758.
            // These values let boss LaserStars activate anywhere inside the room.
            star->SetDetectionRadii(3200.0, 3600.0);
        }

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
        if (token == "REFLECT")
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
        return [player, signTexts](GameObjectTypes /*type*/, Math::vec2 pos, const std::string& rawColor, const std::string& id, const Polygon& polygon) -> CS230::GameObject*
        {
            if (player == nullptr)
            {
                return nullptr;
            }

            Polygon    polygonCopy = polygon;
            Math::rect bounds      = polygonCopy.FindBoundary();

            Math::vec2 svgSize{ bounds.Right() - bounds.Left(), bounds.Top() - bounds.Bottom() };

            if (svgSize.x <= 0.0)
            {
                svgSize.x = 10.0;
            }

            if (svgSize.y <= 0.0)
            {
                svgSize.y = 10.0;
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

            #ffd700 = TargetStar
            #39ff14 = LaserStar

            #00cc00 = LaserCutRope
            #cc00cc = FallingBlock
            #884400 = BreakableWall  (dash to break)
            #AA6600 = Staircase     id: STAIR_(steps)_(R|L)  e.g. STAIR_6_R
            #cc2222 = Spike         id: SPIKE_U/D/L/R  (default Up)
            #4488cc = Elevator      id: ELEV_<dist>_<period>  e.g. ELEV_400_4
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
                const Math::vec2 bbCenter{ (bounds.Left() + bounds.Right()) * 0.5, (bounds.Bottom() + bounds.Top()) * 0.5 };
                auto*            bonfire = new Bonfire(pos + bbCenter, { 100.0, 100.0 });
                bonfire->SetName(id);
                return bonfire;
            }

            if (color == "#0000ff")
            {
                auto* gate = new Gate(pos, { 100.0, 100.0 });
                gate->SetName(id);
                return gate;
            }

            if (color == "#00cc00")
            {
                auto* rope = new LaserCutRope(pos, svgSize);
                rope->SetName(id);
                return rope;
            }

            if (color == "#cc00cc")
            {
                auto* block = new FallingBlock(pos, svgSize);
                block->SetName(id);
                return block;
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

            if (color == "#ffd700" || StartsWith(id, "TS_"))
            {
                auto* target = new TargetStar(pos);
                target->SetName(id);
                return target;
            }

            if (color == "#39ff14" || StartsWith(id, "LS_"))
            {
                return CreateLaserStar(pos, player, id);
            }

            // LaserTurret: #ff6600
            //   LTRT_(dir)_(cooldown)_(delay) = fires only when player is in line
            //   ATRT_(dir)_(cooldown)_(delay) = fires after delay, then waits cooldown between shots
            // delay is optional; without it, the first shot/check waits one cooldown.
            if (color == "#ff6600")
            {
                auto       parts               = SplitID(id, '_');
                Math::vec2 dir                 = { 0.0, -1.0 }; // default South
                double     interval            = 2.0;
                double     initialDelay        = -1.0;
                bool       requirePlayerInPath = true;
                if (parts.size() >= 3 && (parts[0] == "LTRT" || parts[0] == "ATRT"))
                {
                    requirePlayerInPath = parts[0] != "ATRT";
                    dir                 = DirectionFromIDToken(parts[1]);
                    try
                    {
                        interval = std::stod(parts[2]);
                    }
                    catch (...)
                    {
                    }
                    if (parts.size() >= 4)
                        try
                        {
                            initialDelay = std::max(0.0, std::stod(parts[3]));
                        }
                        catch (...)
                        {
                        }
                }
                auto* turret = new LaserTurret(pos, dir, interval, player, requirePlayerInPath, initialDelay);
                turret->SetName(id);
                return turret;
            }

            if (color == "#884400")
            {
                const Math::vec2 bbCenter{ (bounds.Left() + bounds.Right()) * 0.5, (bounds.Bottom() + bounds.Top()) * 0.5 };
                auto*            wall = new BreakableWall(pos + bbCenter, svgSize, id);
                wall->SetName(id);
                return wall;
            }

            if (color == "#aa6600")
            {
                const Math::vec2 bbCenter{ (bounds.Left() + bounds.Right()) * 0.5, (bounds.Bottom() + bounds.Top()) * 0.5 };
                const Math::vec2 center = pos + bbCenter;

                // Detect RAMP vs STAIR from ID prefix
                const auto parts  = SplitID(id, '_');
                const bool isRamp = (!parts.empty() && parts[0] == "RAMP");

                // Shared direction parse: last token R or L
                Ramp::Dir rampDir = Ramp::Dir::UpRight;
                if (!parts.empty() && parts.back() == "L")
                    rampDir = Ramp::Dir::UpLeft;

                // Inject triangular floor collision (same for both Ramp and Staircase)
                auto* gom    = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();
                auto* mapMgr = Engine::GetGameStateManager().GetGSComponent<CS230::MapManager>();
                if (gom && mapMgr)
                {
                    const double hw = svgSize.x * 0.5;
                    const double hh = svgSize.y * 0.5;

                    Polygon rampLocal;
                    if (rampDir == Ramp::Dir::UpRight)
                        rampLocal.vertices = {
                            { -hw, -hh },
                            {  hw, -hh },
                            {  hw,  hh }
                        };
                    else
                        rampLocal.vertices = {
                            { -hw, -hh },
                            {  hw, -hh },
                            { -hw,  hh }
                        };
                    rampLocal.vertexCount = 3;

                    Polygon rampWorld = rampLocal;
                    for (auto& v : rampWorld.vertices)
                        v += center;
                    mapMgr->AddPolygon(rampWorld);

                    auto* rampElem = new CS230::MapElement(center, rampLocal, GameObjectTypes::Floor);
                    gom->Add(rampElem);
                }

                if (isRamp)
                {
                    auto* ramp = new Ramp(center, svgSize, rampDir);
                    ramp->SetName(id);
                    return ramp;
                }

                // Original staircase path
                int stepCount = 5;
                {
                    if (parts.size() >= 2 && parts[0] == "STAIR")
                        try
                        {
                            stepCount = std::stoi(parts[1]);
                        }
                        catch (...)
                        {
                        }
                }
                stepCount = std::max(2, std::min(20, stepCount));

                const Staircase::Dir stairDir = (rampDir == Ramp::Dir::UpLeft) ? Staircase::Dir::UpLeft : Staircase::Dir::UpRight;

                auto* stair = new Staircase(center, svgSize, stepCount, stairDir);
                stair->SetName(id);
                return stair;
            }

            if (color == "#cc2222")
            {
                const Math::vec2 bbCenter{ (bounds.Left() + bounds.Right()) * 0.5, (bounds.Bottom() + bounds.Top()) * 0.5 };

                Spike::Dir dir = Spike::Dir::Up;
                if (id.find("_D") != std::string::npos)
                    dir = Spike::Dir::Down;
                else if (id.find("_L") != std::string::npos)
                    dir = Spike::Dir::Left;
                else if (id.find("_R") != std::string::npos)
                    dir = Spike::Dir::Right;

                auto* spike = new Spike(pos + bbCenter, svgSize, dir);
                spike->SetName(id);
                return spike;
            }

            if (color == "#0044cc")
            {
                // parsePathData duplicates the first vertex when it reads the SVG "Z" close command,
                // so FindCenter() (vertex average) is offset. Use the bounding-box centre instead.
                const Math::vec2 bbOffset{ (bounds.Left() + bounds.Right()) * 0.5, (bounds.Bottom() + bounds.Top()) * 0.5 };
                auto*            water = new WaterZone(pos + bbOffset, svgSize);
                water->SetName(id);
                return water;
            }

            if (color == "#4488cc")
            {
                // ELEV_<dist>_<period>  e.g. ELEV_400_4
                // dist   = vertical travel distance in game-world units (positive = upward)
                // period = full cycle duration in seconds (up and back)
                double travelDist = 400.0;
                double period     = 4.0;

                const auto parts = SplitID(id, '_');
                if (parts.size() >= 2)
                    try
                    {
                        travelDist = std::stod(parts[1]);
                    }
                    catch (...)
                    {
                    }
                if (parts.size() >= 3)
                    try
                    {
                        period = std::stod(parts[2]);
                    }
                    catch (...)
                    {
                    }

                const Math::vec2 bbCenter{ (bounds.Left() + bounds.Right()) * 0.5, (bounds.Bottom() + bounds.Top()) * 0.5 };
                auto*            elev = new Elevator(pos + bbCenter, svgSize, travelDist, period);
                elev->SetName(id);
                return elev;
            }

            return nullptr;
        };
    }
}
