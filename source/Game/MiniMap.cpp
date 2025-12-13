#include "Game/MiniMap.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapManager.h"
#include "Engine/Window.hpp"
#include "Player.hpp"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <string>
#include <utility>

namespace
{
    constexpr double kEpsilon = 1e-5;
}

MiniMap::MiniMap()
    : worldBounds(
          {
              { -1000.0, -1000.0 },
              {  1000.0,  1000.0 }
}),
      windowTitle("Minimap"), player(nullptr), camera(nullptr), mapManager(nullptr), visible(true)
{
    ResetFog();
}

void MiniMap::Update([[maybe_unused]] double dt)
{
    if (!visible || player == nullptr)
        return;

    if (style.enableFog)
    {
        UpdateFogVisibility();
    }
}

void MiniMap::SetWorldBounds(Math::rect bounds)
{
    worldBounds = bounds;
    if (style.enableFog)
    {
        ResizeFogGrid();
    }
}

void MiniMap::AttachPlayer(Player* player_ptr)
{
    player = player_ptr;
}

void MiniMap::AttachCamera(CS230::Camera* camera_ptr)
{
    camera = camera_ptr;
}

void MiniMap::AttachMapManager(CS230::MapManager* map_manager_ptr)
{
    mapManager = map_manager_ptr;
}

void MiniMap::AttachGameObjectManager(CS230::GameObjectManager* gom_ptr)
{
    gameObjectManager = gom_ptr;
}

void MiniMap::SetStyle(const MiniMapStyle& style_config)
{
    style = style_config;
    ResizeFogGrid();
}

void MiniMap::SetWindowTitle(std::string title_text)
{
    windowTitle = std::move(title_text);
}

void MiniMap::SetVisible(bool enabled)
{
    visible = enabled;
}

bool MiniMap::IsVisible() const
{
    return visible;
}

void MiniMap::SetMode(MiniMapMode new_mode)
{
    currentMode = new_mode;
    if (currentMode == MiniMapMode::Full && player != nullptr)
    {
        fullMapCamPos = player->GetPosition();
    }
}

MiniMapMode MiniMap::GetMode() const
{
    return currentMode;
}

void MiniMap::ToggleMode()
{
    SetMode(currentMode == MiniMapMode::Mini ? MiniMapMode::Full : MiniMapMode::Mini);
}

void MiniMap::ResetFog()
{
    ResizeFogGrid();
}

void MiniMap::ResizeFogGrid()
{
    if (style.fogTileSize <= kEpsilon)
        style.fogTileSize = 50.0f;

    double width  = worldBounds.Right() - worldBounds.Left();
    double height = worldBounds.Top() - worldBounds.Bottom();

    if (width <= 0 || height <= 0)
        return;

    fogCols = static_cast<int>(std::ceil(width / style.fogTileSize));
    fogRows = static_cast<int>(std::ceil(height / style.fogTileSize));

    fogVisited.assign(fogRows, std::vector<bool>(fogCols, false));
}

void MiniMap::UpdateFogVisibility()
{
    if (player == nullptr || fogVisited.empty())
        return;

    Math::vec2 playerPos = player->GetPosition();

    if (playerPos.x < worldBounds.Left() || playerPos.x > worldBounds.Right() || playerPos.y < worldBounds.Bottom() || playerPos.y > worldBounds.Top())
        return;

    int centerX = static_cast<int>((playerPos.x - worldBounds.Left()) / style.fogTileSize);
    int centerY = static_cast<int>((playerPos.y - worldBounds.Bottom()) / style.fogTileSize);

    int radiusGrid = static_cast<int>(std::ceil(style.visionRadius / style.fogTileSize));

    for (int y = centerY - radiusGrid; y <= centerY + radiusGrid; ++y)
    {
        for (int x = centerX - radiusGrid; x <= centerX + radiusGrid; ++x)
        {
            if (y >= 0 && y < fogRows && x >= 0 && x < fogCols)
            {
                double distSq = std::pow(x - centerX, 2) + std::pow(y - centerY, 2);
                if (distSq <= std::pow(radiusGrid, 2))
                {
                    fogVisited[y][x] = true;
                }
            }
        }
    }
}

