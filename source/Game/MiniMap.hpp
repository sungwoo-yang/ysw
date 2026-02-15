#pragma once

#include "Engine/Rect.hpp"
#include "Engine/Vec2.hpp"
#include <string>
#include <vector>

class Player;

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

    bool  enableFog    = true;     
    double fogTileSize  = 50.0;    
    float visionRadius = 400.0f;   
    float fogOpacity   = 1.0f;
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

    // Attach system components
    void SetWorldBounds(Math::rect bounds);
    void AttachPlayer(Player* player_ptr);
    void AttachCamera(CS230::Camera* camera_ptr);
    void AttachMapManager(CS230::MapManager* map_manager_ptr);
    void AttachGameObjectManager(CS230::GameObjectManager* gom_ptr);

    void SetStyle(const MiniMapStyle& style_config);
    void SetWindowTitle(std::string title_text);
    void SetVisible(bool enabled);

    [[nodiscard]] bool IsVisible() const;

    void        SetMode(MiniMapMode new_mode);
    [[nodiscard]] MiniMapMode GetMode() const;
    void        ToggleMode();

    void ResetFog();
    void DrawImGui();

private:
    // World to Canvas Conversion
    Math::vec2 WorldToMapCanvas(const Math::vec2& world_position, const struct ImVec2& canvas_size) const;
    
    // Rendering Passes
    void DrawGrid(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawLevelBounds(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawCameraFrustum(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawPlayerMarker(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawTerrainPolygons(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    void DrawGameObjects(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;
    
    // Fog of War
    void ResizeFogGrid();
    void UpdateFogVisibility();
    void DrawFog(struct ImDrawList* draw_list, const struct ImVec2& canvas_min, const struct ImVec2& canvas_max) const;

    Math::rect                worldBounds;
    MiniMapStyle              style;
    std::string               windowTitle;
    Player* player;
    CS230::Camera* camera;
    CS230::MapManager* mapManager;
    CS230::GameObjectManager* gameObjectManager = nullptr;
    bool                      visible;

    MiniMapMode currentMode   = MiniMapMode::Mini;
    Math::vec2  fullMapCamPos = { 0.0, 0.0 };
    double      fullMapScale  = 0.5;
    
    std::vector<std::vector<bool>> fogVisited; 
    int                            fogRows = 0;
    int                            fogCols = 0;
};