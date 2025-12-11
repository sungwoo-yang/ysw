#include "Game/MiniMap.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapManager.h"
#include "Engine/Window.hpp"
#include "Player.hpp"
#include <algorithm>
#include <imgui.h>
#include <utility>

namespace
{
    constexpr double kEpsilon = 1e-5;
}

MiniMap::MiniMap()
    : worldBounds(
          {
              { 0.0, 0.0 },
              { 1.0, 1.0 }
}),
      windowTitle("Minimap"), player(nullptr), camera(nullptr), mapManager(nullptr), visible(true)
{
}

void MiniMap::SetWorldBounds(Math::rect bounds)
{
    worldBounds = bounds;
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

void MiniMap::SetStyle(const MiniMapStyle& style_config)
{
    style = style_config;
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

void MiniMap::DrawImGui()
{
    if (!visible || player == nullptr || camera == nullptr)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

    ImVec2 targetSize;
    ImVec2 targetPos;

    if (currentMode == MiniMapMode::Full)
    {
        targetSize = ImVec2(1280.0f, 720.0f);
        targetPos  = ImVec2((1600.0f - 1280.0f) * 0.5f, (900.0f - 720.0f) * 0.5f);
    }
    else
    {
        targetSize = ImVec2(static_cast<float>(style.canvasSize.x) + 20.0f, static_cast<float>(style.canvasSize.y) + 20.0f);
        targetPos  = ImVec2(20.0f, 20.0f);
    }

    ImGui::SetNextWindowPos(targetPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(targetSize, ImGuiCond_Always);

    if (ImGui::Begin(windowTitle.c_str(), nullptr, window_flags))
    {
        ImVec2 canvas_min = ImGui::GetCursorScreenPos();
        ImVec2 canvas_max = ImVec2(
            canvas_min.x + static_cast<float>(currentMode == MiniMapMode::Full ? 1280.0f : style.canvasSize.x),
            canvas_min.y + static_cast<float>(currentMode == MiniMapMode::Full ? 720.0f : style.canvasSize.y));

        if (currentMode == MiniMapMode::Full)
        {
            canvas_max = ImVec2(canvas_min.x + ImGui::GetContentRegionAvail().x, canvas_min.y + ImGui::GetContentRegionAvail().y);
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        if (currentMode == MiniMapMode::Full)
        {
            ImGui::InvisibleButton("MapDragZone", ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y));

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
            {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                fullMapCamPos.x -= delta.x / fullMapScale;
                fullMapCamPos.y += delta.y / fullMapScale;
            }
        }

        draw_list->AddRectFilled(canvas_min, canvas_max, IM_COL32(18, 21, 29, 255));
        draw_list->AddRect(canvas_min, canvas_max, IM_COL32(255, 255, 255, 200), 4.0f, 0, 2.0f);

        draw_list->PushClipRect(canvas_min, canvas_max, true);

        if (style.showGrid)
            DrawGrid(draw_list, canvas_min, canvas_max);
        DrawLevelBounds(draw_list, canvas_min, canvas_max);
        DrawTerrainPolygons(draw_list, canvas_min, canvas_max);
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
        const double level_width  = std::max(worldBounds.Right() - worldBounds.Left(), kEpsilon);
        const double level_height = std::max(worldBounds.Top() - worldBounds.Bottom(), kEpsilon);

        double u = (world_position.x - worldBounds.Left()) / level_width;
        double v = (world_position.y - worldBounds.Bottom()) / level_height;

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

    Math::vec2 bl = WorldToMapCanvas({ worldBounds.Left(), worldBounds.Bottom() }, size);
    Math::vec2 tr = WorldToMapCanvas({ worldBounds.Right(), worldBounds.Top() }, size);
    Math::vec2 tl = WorldToMapCanvas({ worldBounds.Left(), worldBounds.Top() }, size);
    Math::vec2 br = WorldToMapCanvas({ worldBounds.Right(), worldBounds.Bottom() }, size);

    if (currentMode == MiniMapMode::Mini)
    {
        draw_list->AddRect(canvas_min, canvas_max, IM_COL32(150, 150, 150, 255));
    }
    else
    {
        ImVec2 p1 = ImVec2(canvas_min.x + (float)tl.x, canvas_min.y + (float)tl.y);
        ImVec2 p2 = ImVec2(canvas_min.x + (float)tr.x, canvas_min.y + (float)tr.y);
        ImVec2 p3 = ImVec2(canvas_min.x + (float)br.x, canvas_min.y + (float)br.y);
        ImVec2 p4 = ImVec2(canvas_min.x + (float)bl.x, canvas_min.y + (float)bl.y);

        draw_list->AddQuad(p1, p2, p3, p4, IM_COL32(150, 150, 150, 255));
    }
}

void MiniMap::DrawCameraFrustum(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    if (camera == nullptr)
        return;

    ImVec2 canvas_size = ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y);

    const Math::vec2 camera_pos = camera->GetPosition();
    Math::vec2       camera_size;
    {
        const Math::ivec2 window_px = Engine::GetWindow().GetSize();
        camera_size                 = { static_cast<double>(window_px.x), static_cast<double>(window_px.y) };
    }
    Math::vec2 bl = camera_pos;
    Math::vec2 tr = camera_pos + camera_size;

    Math::vec2 p_bl = WorldToMapCanvas(bl, canvas_size);
    Math::vec2 p_tr = WorldToMapCanvas(tr, canvas_size);

    ImVec2 rect_min(canvas_min.x + static_cast<float>(p_bl.x), canvas_min.y + static_cast<float>(p_tr.y));

    Math::vec2 w_tl = { bl.x, tr.y };
    Math::vec2 w_br = { tr.x, bl.y };

    Math::vec2 s_tl = WorldToMapCanvas(w_tl, canvas_size);
    Math::vec2 s_br = WorldToMapCanvas(w_br, canvas_size);

    draw_list->AddRect(
        ImVec2(canvas_min.x + (float)s_tl.x, canvas_min.y + (float)s_tl.y), ImVec2(canvas_min.x + (float)s_br.x, canvas_min.y + (float)s_br.y), IM_COL32(255, 255, 0, 220), 2.0f, 0,
        style.cameraLineWidth);
}

void MiniMap::DrawPlayerMarker(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    if (player == nullptr)
        return;

    ImVec2     canvas_size = ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y);
    Math::vec2 pos         = WorldToMapCanvas(player->GetPosition(), canvas_size);

    const float px = canvas_min.x + static_cast<float>(pos.x);
    const float py = canvas_min.y + static_cast<float>(pos.y);

    draw_list->AddCircleFilled(ImVec2(px, py), style.playerMarkerRadius, IM_COL32(0, 220, 130, 255));
    draw_list->AddCircle(ImVec2(px, py), style.playerMarkerRadius + 1.5f, IM_COL32(0, 0, 0, 180));
}

void MiniMap::DrawTerrainPolygons(ImDrawList* draw_list, const ImVec2& canvas_min, const ImVec2& canvas_max) const
{
    if (!style.showTerrain || mapManager == nullptr)
        return;

    const auto& polygons = mapManager->GetMiniMapPolygons();
    if (polygons.empty())
        return;

    ImVec2 canvas_size = ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y);

    for (const Polygon& polygon : polygons)
    {
        const size_t vertex_count = polygon.vertices.size();
        if (vertex_count < 2)
            continue;

        for (size_t i = 0; i < vertex_count; ++i)
        {
            const size_t next_index = (i + 1) % vertex_count;
            Math::vec2   p1         = WorldToMapCanvas(polygon.vertices[i], canvas_size);
            Math::vec2   p2         = WorldToMapCanvas(polygon.vertices[next_index], canvas_size);

            ImVec2 v1(canvas_min.x + static_cast<float>(p1.x), canvas_min.y + static_cast<float>(p1.y));
            ImVec2 v2(canvas_min.x + static_cast<float>(p2.x), canvas_min.y + static_cast<float>(p2.y));

            draw_list->AddLine(v1, v2, IM_COL32(120, 200, 255, 190), style.terrainLineWidth);
        }
    }
}