void MiniMap::DrawImGui()
{
    if (!visible || player == nullptr || camera == nullptr)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

    ImVec2 targetSize, targetPos;
    ImVec2 viewportPos  = ImGui::GetMainViewport()->Pos;
    ImVec2 viewportSize = ImGui::GetMainViewport()->Size;

    if (currentMode == MiniMapMode::Full)
    {
        targetSize = ImVec2(1280.0f, 720.0f);
        targetPos  = ImVec2(viewportPos.x + (viewportSize.x - 1280.0f) * 0.5f, viewportPos.y + (viewportSize.y - 720.0f) * 0.5f);
    }
    else
    {
        targetSize = ImVec2(static_cast<float>(style.canvasSize.x) + 20.0f, static_cast<float>(style.canvasSize.y) + 20.0f);
        targetPos  = ImVec2(viewportPos.x + 20.0f, viewportPos.y + 20.0f);
    }

    ImGui::SetNextWindowPos(targetPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(targetSize, ImGuiCond_Always);

    if (ImGui::Begin(windowTitle.c_str(), nullptr, window_flags))
    {
        ImVec2 canvas_min = ImGui::GetCursorScreenPos();
        ImVec2 canvas_max = ImVec2(
            canvas_min.x + (currentMode == MiniMapMode::Full ? 1280.0f : static_cast<float>(style.canvasSize.x)),
            canvas_min.y + (currentMode == MiniMapMode::Full ? 720.0f : static_cast<float>(style.canvasSize.y)));

        if (currentMode == MiniMapMode::Full)
        {
            canvas_max = ImVec2(canvas_min.x + ImGui::GetContentRegionAvail().x, canvas_min.y + ImGui::GetContentRegionAvail().y);

            ImGui::InvisibleButton("MapDragZone", ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y));
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
            {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                fullMapCamPos.x -= delta.x / fullMapScale;
                fullMapCamPos.y += delta.y / fullMapScale;
            }
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->AddRectFilled(canvas_min, canvas_max, IM_COL32(18, 21, 29, 255));
        draw_list->AddRect(canvas_min, canvas_max, IM_COL32(255, 255, 255, 200), 4.0f, 0, 2.0f);

        draw_list->PushClipRect(canvas_min, canvas_max, true);

        if (style.showGrid)
            DrawGrid(draw_list, canvas_min, canvas_max);
        DrawLevelBounds(draw_list, canvas_min, canvas_max);
        DrawTerrainPolygons(draw_list, canvas_min, canvas_max);
        DrawGameObjects(draw_list, canvas_min, canvas_max);

        if (currentMode == MiniMapMode::Full && style.enableFog)
        {
            DrawFog(draw_list, canvas_min, canvas_max);
        }

        DrawPlayerMarker(draw_list, canvas_min, canvas_max);
        if (style.showCameraFrustum)
            DrawCameraFrustum(draw_list, canvas_min, canvas_max);

        draw_list->PopClipRect();
    }
    ImGui::End();
}

Math::vec2 MiniMap::WorldToMapCanvas(const Math::vec2& world_position, const struct ImVec2& canvas_size) const
{
    if (currentMode == MiniMapMode::Mini)
    {
        if (!camera)
            return { 0.0, 0.0 };

        Math::vec2  camPos  = camera->GetPosition();
        Math::ivec2 winSize = Engine::GetWindow().GetSize();
        Math::vec2  camSize = { static_cast<double>(winSize.x), static_cast<double>(winSize.y) };

        double relX = world_position.x - camPos.x;
        double relY = world_position.y - camPos.y;

        double u = relX / camSize.x;
        double v = relY / camSize.y;

        return { u * canvas_size.x, (1.0 - v) * canvas_size.y };
    }
    else
    {
        double centerX = canvas_size.x * 0.5;
        double centerY = canvas_size.y * 0.5;

        double dx = (world_position.x - fullMapCamPos.x) * fullMapScale;
        double dy = (world_position.y - fullMapCamPos.y) * fullMapScale;

        return { centerX + dx, centerY - dy };
    }
}

void MiniMap::DrawGrid(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    if (style.gridDivisions <= 1.0 || currentMode == MiniMapMode::Full)
        return;

    const float width  = canvas_max.x - canvas_min.x;
    const float height = canvas_max.y - canvas_min.y;
    const int   lines  = static_cast<int>(style.gridDivisions);

    for (int i = 1; i < lines; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(lines);
        const float x = canvas_min.x + width * t;
        draw_list->AddLine(ImVec2(x, canvas_min.y), ImVec2(x, canvas_max.y), IM_COL32(70, 70, 70, 180));

        const float y = canvas_min.y + height * t;
        draw_list->AddLine(ImVec2(canvas_min.x, y), ImVec2(canvas_max.x, y), IM_COL32(70, 70, 70, 180));
    }
}

void MiniMap::DrawLevelBounds(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    ImVec2 size = ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y);

    Math::vec2 tl = WorldToMapCanvas({ worldBounds.Left(), worldBounds.Top() }, size);
    Math::vec2 br = WorldToMapCanvas({ worldBounds.Right(), worldBounds.Bottom() }, size);

    if (currentMode == MiniMapMode::Mini)
    {
        ImVec2 p1(canvas_min.x + (float)tl.x, canvas_min.y + (float)tl.y);
        ImVec2 p3(canvas_min.x + (float)br.x, canvas_min.y + (float)br.y);
        draw_list->AddRect(p1, p3, IM_COL32(150, 150, 150, 255));
    }
    else
    {
        Math::vec2 tr = WorldToMapCanvas({ worldBounds.Right(), worldBounds.Top() }, size);
        Math::vec2 bl = WorldToMapCanvas({ worldBounds.Left(), worldBounds.Bottom() }, size);

        ImVec2 p1(canvas_min.x + (float)tl.x, canvas_min.y + (float)tl.y);
        ImVec2 p2(canvas_min.x + (float)tr.x, canvas_min.y + (float)tr.y);
        ImVec2 p3(canvas_min.x + (float)br.x, canvas_min.y + (float)br.y);
        ImVec2 p4(canvas_min.x + (float)bl.x, canvas_min.y + (float)bl.y);
        draw_list->AddQuad(p1, p2, p3, p4, IM_COL32(150, 150, 150, 255));
    }
}

