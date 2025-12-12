#include "Engine/MapManager.h"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapElement.h"
#include "Engine/Path.hpp"
#include "Game/Gate.hpp"

namespace CS230
{
    // --- MapManager ---
    MapManager::~MapManager()
    {
        Unload();
    }

    void MapManager::AddMap(Map* new_map)
    {
        maps.push_back(new_map);
    }

    void MapManager::LoadMap()
    {
        if (maps.empty())
            return;
        maps[currentMapIndex]->OpenSVG();
    }

    void MapManager::Unload()
    {
        for (Map* map : maps)
        {
            delete map;
        }
        maps.clear();
        miniMapPolygons.clear();
    }

    Map* MapManager::GetCurrentMap()
    {
        if (maps.empty())
            return nullptr;
        return maps[currentMapIndex];
    }

    void MapManager::Update([[maybe_unused]] double dt)
    {
        Map* currentMap = GetCurrentMap();
        if (currentMap && !currentMap->IsLevelLoaded())
        {
            currentMap->ParseSVG();
        }
    }

    Map::Map(const std::string& filename)
        : file_path(filename), level_loaded(false), currentCommand('\0'), pathRegex(R"(<path[^>]*\sd\s*=\s*"([^"]+))"), gIdRegex(R"(<g[^>]*\bid\s*=\s*"([^"]+))"),
          transformRegex(R"xxx(transform\s*=\s*"([^"]+)")xxx"), translateRegex(R"(translate\(([^,]+),\s*([^\)]+)\))"), rotateRegex(R"(rotate\(\s*([^\s,]+)\s*,\s*([^\s,]+)\s*,\s*([^\)]+)\s*\))"),
          matrixRegex(R"(matrix\(([^,]+),([^,]+),([^,]+),([^,]+),([^,]+),([^,]+)\))"), pathIdRegex(R"xxx(id="([^"]+)")xxx"), fillColorRegex(R"(fill:\s*(#[0-9a-fA-F]+);)"), gEndTagRegex(R"(</g>)"),
          svgEndTagRegex(R"(</svg>)"), scale({ 1.0f, 1.0f }), IsinG(false), IsTranslate(false), IsRotate(false), IsScale(false)
    {
        try
        {
            this->file_path = assets::locate_asset(filename).string();
            Engine::GetLogger().LogDebug("Map: " + this->file_path);
        }
        catch (const std::exception& e)
        {
            Engine::GetLogger().LogError("Map " + filename);
            Engine::GetLogger().LogError(e.what());
            this->file_path = filename;
        }
    }

    Map::~Map()
    {
        if (map_file.is_open())
            map_file.close();
    }

    void Map::OpenSVG()
    {
        map_file.open(file_path);
        if (!map_file.is_open())
        {
            Engine::GetLogger().LogError(file_path + " SVG");
            return;
        }
        Engine::GetLogger().LogEvent(file_path + " SVG.");
    }

    void Map::ParseSVG()
    {
        if (level_loaded || !map_file.is_open())
            return;

        std::string line;
        if (!std::getline(map_file, line))
        {
            level_loaded = true;
            map_file.close();
            Engine::GetLogger().LogEvent(file_path + "");
            return;
        }
        currentTagBuffer += line;

        size_t tagEnd = currentTagBuffer.find('>');
        if (tagEnd == std::string::npos)
        {
            return;
        }

        std::string currentTag = currentTagBuffer.substr(0, tagEnd + 1);
        currentTagBuffer.erase(0, tagEnd + 1);

        std::smatch match;

        if (std::regex_search(currentTag, match, svgEndTagRegex))
        {
            level_loaded = true;
            map_file.close();
            Engine::GetLogger().LogEvent(file_path + "SVG.");
            return;
        }

        if (std::regex_search(currentTag, match, gEndTagRegex))
        {
            IsinG           = false;
            IsTranslate     = false;
            IsRotate        = false;
            IsScale         = false;
            translate       = { 0, 0 };
            rotateAngle     = 0;
            rotatetranslate = { 0, 0 };
            scale           = { 1.0f, 1.0f };
            return;
        }

        if (std::regex_search(currentTag, match, gIdRegex))
        {
            IsinG = true;
        }

        if (std::regex_search(currentTag, match, transformRegex))
        {
            std::string transformStr = match[1].str();
            if (std::regex_search(transformStr, match, matrixRegex))
            {
                IsScale = true;
                float a = std::stof(match[1].str());
                float b = std::stof(match[2].str());
                float c = std::stof(match[3].str());
                float d = std::stof(match[4].str());
                scale.x = std::sqrt(a * a + c * c);
                scale.y = std::sqrt(b * b + d * d);
            }
            else if (std::regex_search(transformStr, match, rotateRegex))
            {
                IsTranslate       = false;
                IsRotate          = true;
                rotateAngle       = -std::stof(match[1].str()) * static_cast<float>(M_PI) / 180.0f;
                rotatetranslate.x = std::stof(match[2].str());
                rotatetranslate.y = std::stof(match[3].str());
            }
            else if (std::regex_search(transformStr, match, translateRegex))
            {
                IsRotate    = false;
                IsTranslate = true;
                translate.x = std::stof(match[1].str());
                translate.y = std::stof(match[2].str());
            }
            return;
        }

        if (std::regex_search(currentTag, match, pathRegex))
        {
            std::string pathData = match[1].str();
            std::replace(pathData.begin(), pathData.end(), ' ', ',');
            std::vector<Math::vec2> positions = parsePathData(pathData);
            if (positions.empty())
                return;

            for (auto& vec : positions)
            {
                if (IsinG)
                {
                    if (IsScale)
                    {
                        vec.x *= scale.x;
                        vec.y *= scale.y;
                    }
                    if (IsRotate)
                    {
                        vec.x += rotatetranslate.x;
                        vec.y += rotatetranslate.y;
                        double rotatedX = vec.x * std::cos(rotateAngle) - vec.y * std::sin(rotateAngle);
                        double rotatedY = vec.x * std::sin(rotateAngle) + vec.y * std::cos(rotateAngle);
                        vec.x           = rotatedX;
                        vec.y           = rotatedY;
                    }
                    if (IsTranslate)
                    {
                        vec.x += translate.x;
                        vec.y += translate.y;
                    }
                }
                // vec.y = -vec.y;
            }

            if (std::regex_search(currentTag, match, fillColorRegex))
            {
                fillColor = match[1].str();
            }

            Polygon poly;
            poly.vertices    = positions;
            poly.vertexCount = static_cast<int>(positions.size());

            if (auto mapManager = Engine::GetGameStateManager().GetGSComponent<MapManager>())
            {
                mapManager->AddPolygon(poly);
            }

            Math::vec2 poly_center = poly.FindCenter();

            static bool first_path_logged = false;
            if (!first_path_logged)
            {
                Engine::GetLogger().LogEvent("" + std::to_string(poly_center.x) + ", " + std::to_string(poly_center.y));
                first_path_logged = true;
            }

            Polygon modified_poly = poly;
            for (auto& v : modified_poly.vertices)
            {
                v -= poly_center;
            }

            // Gate
            std::string objID = "";
            if (std::regex_search(currentTag, match, pathIdRegex))
            {
                objID = match[1].str();
            }

            CS230::GameObject* newObj = nullptr;

            if (objID.find("Gate") != std::string::npos)
            {
                Math::rect bounds   = modified_poly.FindBoundary();
                Math::vec2 gateSize = bounds.Size();

                newObj = new Gate(poly_center, gateSize);
                Engine::GetLogger().LogEvent("Created Gate at: " + std::to_string(poly_center.x) + ", " + std::to_string(poly_center.y));
            }
            else
            {
                if (auto mapManager = Engine::GetGameStateManager().GetGSComponent<MapManager>())
                {
                    mapManager->AddPolygon(poly);
                }
                newObj = new MapElement(poly_center, modified_poly);
            }

            if (newObj != nullptr)
            {
                Engine::GetGameStateManager().GetGSComponent<GameObjectManager>()->Add(newObj);
            }

            // MapElement* map_obj = new MapElement(poly_center, modified_poly);
            // Engine::GetGameStateManager().GetGSComponent<GameObjectManager>()->Add(map_obj);


            return;
        }
    }

    std::vector<Math::vec2> Map::parsePathData(const std::string& pathData)
    {
        std::istringstream      stream(pathData);
        std::string             data;
        float                   last_x = 0, last_y = 0;
        bool                    isRelative = false;
        std::vector<Math::vec2> positions;

        while (std::getline(stream, data, ','))
        {
            if (data.empty())
                continue;

            if (std::isalpha(data[0]))
            {
                currentCommand = data[0];
                isRelative     = std::islower(currentCommand);
                if (data.length() > 1)
                {
                    data = data.substr(1);
                }
                else
                {
                    continue;
                }
            }

            float x = 0.0f, y = 0.0f;

            try
            {
                if (currentCommand == 'm' || currentCommand == 'M' || currentCommand == 'l' || currentCommand == 'L')
                {
                    x = std::stof(data);
                    if (std::getline(stream, data, ','))
                    {
                        y      = std::stof(data);
                        last_x = isRelative ? last_x + x : x;
                        last_y = isRelative ? last_y + y : y;
                        positions.push_back({ last_x, last_y });
                        if (currentCommand == 'm' || currentCommand == 'M')
                        {
                            currentCommand = isRelative ? 'l' : 'L';
                        }
                    }
                }
                else if (currentCommand == 'v' || currentCommand == 'V')
                {
                    y      = std::stof(data);
                    last_y = isRelative ? last_y + y : y;
                    positions.push_back({ last_x, last_y });
                }
                else if (currentCommand == 'h' || currentCommand == 'H')
                {
                    x      = std::stof(data);
                    last_x = isRelative ? last_x + x : x;
                    positions.push_back({ last_x, last_y });
                }
                else if (currentCommand == 'z' || currentCommand == 'Z')
                {
                    if (!positions.empty())
                    {
                        positions.push_back(positions.front());
                    }
                }
            }
            catch (const std::exception& e)
            {
                Engine::GetLogger().LogError("SVG parsePathData: " + std::string(e.what()));
                continue;
            }
        }
        return positions;
    }
}