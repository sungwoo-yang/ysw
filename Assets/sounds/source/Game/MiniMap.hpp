#pragma once

#include "Engine/Rect.hpp"
#include "Engine/Vec2.hpp"
#include <string>
#include <vector>

class Player;

// Forward declarations to minimize header dependencies
namespace CS230
{
    class Camera;
    class MapManager;
    class GameObjectManager;
}

// Minimap Visual Style Configuration
struct MiniMapStyle
{
    Math::vec2 canvasSize{ 220.0, 220.0 };
    Math::vec2 canvasMargin{ 16.0, 16.0 };
    Math::vec2 cameraViewSize{ 800.0, 600.0 };
    double     gridDivisions      = 4.0;
    float      playerMarkerRadius = 4.0f;
    float      cameraLineWidth    = 1.5f;
    bool       showCameraFrustum  = true;
    bool       showGrid           = true;
    bool       showTerrain        = true;
    float      terrainLineWidth   = 1.2f;

    bool   enableFog    = true;
    double fogTileSize  = 50.0;
    float  visionRadius = 400.0f;
    float  fogOpacity   = 1.0f;
};

enum class MiniMapMode
{
    Mini,
    Full
};

class MiniMap
{
public:
    MiniMap();

    void Update(double dt);
    void DrawImGui();

    // System attachment methods for data synchronization
    void AttachPlayer(Player* player_ptr);
    void AttachCamera(CS230::Camera* camera_ptr);
    void AttachMapManager(CS230::MapManager* map_manager_ptr);
    void AttachGameObjectManager(CS230::GameObjectManager* gom_ptr);

    void SetWorldBounds(Math::rect bounds);
    void SetStyle(const MiniMapStyle& style_config);
    void SetWindowTitle(std::string title_text);

    // Visibility and mode control logic
    void        SetVisible(bool enabled);
    bool        IsVisible() const;
    void        SetMode(MiniMapMode new_mode);
    MiniMapMode GetMode() const;
    void        ToggleMode();

    void ResetFog();

private:
    // Internal coordinate transformation: World Space -> UI Canvas Space
    Math::vec2 WorldToMapCanvas(const Math::vec2& world_position, const struct ImVec2& canvas_size) const;

    // Layered rendering helper methods
    void DrawGrid(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawLevelBounds(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawCameraFrustum(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawPlayerMarker(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawTerrainPolygons(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawGameObjects(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawFog(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;

    // Fog of War management
    void ResizeFogGrid();
    void UpdateFogVisibility();

    Math::rect  worldBounds;
    std::string windowTitle;

    // Pointers to required engine systems
    Player*                   player;
    CS230::Camera*            camera;
    CS230::MapManager*        mapManager;
    CS230::GameObjectManager* gameObjectManager = nullptr;

    MiniMapStyle style;
    MiniMapMode  currentMode = MiniMapMode::Mini;
    bool         visible;

    // Fog of war state tracking
    std::vector<std::vector<bool>> fogVisited;
    int                            fogRows = 0;
    int                            fogCols = 0;

    // Full map navigation state
    Math::vec2 fullMapCamPos = { 0.0, 0.0 };
    double     fullMapScale  = 0.5;
};