void MiniMap::DrawCameraFrustum(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    if (!camera)
        return;
    ImVec2      canvas_size = ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y);
    Math::vec2  camPos      = camera->GetPosition();
    Math::ivec2 winSize     = Engine::GetWindow().GetSize();
    Math::vec2  camSize     = { (double)winSize.x, (double)winSize.y };

    Math::vec2 tl = WorldToMapCanvas({ camPos.x, camPos.y + camSize.y }, canvas_size);
    Math::vec2 br = WorldToMapCanvas({ camPos.x + camSize.x, camPos.y }, canvas_size);

    draw_list->AddRect(
        ImVec2(canvas_min.x + (float)tl.x, canvas_min.y + (float)tl.y), ImVec2(canvas_min.x + (float)br.x, canvas_min.y + (float)br.y), IM_COL32(255, 255, 0, 220), 2.0f, 0, style.cameraLineWidth);
}

void MiniMap::DrawPlayerMarker(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    if (!player)
        return;
    ImVec2     canvas_size = ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y);
    Math::vec2 pos         = WorldToMapCanvas(player->GetPosition(), canvas_size);

    draw_list->AddCircleFilled(ImVec2(canvas_min.x + (float)pos.x, canvas_min.y + (float)pos.y), style.playerMarkerRadius, IM_COL32(0, 220, 130, 255));
}

void MiniMap::DrawTerrainPolygons(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    if (!style.showTerrain || !mapManager)
        return;
    ImVec2 canvas_size = ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y);

    for (const auto& poly : mapManager->GetMiniMapPolygons())
    {
        if (poly.vertices.size() < 2)
            continue;
        for (size_t i = 0; i < poly.vertices.size(); ++i)
        {
            Math::vec2 p1 = WorldToMapCanvas(poly.vertices[i], canvas_size);
            Math::vec2 p2 = WorldToMapCanvas(poly.vertices[(i + 1) % poly.vertices.size()], canvas_size);

            draw_list->AddLine(
                ImVec2(canvas_min.x + (float)p1.x, canvas_min.y + (float)p1.y), ImVec2(canvas_min.x + (float)p2.x, canvas_min.y + (float)p2.y), IM_COL32(120, 200, 255, 190), style.terrainLineWidth);
        }
    }
}

