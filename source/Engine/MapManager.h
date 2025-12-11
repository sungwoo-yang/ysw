#pragma once

#include "Engine/Component.hpp"
#include "Engine/Polygon.h"
#include "Engine/Vec2.hpp"
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

namespace CS230
{
    class Map;

    class MapManager : public Component
    {
    public:
        MapManager() : currentMapIndex(0)
        {
        }

        ~MapManager();

        void AddMap(Map* new_map);
        void LoadMap();
        void Unload();

        void Update([[maybe_unused]] double dt) override;
        Map* GetCurrentMap();

        const std::vector<Polygon>& GetMiniMapPolygons() const
        {
            return miniMapPolygons;
        }

        void AddPolygon(const Polygon& poly)
        {
            miniMapPolygons.push_back(poly);
        }

    private:
        std::vector<Map*>    maps;
        int                  currentMapIndex;
        std::vector<Polygon> miniMapPolygons;
    };

    class Map : public Component
    {
    public:
        Map(const std::string& filename);
        ~Map();

        void OpenSVG();
        void ParseSVG();

        bool IsLevelLoaded() const
        {
            return level_loaded;
        }

    private:
        std::vector<Math::vec2> parsePathData(const std::string& pathData);

        std::ifstream map_file;
        std::string   file_path;
        bool          level_loaded   = false;
        char          currentCommand = '\0';
        std::string   currentTagBuffer;

        std::regex pathRegex;
        std::regex gIdRegex;
        std::regex transformRegex;
        std::regex translateRegex;
        std::regex rotateRegex;
        std::regex matrixRegex;
        std::regex pathIdRegex;
        std::regex fillColorRegex;
        std::regex gEndTagRegex;
        std::regex svgEndTagRegex;

        Math::vec2  translate       = { 0, 0 };
        float       rotateAngle     = 0;
        Math::vec2  rotatetranslate = { 0, 0 };
        Math::vec2  scale           = { 1.0f, 1.0f };
        std::string fillColor       = "#00000000";
        bool        IsinG           = false;
        bool        IsTranslate     = false;
        bool        IsRotate        = false;
        bool        IsScale         = false;
    };
}