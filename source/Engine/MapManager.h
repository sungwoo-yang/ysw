#pragma once

#include "Engine/Component.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Polygon.h"
#include "Engine/Rect.hpp"
#include "Engine/Vec2.hpp"

#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

namespace CS230
{
    class GameObject;
    class Map;

    // Component responsible for managing multiple levels (Maps), handling transitions,
    // and storing global map data such as collision polygons for the MiniMap.
    class MapManager : public Component
    {
    public:
        using GameObjectFactory = std::function<CS230::GameObject*(GameObjectTypes, Math::vec2, const std::string&, const std::string&, const Polygon&)>;

        MapManager() : currentMapIndex(0)
        {
        }

        ~MapManager();

        void SetGameObjectFactory(GameObjectFactory factory)
        {
            objectFactory = factory;
        }

        GameObjectFactory GetGameObjectFactory() const
        {
            return objectFactory;
        }

        void AddMap(Map* new_map);
        void LoadMap();
        void Unload();

        // Progressively processes map loading to prevent blocking the main thread
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

        // Returns the room that contains playerPos, or nullopt if none
        std::optional<Math::rect> GetCurrentRoom(Math::vec2 playerPos) const;

        // All room bounds in the map
        const std::vector<Math::rect>& GetAllRooms() const;

        // Index of the room active last frame (for neighbour checks)
        int GetCurrentRoomIndex(Math::vec2 playerPos) const;

    private:
        std::vector<Map*>    maps;
        int                  currentMapIndex;
        std::vector<Polygon> miniMapPolygons; // Aggregated geometry for UI rendering

        GameObjectFactory objectFactory = nullptr;
    };

    // Represents a single playable level parsed from an SVG file.
    // Handles the conversion of 2D vector graphic paths into physical game geometry.
    class Map : public Component
    {
    public:
        Map(const std::string& filename);
        ~Map();

        // Opens the file stream for the SVG map asset
        void OpenSVG();

        // Iteratively reads and parses SVG tags, generating terrain and entities
        void ParseSVG();

        bool IsLevelLoaded() const
        {
            return level_loaded;
        }

        const std::vector<Math::rect>& GetRoomBounds() const
        {
            return roomBounds;
        }

    private:
        // Parses the "d" attribute of an SVG <path> into a list of 2D vertices
        std::vector<Math::vec2> parsePathData(const std::string& pathData);

        std::ifstream map_file;
        std::string   file_path;
        bool          level_loaded   = false;
        char          currentCommand = '\0';
        std::string   currentTagBuffer;

        // Precompiled regular expressions for extracting SVG attributes
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

        // SVG Group (<g>) transform states applied to child paths
        Math::vec2  translate       = { 0, 0 };
        float       rotateAngle     = 0;
        Math::vec2  rotatetranslate = { 0, 0 };
        Math::vec2  scale           = { 1.0, 1.0 };
        std::string fillColor       = "#00000000";

        // Parsing state flags
        bool IsinG       = false;
        bool IsTranslate = false;
        bool IsRotate    = false;
        bool IsScale     = false;

        // All room boundaries parsed from <rect fill="#ffffff"> in SVG
        std::vector<Math::rect> roomBounds;
    };
}