void MiniMap::DrawGameObjects(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    if (!gameObjectManager)
        return;
    ImVec2 canvas_size = ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y);

    for (CS230::GameObject* obj : gameObjectManager->GetObjects())
    {
        if (obj->Type() == GameObjectTypes::Player || obj->Type() == GameObjectTypes::Floor || obj->Type() == GameObjectTypes::Particle)
            continue;

        if (currentMode == MiniMapMode::Full && style.enableFog && !fogVisited.empty())
        {
            int gridX = static_cast<int>((obj->GetPosition().x - worldBounds.Left()) / style.fogTileSize);
            int gridY = static_cast<int>((obj->GetPosition().y - worldBounds.Bottom()) / style.fogTileSize);

            if (gridY >= 0 && gridY < fogRows && gridX >= 0 && gridX < fogCols)
            {
                if (!fogVisited[gridY][gridX])
                    continue;
            }
        }

        Math::vec2 pos = WorldToMapCanvas(obj->GetPosition(), canvas_size);
        float      px  = canvas_min.x + (float)pos.x;
        float      py  = canvas_min.y + (float)pos.y;

        if (px < canvas_min.x || px > canvas_max.x || py < canvas_min.y || py > canvas_max.y)
            continue;

        switch (obj->Type())
        {
            case GameObjectTypes::Bonfire: draw_list->AddTriangleFilled(ImVec2(px, py - 4), ImVec2(px - 4, py + 4), ImVec2(px + 4, py + 4), IM_COL32(255, 140, 0, 255)); break;
            case GameObjectTypes::Door: draw_list->AddRectFilled(ImVec2(px - 3, py - 5), ImVec2(px + 3, py + 5), IM_COL32(139, 69, 19, 255)); break;
            case GameObjectTypes::Mirror:
            case GameObjectTypes::PushableMirror: draw_list->AddCircleFilled(ImVec2(px, py), 3.0f, IM_COL32(0, 255, 255, 255)); break;
            case GameObjectTypes::Star: draw_list->AddCircleFilled(ImVec2(px, py), 3.0f, IM_COL32(255, 0, 0, 255)); break;
            case GameObjectTypes::Target: draw_list->AddCircleFilled(ImVec2(px, py), 3.0f, IM_COL32(255, 215, 0, 255)); break;
            case GameObjectTypes::Sign: draw_list->AddCircleFilled(ImVec2(px, py), 2.0f, IM_COL32(200, 200, 200, 255)); break;
            default: draw_list->AddCircleFilled(ImVec2(px, py), 2.0f, IM_COL32(255, 255, 255, 255)); break;
        }
    }
}

void MiniMap::DrawFog(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    if (fogVisited.empty())
        return;

    ImVec2 canvas_size = ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y);
    ImU32  fogColor    = IM_COL32(0, 0, 0, static_cast<int>(style.fogOpacity * 255.0f));

    for (int y = 0; y < fogRows; ++y)
    {
        for (int x = 0; x < fogCols; ++x)
        {
            if (fogVisited[y][x])
                continue;

            double wx1 = worldBounds.Left() + x * style.fogTileSize;
            double wy1 = worldBounds.Bottom() + y * style.fogTileSize;
            double wx2 = wx1 + style.fogTileSize;
            double wy2 = wy1 + style.fogTileSize;

            Math::vec2 p1 = WorldToMapCanvas({ wx1, wy1 }, canvas_size);
            Math::vec2 p2 = WorldToMapCanvas({ wx2, wy2 }, canvas_size);

            float minX = canvas_min.x + (float)std::min(p1.x, p2.x);
            float minY = canvas_min.y + (float)std::min(p1.y, p2.y);
            float maxX = canvas_min.x + (float)std::max(p1.x, p2.x);
            float maxY = canvas_min.y + (float)std::max(p1.y, p2.y);

            if (maxX < canvas_min.x || minX > canvas_max.x || maxY < canvas_min.y || minY > canvas_max.y)
                continue;

            draw_list->AddRectFilled(ImVec2(minX, minY), ImVec2(maxX + 1.0f, maxY + 1.0f), fogColor);
        }
    }
}