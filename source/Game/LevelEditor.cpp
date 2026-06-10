#include "LevelEditor.hpp"
#include "Mode3.hpp"
#include "ScriptManager.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "CS200/RGBA.hpp"

#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Window.hpp"

#include "Engine/Path.hpp"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <regex>
#include <sstream>

// ---------------------------------------------------------------------------
// Static storage
// ---------------------------------------------------------------------------

CS230::GameObjectManager* LevelEditor::s_gom           = nullptr;
Math::vec2                LevelEditor::s_spawnPos      = { 0.0, 0.0 };
Math::vec2                LevelEditor::s_savedCamPos   = { 0.0, 0.0 };
double                    LevelEditor::s_savedZoom     = 0.0;
Math::vec2                LevelEditor::s_gameReturnPos = { 0.0, 0.0 };

// ---------------------------------------------------------------------------
// Anonymous helpers
// ---------------------------------------------------------------------------

namespace
{
    constexpr CS200::RGBA COL_FLOOR      = 0x00FFFFFF;
    constexpr CS200::RGBA COL_PLATFORM   = 0xFFFF00FF;
    constexpr CS200::RGBA COL_SPECIAL    = 0xFF0000FF;
    constexpr CS200::RGBA COL_ROOMBOUNDS = 0xFFFFFFFF;
    constexpr CS200::RGBA COL_TARGETSTAR = 0xFFD700FF;
    constexpr CS200::RGBA COL_LASERSTAR  = 0x39FF14FF;
    constexpr CS200::RGBA COL_BOSSSTAR   = 0xFF44AAFF;
    constexpr CS200::RGBA COL_TURRET     = 0xFF6600FF;
    constexpr CS200::RGBA COL_BLOCK      = 0xCC00CCFF;
    constexpr CS200::RGBA COL_ROPE       = 0x00CC00FF;
    constexpr CS200::RGBA COL_WATER      = 0x0044CCFF;
    constexpr CS200::RGBA COL_BREAKWALL  = 0x884400FF;
    constexpr CS200::RGBA COL_STAIRCASE  = 0xAA6600FF;
    constexpr CS200::RGBA COL_SPIKE      = 0xCC2222FF;
    constexpr CS200::RGBA COL_ELEVATOR   = 0x4488CCFF;

    CS200::RGBA TypeColor(ObjectType t)
    {
        switch (t)
        {
            case ObjectType::Floor: return COL_FLOOR;
            case ObjectType::Platform: return COL_PLATFORM;
            case ObjectType::Special: return COL_SPECIAL;
            case ObjectType::RoomBounds: return COL_ROOMBOUNDS;
            case ObjectType::TargetStar: return COL_TARGETSTAR;
            case ObjectType::LaserStar: return COL_LASERSTAR;
            case ObjectType::BossStar: return COL_BOSSSTAR;
            case ObjectType::Turret: return COL_TURRET;
            case ObjectType::Block: return COL_BLOCK;
            case ObjectType::Rope: return COL_ROPE;
            case ObjectType::Water: return COL_WATER;
            case ObjectType::BreakableWall: return COL_BREAKWALL;
            case ObjectType::Staircase: return COL_STAIRCASE;
            case ObjectType::Spike: return COL_SPIKE;
            case ObjectType::Elevator: return COL_ELEVATOR;
        }
        return CS200::WHITE;
    }

    CS200::RGBA Dim(CS200::RGBA c)
    {
        return (c & 0xFFFFFF00u) | 0x80u;
    }

    // Ray-casting point-in-polygon test
    bool PointInPolygon(const std::vector<Math::vec2>& v, Math::vec2 p)
    {
        bool   inside = false;
        size_t n      = v.size();
        for (size_t i = 0, j = n - 1; i < n; j = i++)
        {
            if (((v[i].y > p.y) != (v[j].y > p.y)) && (p.x < (v[j].x - v[i].x) * (p.y - v[i].y) / (v[j].y - v[i].y) + v[i].x))
                inside = !inside;
        }
        return inside;
    }

    const char* TypeNames[] = { "Floor", "Platform", "Special", "RoomBounds",    "TargetStar", "LaserStar", "BossStar", "Turret",
                                "Block", "Rope",     "Water",   "BreakableWall", "Staircase",  "Spike",     "Elevator" };

    std::string MakeDefaultObjectId(ObjectType type, int index)
    {
        switch (type)
        {
            case ObjectType::TargetStar: return "TS_BOSS_" + std::to_string(index);

            case ObjectType::LaserStar: return "LS_Y_SHOT_TRACK_E_BOSS";

            case ObjectType::BossStar: return "BS_BOSS";

            default: return "";
        }
    }

    std::string ExtractBossGroupFromId(const std::string& id)
    {
        if (id.rfind("BS_", 0) == 0)
            return id.substr(3);

        return "BOSS";
    }

    std::string ExtractTargetGroupFromId(const std::string& id)
    {
        if (id.rfind("TS_", 0) != 0)
            return "BOSS";

        std::string  body           = id.substr(3);
        const size_t lastUnderscore = body.rfind('_');

        if (lastUnderscore == std::string::npos)
            return body;

        return body.substr(0, lastUnderscore);
    }

    int ExtractTargetIndexFromId(const std::string& id)
    {
        if (id.rfind("TS_", 0) != 0)
            return 0;

        const size_t lastUnderscore = id.rfind('_');
        if (lastUnderscore == std::string::npos)
            return 0;

        try
        {
            return std::max(0, std::stoi(id.substr(lastUnderscore + 1)));
        }
        catch (...)
        {
            return 0;
        }
    }

    const char* ModeName(int m)
    {
        switch (m)
        {
            case 0: return "View  [V]";
            case 1: return "Rect  [R]";
            case 2: return "Polygon  [P]";
            case 3: return "VertexEdit  [E]";
        }
        return "?";
    }

    // Must match Mode3::currentScale
    constexpr double GAME_SCALE = 0.8;

    // ---- Grid snap ----
    Math::vec2 SnapToGrid(Math::vec2 pos, double grid)
    {
        return { std::round(pos.x / grid) * grid, std::round(pos.y / grid) * grid };
    }

    // ---- SVG helpers ----

    const char* TypeFill(ObjectType t)
    {
        switch (t)
        {
            case ObjectType::Floor: return "#00ffff";
            case ObjectType::Platform: return "#ffff00";
            case ObjectType::Special: return "#ff0000";
            case ObjectType::RoomBounds: return "#ffffff";
            case ObjectType::TargetStar: return "#ffd700";
            case ObjectType::LaserStar: return "#39ff14";
            case ObjectType::BossStar: return "#ff44aa";
            case ObjectType::Turret: return "#ff6600";
            case ObjectType::Block: return "#cc00cc";
            case ObjectType::Rope: return "#00cc00";
            case ObjectType::Water: return "#0044cc";
            case ObjectType::BreakableWall: return "#884400";
            case ObjectType::Staircase: return "#aa6600";
            case ObjectType::Spike: return "#cc2222";
            case ObjectType::Elevator: return "#4488cc";
        }
        return "#888888";
    }

    ObjectType FillToType(const std::string& fill)
    {
        if (fill == "#00ffff" || fill == "#00FFFF")
            return ObjectType::Floor;
        if (fill == "#ffff00" || fill == "#FFFF00")
            return ObjectType::Platform;
        if (fill == "#ff0000" || fill == "#FF0000")
            return ObjectType::Special;
        if (fill == "#ffffff" || fill == "#FFFFFF")
            return ObjectType::RoomBounds;
        if (fill == "#ffd700" || fill == "#FFD700")
            return ObjectType::TargetStar;
        if (fill == "#39ff14" || fill == "#39FF14")
            return ObjectType::LaserStar;
        if (fill == "#ff44aa" || fill == "#FF44AA")
            return ObjectType::BossStar;
        if (fill == "#ff6600" || fill == "#FF6600")
            return ObjectType::Turret;
        if (fill == "#cc00cc" || fill == "#CC00CC")
            return ObjectType::Block;
        if (fill == "#00cc00" || fill == "#00CC00")
            return ObjectType::Rope;
        if (fill == "#0044cc" || fill == "#0044CC")
            return ObjectType::Water;
        if (fill == "#884400")
            return ObjectType::BreakableWall;
        if (fill == "#aa6600" || fill == "#AA6600")
            return ObjectType::Staircase;
        if (fill == "#cc2222" || fill == "#CC2222")
            return ObjectType::Spike;
        if (fill == "#4488cc" || fill == "#4488CC")
            return ObjectType::Elevator;
        return ObjectType::Floor;
    }

    // Parse "M x,y L x,y ... Z" — Y is already in SVG space, caller flips
    std::vector<Math::vec2> ParsePathD(const std::string& d)
    {
        std::vector<Math::vec2> verts;
        std::istringstream      ss(d);
        std::string             tok;

        while (ss >> tok)
        {
            if (tok == "Z" || tok == "z")
                continue;

            std::string coord;
            if (tok == "M" || tok == "m" || tok == "L" || tok == "l")
            {
                if (!(ss >> coord))
                    break;
            }
            else
            {
                coord = tok; // bare coordinate
            }

            const size_t comma = coord.find(',');
            if (comma == std::string::npos)
                continue;

            try
            {
                const double x = std::stod(coord.substr(0, comma));
                const double y = -std::stod(coord.substr(comma + 1)); // SVG→game Y-flip
                verts.push_back({ x, y });
            }
            catch (...)
            {
            }
        }
        return verts;
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void LevelEditor::Load()
{
    editorCamPos  = s_savedCamPos;
    editorZoom    = (s_savedZoom > 0.0) ? s_savedZoom : 1.0;
    gridSize      = 40.0;
    pendingWheel  = 0.0f;
    prevMousePos  = Engine::GetInput().GetMousePosition();
    currentMode   = EditorMode::View;
    isDrawingRect = false;
    polyInProgress.clear();
    selectedIndex      = -1;
    isDraggingObject   = false;
    placingSpawn       = false;
    isDraggingSpawn    = false;
    draggedVertexIndex = -1;
    prevDeleteDown     = false;
    prevTabDown        = true; // Tab was held to enter; skip first frame to avoid immediate exit
    undoStack.clear();
    showHotkeyHelp = false;

    // Auto-load previously saved map
    LoadSVG();
    LoadTriggers();
    scriptEditorMode = false;
    selectedTrigger  = -1;
    placingTrigger   = false;
    selectedEvent    = -1;
    timelineViewEnd  = 10.0;
    editEvents.clear();
    statusMessage.clear();
    statusTimer = 0.0;
}

void LevelEditor::Unload()
{
    s_savedCamPos = editorCamPos;
    s_savedZoom   = editorZoom;
    s_gom         = nullptr;
}

// ---------------------------------------------------------------------------
// Camera math
// ---------------------------------------------------------------------------

Math::TransformationMatrix LevelEditor::GetViewMatrix() const
{
    const Math::ivec2 win    = Engine::GetWindow().GetSize();
    const Math::vec2  half   = { win.x * 0.5, win.y * 0.5 };
    const Math::vec2  origin = editorCamPos - half * (1.0 / editorZoom);
    return Math::ScaleMatrix(editorZoom) * Math::TranslationMatrix(-origin);
}

Math::vec2 LevelEditor::ScreenToWorld(Math::vec2 sp) const
{
    const Math::ivec2 win  = Engine::GetWindow().GetSize();
    const Math::vec2  half = { win.x * 0.5, win.y * 0.5 };
    const Math::vec2  fp   = { sp.x, static_cast<double>(win.y) - sp.y };
    return editorCamPos + (fp - half) * (1.0 / editorZoom);
}

Math::vec2 LevelEditor::WorldToScreen(Math::vec2 wp) const
{
    const Math::ivec2 win    = Engine::GetWindow().GetSize();
    const Math::vec2  half   = { win.x * 0.5, win.y * 0.5 };
    const Math::vec2  offset = (wp - editorCamPos) * editorZoom;
    const Math::vec2  fp     = offset + half;
    return { fp.x, static_cast<double>(win.y) - fp.y };
}

// ---------------------------------------------------------------------------
// Camera input
// ---------------------------------------------------------------------------

void LevelEditor::HandleCamera()
{
    auto&            input    = Engine::GetInput();
    const Math::vec2 mousePos = input.GetMousePosition();

    if (pendingWheel != 0.0f)
    {
        const Math::vec2 anchor = ScreenToWorld(mousePos);
        const double     factor = (pendingWheel > 0.0f) ? 1.12 : (1.0 / 1.12);
        editorZoom              = std::clamp(editorZoom * factor, 0.1, 10.0);

        const Math::ivec2 win  = Engine::GetWindow().GetSize();
        const Math::vec2  half = { win.x * 0.5, win.y * 0.5 };
        const Math::vec2  fp   = { mousePos.x, static_cast<double>(win.y) - mousePos.y };
        editorCamPos           = anchor - (fp - half) * (1.0 / editorZoom);
        pendingWheel           = 0.0f;
    }

    if (input.MouseButtonDown(CS230::Input::MouseButton::Middle))
    {
        const Math::vec2 delta = mousePos - prevMousePos;
        editorCamPos.x -= delta.x / editorZoom;
        editorCamPos.y += delta.y / editorZoom;

        if (SDL_GetModState() & KMOD_CTRL)
        {
            editorCamPos.x = std::round(editorCamPos.x / gridSize) * gridSize;
            editorCamPos.y = std::round(editorCamPos.y / gridSize) * gridSize;
        }
    }

    // Arrow keys: move exactly one viewport in that direction
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        const Math::ivec2 win   = Engine::GetWindow().GetSize();
        const double      stepX = win.x / editorZoom;
        const double      stepY = win.y / editorZoom;

        if (input.KeyJustPressed(CS230::Input::Keys::Right))
            editorCamPos.x += stepX;
        if (input.KeyJustPressed(CS230::Input::Keys::Left))
            editorCamPos.x -= stepX;
        if (input.KeyJustPressed(CS230::Input::Keys::Up))
            editorCamPos.y += stepY;
        if (input.KeyJustPressed(CS230::Input::Keys::Down))
            editorCamPos.y -= stepY;
    }

    prevMousePos = mousePos;
}

// ---------------------------------------------------------------------------
// Grid
// ---------------------------------------------------------------------------

void LevelEditor::DrawGrid(CS200::IRenderer2D& renderer, Math::ivec2 winSize)
{
    constexpr CS200::RGBA FINE   = 0x2A2A2AFF;
    constexpr CS200::RGBA COARSE = 0x404040FF;
    constexpr CS200::RGBA AXIS   = 0x606060FF;

    if (gridSize * editorZoom < 3.0)
        return;

    const Math::vec2 tl = ScreenToWorld({ 0.0, 0.0 });
    const Math::vec2 br = ScreenToWorld({ static_cast<double>(winSize.x), static_cast<double>(winSize.y) });

    const double xMin = std::min(tl.x, br.x), xMax = std::max(tl.x, br.x);
    const double yMin = std::min(tl.y, br.y), yMax = std::max(tl.y, br.y);

    const long ixMin = static_cast<long>(std::floor(xMin / gridSize));
    const long ixMax = static_cast<long>(std::ceil(xMax / gridSize));
    const long iyMin = static_cast<long>(std::floor(yMin / gridSize));
    const long iyMax = static_cast<long>(std::ceil(yMax / gridSize));

    for (long i = ixMin; i <= ixMax; ++i)
    {
        const double      x = i * gridSize;
        const CS200::RGBA c = (i == 0) ? AXIS : ((i % 8 == 0) ? COARSE : FINE);
        renderer.DrawLine({ x, yMin }, { x, yMax }, c, 1.0);
    }
    for (long i = iyMin; i <= iyMax; ++i)
    {
        const double      y = i * gridSize;
        const CS200::RGBA c = (i == 0) ? AXIS : ((i % 8 == 0) ? COARSE : FINE);
        renderer.DrawLine({ xMin, y }, { xMax, y }, c, 1.0);
    }
}

// ---------------------------------------------------------------------------
// Mode switching + Delete
// ---------------------------------------------------------------------------

void LevelEditor::UpdateModeSwitch()
{
    auto& input = Engine::GetInput();
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    // V → View mode (always safe to call)
    if (input.KeyJustPressed(CS230::Input::Keys::V))
    {
        currentMode     = EditorMode::View;
        isDrawingRect   = false;
        placingSpawn    = false;
        isDraggingSpawn = false;
        polyInProgress.clear();
        draggedVertexIndex = -1;
    }

    // R → toggle Rect
    if (input.KeyJustPressed(CS230::Input::Keys::R))
    {
        currentMode     = (currentMode == EditorMode::Rect) ? EditorMode::View : EditorMode::Rect;
        isDrawingRect   = false;
        placingSpawn    = false;
        isDraggingSpawn = false;
    }

    // P → toggle Polygon
    if (input.KeyJustPressed(CS230::Input::Keys::P))
    {
        if (currentMode == EditorMode::Polygon)
        {
            currentMode     = EditorMode::View;
            placingSpawn    = false;
            isDraggingSpawn = false;
            polyInProgress.clear();
        }
        else
        {
            currentMode     = EditorMode::Polygon;
            placingSpawn    = false;
            isDraggingSpawn = false;
        }
    }

    // E → toggle VertexEdit (only if something selected)
    if (input.KeyJustPressed(CS230::Input::Keys::E))
    {
        if (currentMode == EditorMode::VertexEdit)
        {
            currentMode        = EditorMode::View;
            placingSpawn       = false;
            isDraggingSpawn    = false;
            draggedVertexIndex = -1;
        }
        else if (selectedIndex >= 0 && selectedIndex < static_cast<int>(objects.size()))
        {
            currentMode     = EditorMode::VertexEdit;
            placingSpawn    = false;
            isDraggingSpawn = false;
        }
    }

    // Escape
    if (input.KeyJustPressed(CS230::Input::Keys::Escape))
    {
        if (currentMode == EditorMode::View)
        {
            selectedIndex = -1;
        }
        else
        {
            currentMode     = EditorMode::View;
            isDrawingRect   = false;
            placingSpawn    = false;
            isDraggingSpawn = false;
            polyInProgress.clear();
            draggedVertexIndex = -1;
        }
    }

    // Delete (SDL only, engine Input has no Delete key)
    const Uint8* sdlKeys   = SDL_GetKeyboardState(nullptr);
    const bool   deleteNow = sdlKeys[SDL_SCANCODE_DELETE] != 0;
    if (deleteNow && !prevDeleteDown && selectedIndex >= 0 && selectedIndex < static_cast<int>(objects.size()))
    {
        PushUndo();
        objects.erase(objects.begin() + selectedIndex);
        selectedIndex      = -1;
        draggedVertexIndex = -1;
        currentMode        = EditorMode::View;
    }
    prevDeleteDown = deleteNow;
}

// ---------------------------------------------------------------------------
// Object creation (Rect / Polygon)
// ---------------------------------------------------------------------------

void LevelEditor::UpdateObjectCreation()
{
    if (currentMode != EditorMode::Rect && currentMode != EditorMode::Polygon)
        return;

    auto&            input      = Engine::GetInput();
    const bool       imguiMouse = ImGui::GetIO().WantCaptureMouse;
    const Math::vec2 mouseWorld = ScreenToWorld(input.GetMousePosition());

    // ---- Rect ----
    if (currentMode == EditorMode::Rect)
    {
        if (!imguiMouse && input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
        {
            isDrawingRect = true;
            rectStart     = SnapToGrid(mouseWorld, gridSize);
        }

        if (isDrawingRect && input.MouseButtonJustReleased(CS230::Input::MouseButton::Left))
        {
            const Math::vec2 snapped = SnapToGrid(mouseWorld, gridSize);
            const double     x0      = std::min(rectStart.x, snapped.x);
            const double     x1      = std::max(rectStart.x, snapped.x);
            const double     y0      = std::min(rectStart.y, snapped.y);
            const double     y1      = std::max(rectStart.y, snapped.y);

            if (x1 - x0 > 4.0 && y1 - y0 > 4.0)
            {
                PushUndo();
                EditorObject obj;
                obj.isRect   = true;
                obj.type     = currentType;
                obj.vertices = {
                    { x0, y0 },
                    { x1, y0 },
                    { x1, y1 },
                    { x0, y1 }
                };
                obj.id = MakeDefaultObjectId(currentType, static_cast<int>(objects.size()));
                objects.push_back(std::move(obj));
            }
            isDrawingRect = false;
        }
    }

    // ---- Polygon ----
    if (currentMode == EditorMode::Polygon)
    {
        if (!imguiMouse && input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
            polyInProgress.push_back(SnapToGrid(mouseWorld, gridSize));

        if (input.KeyJustPressed(CS230::Input::Keys::Enter) && polyInProgress.size() >= 3)
        {
            PushUndo();
            EditorObject obj;
            obj.isRect   = false;
            obj.type     = currentType;
            obj.vertices = polyInProgress;
            obj.id       = MakeDefaultObjectId(currentType, static_cast<int>(objects.size()));
            objects.push_back(std::move(obj));
            polyInProgress.clear();
        }
    }
}

// ---------------------------------------------------------------------------
// Selection, movement, vertex editing
// ---------------------------------------------------------------------------

void LevelEditor::UpdateSelectionAndEdit()
{
    if (currentMode != EditorMode::View && currentMode != EditorMode::VertexEdit)
        return;

    auto&            input      = Engine::GetInput();
    const bool       imguiMouse = ImGui::GetIO().WantCaptureMouse;
    const Math::vec2 mouseWorld = ScreenToWorld(input.GetMousePosition());

    // ---- View mode: select + move ----
    if (currentMode == EditorMode::View)
    {
        if (!imguiMouse && input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
        {
            // If clicking inside the already-selected object, start dragging it
            if (selectedIndex >= 0 && PointInPolygon(objects[selectedIndex].vertices, mouseWorld))
            {
                PushUndo();
                isDraggingObject = true;
                dragLastWorld    = mouseWorld;
            }
            else
            {
                // Try to find a new object under the cursor (top-most first)
                selectedIndex = -1;
                for (int i = static_cast<int>(objects.size()) - 1; i >= 0; --i)
                {
                    if (PointInPolygon(objects[i].vertices, mouseWorld))
                    {
                        selectedIndex = i;
                        PushUndo();
                        isDraggingObject = true;
                        dragLastWorld    = mouseWorld;
                        break;
                    }
                }
            }
        }

        // Drag selected object
        if (isDraggingObject && input.MouseButtonDown(CS230::Input::MouseButton::Left) && selectedIndex >= 0)
        {
            const Math::vec2 delta = mouseWorld - dragLastWorld;
            for (auto& v : objects[selectedIndex].vertices)
                v += delta;
            dragLastWorld = mouseWorld;
        }

        if (input.MouseButtonJustReleased(CS230::Input::MouseButton::Left))
            isDraggingObject = false;
    }

    // ---- VertexEdit mode ----
    if (currentMode == EditorMode::VertexEdit && selectedIndex >= 0 && selectedIndex < static_cast<int>(objects.size()))
    {
        constexpr double HIT_R = 8.0;
        auto&            verts = objects[selectedIndex].vertices;

        if (!imguiMouse && input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
        {
            draggedVertexIndex = -1;
            for (int i = 0; i < static_cast<int>(verts.size()); ++i)
            {
                const Math::vec2 diff = verts[i] - mouseWorld;
                if (diff.LengthSquared() <= HIT_R * HIT_R)
                {
                    PushUndo();
                    draggedVertexIndex = i;
                    break;
                }
            }
        }

        if (draggedVertexIndex >= 0 && input.MouseButtonDown(CS230::Input::MouseButton::Left))
        {
            verts[draggedVertexIndex] = mouseWorld;
        }

        if (input.MouseButtonJustReleased(CS230::Input::MouseButton::Left))
            draggedVertexIndex = -1;
    }
}

// ---------------------------------------------------------------------------
// Dashed line helper
// ---------------------------------------------------------------------------

void LevelEditor::DrawDashedLine(CS200::IRenderer2D& renderer, Math::vec2 from, Math::vec2 to, uint32_t color, double width)
{
    constexpr double DASH = 10.0, GAP = 7.0;
    const Math::vec2 diff = to - from;
    const double     len  = diff.Length();
    if (len < 0.001)
        return;
    const Math::vec2 dir = diff * (1.0 / len);
    for (double t = 0.0; t < len;)
    {
        const double end = std::min(t + DASH, len);
        renderer.DrawLine(from + dir * t, from + dir * end, color, width);
        t = end + GAP;
    }
}

// ---------------------------------------------------------------------------
// Draw objects
// ---------------------------------------------------------------------------

void LevelEditor::DrawObjects(CS200::IRenderer2D& renderer)
{
    constexpr double VERT_R   = 5.0; // normal vertex circle radius
    constexpr double HANDLE_R = 8.0; // vertex edit handle radius

    // ---- Draw all finished objects ----
    for (int idx = 0; idx < static_cast<int>(objects.size()); ++idx)
    {
        const auto&       obj   = objects[idx];
        const CS200::RGBA col   = TypeColor(obj.type);
        const auto&       verts = obj.vertices;

        // Edges
        for (size_t i = 0; i < verts.size(); ++i)
        {
            const size_t next = (i + 1) % verts.size();
            renderer.DrawLine(verts[i], verts[next], col, 1.5);
        }

        // Vertex dots
        for (const auto& v : verts)
        {
            const auto t = Math::TranslationMatrix(v) * Math::ScaleMatrix(VERT_R);
            renderer.DrawCircle(t, CS200::CLEAR, col, 1.0);
        }
    }

    // ---- Selected object: white outline overlay ----
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(objects.size()))
    {
        const auto& sel   = objects[selectedIndex];
        const auto& verts = sel.vertices;

        for (size_t i = 0; i < verts.size(); ++i)
        {
            const size_t next = (i + 1) % verts.size();
            renderer.DrawLine(verts[i], verts[next], CS200::WHITE, 2.5);
        }

        // ---- VertexEdit handles ----
        if (currentMode == EditorMode::VertexEdit)
        {
            for (int i = 0; i < static_cast<int>(verts.size()); ++i)
            {
                const bool        dragging = (i == draggedVertexIndex);
                const CS200::RGBA hcol     = dragging ? 0xFFFF00FF : CS200::WHITE;
                const CS200::RGBA fill     = dragging ? 0xFFFF0040u : CS200::CLEAR;
                const auto        t        = Math::TranslationMatrix(verts[i]) * Math::ScaleMatrix(HANDLE_R * 2.0);
                renderer.DrawCircle(t, fill, hcol, 2.0);
            }
        }
    }

    // ---- Rect preview ----
    const Math::vec2  mouseWorld   = ScreenToWorld(Engine::GetInput().GetMousePosition());
    const Math::vec2  snappedMouse = SnapToGrid(mouseWorld, gridSize);
    const CS200::RGBA col          = TypeColor(currentType);
    const CS200::RGBA dimCol       = Dim(col);

    // ---- Spawn position marker (player bounding box) ----
    {
        constexpr CS200::RGBA SPAWN_COL  = 0xFF8800FF; // orange
        constexpr CS200::RGBA SPAWN_FILL = 0xFF880040u;
        constexpr double      HW         = 20.0; // player half-width
        constexpr double      HH         = 40.0; // player half-height
        const Math::vec2&     sp         = s_spawnPos;
        const auto            spawnBox   = Math::TranslationMatrix(sp) * Math::ScaleMatrix({ HW * 2.0, HH * 2.0 });

        // Player collision rect
        if (placingSpawn || isDraggingSpawn)
            renderer.DrawRectangle(spawnBox, SPAWN_FILL, CS200::CLEAR, 0.0);
        renderer.DrawLine({ sp.x - HW, sp.y - HH }, { sp.x + HW, sp.y - HH }, SPAWN_COL, 2.0);
        renderer.DrawLine({ sp.x + HW, sp.y - HH }, { sp.x + HW, sp.y + HH }, SPAWN_COL, 2.0);
        renderer.DrawLine({ sp.x + HW, sp.y + HH }, { sp.x - HW, sp.y + HH }, SPAWN_COL, 2.0);
        renderer.DrawLine({ sp.x - HW, sp.y + HH }, { sp.x - HW, sp.y - HH }, SPAWN_COL, 2.0);
        // Center crosshair
        renderer.DrawLine({ sp.x - HW * 0.5, sp.y }, { sp.x + HW * 0.5, sp.y }, SPAWN_COL, 1.5);
        renderer.DrawLine({ sp.x, sp.y - HH * 0.5 }, { sp.x, sp.y + HH * 0.5 }, SPAWN_COL, 1.5);

        if (placingSpawn && !isDraggingSpawn)
        {
            const auto previewBox = Math::TranslationMatrix(snappedMouse) * Math::ScaleMatrix({ HW * 2.0, HH * 2.0 });
            renderer.DrawRectangle(previewBox, SPAWN_FILL, SPAWN_COL, 1.5);
        }
    }

    if (currentMode == EditorMode::Rect && isDrawingRect)
    {
        const double x0 = std::min(rectStart.x, snappedMouse.x);
        const double x1 = std::max(rectStart.x, snappedMouse.x);
        const double y0 = std::min(rectStart.y, snappedMouse.y);
        const double y1 = std::max(rectStart.y, snappedMouse.y);

        renderer.DrawLine({ x0, y0 }, { x1, y0 }, col, 1.5);
        renderer.DrawLine({ x1, y0 }, { x1, y1 }, col, 1.5);
        renderer.DrawLine({ x1, y1 }, { x0, y1 }, col, 1.5);
        renderer.DrawLine({ x0, y1 }, { x0, y0 }, col, 1.5);

        for (const Math::vec2 c : {
                 Math::vec2{ x0, y0 },
                 { x1, y0 },
                 { x1, y1 },
                 { x0, y1 }
        })
        {
            const auto t = Math::TranslationMatrix(c) * Math::ScaleMatrix(VERT_R);
            renderer.DrawCircle(t, CS200::CLEAR, col, 1.0);
        }
    }

    // ---- Game viewport indicator (dashed rectangle) ----
    {
        constexpr CS200::RGBA VP_COL = 0x00FF88FF; // bright green
        const Math::ivec2     win    = Engine::GetWindow().GetSize();
        const double          hw     = (win.x / GAME_SCALE) * 0.5;
        const double          hh     = (win.y / GAME_SCALE) * 0.5;
        const Math::vec2&     c      = editorCamPos;

        DrawDashedLine(renderer, { c.x - hw, c.y - hh }, { c.x + hw, c.y - hh }, VP_COL, 1.0);
        DrawDashedLine(renderer, { c.x + hw, c.y - hh }, { c.x + hw, c.y + hh }, VP_COL, 1.0);
        DrawDashedLine(renderer, { c.x + hw, c.y + hh }, { c.x - hw, c.y + hh }, VP_COL, 1.0);
        DrawDashedLine(renderer, { c.x - hw, c.y + hh }, { c.x - hw, c.y - hh }, VP_COL, 1.0);
    }

    // ---- Polygon in-progress ----
    if (currentMode == EditorMode::Polygon && !polyInProgress.empty())
    {
        for (size_t i = 0; i + 1 < polyInProgress.size(); ++i)
            renderer.DrawLine(polyInProgress[i], polyInProgress[i + 1], col, 1.5);

        // Preview lines use snapped position so user sees exactly where vertex lands
        renderer.DrawLine(polyInProgress.back(), snappedMouse, dimCol, 1.0);

        if (polyInProgress.size() >= 2)
            DrawDashedLine(renderer, snappedMouse, polyInProgress.front(), dimCol, 1.0);

        // Snap indicator dot at cursor
        {
            const auto t = Math::TranslationMatrix(snappedMouse) * Math::ScaleMatrix(6.0);
            renderer.DrawCircle(t, CS200::CLEAR, col, 2.0);
        }

        for (const auto& v : polyInProgress)
        {
            const auto t = Math::TranslationMatrix(v) * Math::ScaleMatrix(VERT_R);
            renderer.DrawCircle(t, CS200::CLEAR, col, 1.0);
        }

        // Highlight first vertex (close hint)
        const auto t = Math::TranslationMatrix(polyInProgress.front()) * Math::ScaleMatrix(VERT_R * 2.2);
        renderer.DrawCircle(t, CS200::CLEAR, dimCol, 1.0);
    }
}

// ---------------------------------------------------------------------------
// Spawn marker editing
// ---------------------------------------------------------------------------

bool LevelEditor::UpdateSpawnEditing()
{
    auto&            input      = Engine::GetInput();
    const bool       imguiMouse = ImGui::GetIO().WantCaptureMouse;
    const Math::vec2 mouseWorld = ScreenToWorld(input.GetMousePosition());
    const Math::vec2 snapped    = SnapToGrid(mouseWorld, gridSize);

    constexpr double SPAWN_PICK_RADIUS = 54.0;
    const bool       mouseOnSpawn      = (mouseWorld - s_spawnPos).LengthSquared() <= SPAWN_PICK_RADIUS * SPAWN_PICK_RADIUS;

    bool consumed = false;

    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        if (input.KeyJustPressed(CS230::Input::Keys::G))
        {
            placingSpawn    = !placingSpawn;
            isDraggingSpawn = false;
            currentMode     = EditorMode::View;
            isDrawingRect   = false;
            polyInProgress.clear();
            draggedVertexIndex = -1;
            consumed           = true;
        }

        if (input.KeyJustPressed(CS230::Input::Keys::Escape) && placingSpawn)
        {
            placingSpawn    = false;
            isDraggingSpawn = false;
            consumed        = true;
        }
    }

    if (imguiMouse)
    {
        if (!input.MouseButtonDown(CS230::Input::MouseButton::Left))
            isDraggingSpawn = false;
        return consumed || placingSpawn || isDraggingSpawn;
    }

    if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        if (placingSpawn || (currentMode == EditorMode::View && mouseOnSpawn))
        {
            s_spawnPos      = snapped;
            isDraggingSpawn = true;
            currentMode     = EditorMode::View;
            selectedIndex   = -1;
            consumed        = true;
        }
    }

    if (isDraggingSpawn)
    {
        if (input.MouseButtonDown(CS230::Input::MouseButton::Left))
        {
            s_spawnPos = snapped;
            consumed   = true;
        }
        else
        {
            isDraggingSpawn = false;
            placingSpawn    = false;
            consumed        = true;
        }
    }

    return consumed || placingSpawn || isDraggingSpawn;
}

// ---------------------------------------------------------------------------
// SVG Save
// ---------------------------------------------------------------------------

void LevelEditor::SaveSVG()
{
    // Build output path: <project_root>/Assets/maps/editor_output.svg
    const auto outPath = (assets::get_base_path() / "Assets/maps/editor_output.svg").string();

    std::ofstream f(outPath);
    if (!f.is_open())
    {
        statusMessage = "Save FAILED: " + outPath;
        statusTimer   = 2.0;
        return;
    }

    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    f << "<svg xmlns=\"http://www.w3.org/2000/svg\">\n";

    for (const auto& obj : objects)
    {
        if (obj.vertices.empty())
            continue;

        if (obj.type == ObjectType::RoomBounds)
        {
            // Compute AABB from vertices
            double minX = obj.vertices[0].x, maxX = minX;
            double minY = obj.vertices[0].y, maxY = minY;
            for (const auto& v : obj.vertices)
            {
                minX = std::min(minX, v.x);
                maxX = std::max(maxX, v.x);
                minY = std::min(minY, v.y);
                maxY = std::max(maxY, v.y);
            }
            // SVG rect: x=left, y_svg_top = -gameMaxY, w=width, h=height
            f << "  <rect style=\"fill:#ffffff;\""
              << " x=\"" << minX << "\""
              << " y=\"" << -maxY << "\""
              << " width=\"" << (maxX - minX) << "\""
              << " height=\"" << (maxY - minY) << "\"/>\n";
        }
        else
        {
            f << "  <path style=\"fill:" << TypeFill(obj.type) << ";\" d=\"";
            for (size_t i = 0; i < obj.vertices.size(); ++i)
            {
                const double svgY = -obj.vertices[i].y; // game→SVG Y-flip
                if (i == 0)
                    f << "M " << obj.vertices[i].x << "," << svgY;
                else
                    f << " L " << obj.vertices[i].x << "," << svgY;
            }
            if (!obj.id.empty())
                f << " Z\" id=\"" << obj.id << "\"/>\n";
            else
                f << " Z\"/>\n";
        }
    }

    // Save spawn position as a distinct orange circle
    f << "  <circle cx=\"" << s_spawnPos.x << "\" cy=\"" << -s_spawnPos.y << "\" r=\"20\" fill=\"#ff8800\" id=\"PlayerSpawn\"/>\n";

    f << "</svg>\n";

    statusMessage = "Saved (" + std::to_string(objects.size()) + " objects)";
    statusTimer   = 2.0;
}

// ---------------------------------------------------------------------------
// SVG Load
// ---------------------------------------------------------------------------

void LevelEditor::LoadSVG()
{
    std::string filePath;
    try
    {
        filePath = assets::locate_asset("Assets/maps/editor_output.svg").string();
    }
    catch (...)
    {
        statusMessage = "Load FAILED: editor_output.svg not found";
        statusTimer   = 2.0;
        return;
    }

    std::ifstream f(filePath);
    if (!f.is_open())
    {
        statusMessage = "Load FAILED: " + filePath;
        statusTimer   = 2.0;
        return;
    }

    // Read entire file, flatten newlines for single-pass regex matching
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::replace(content.begin(), content.end(), '\n', ' ');
    std::replace(content.begin(), content.end(), '\r', ' ');

    // Precompiled attribute regexes (xxx delimiter avoids )" clash)
    static const std::regex rTag(R"(<(path|rect)[^>]*>)", std::regex::icase);
    static const std::regex rFillCSS(R"xxx(fill:\s*(#[0-9a-fA-F]{6}))xxx");
    static const std::regex rFillAttr(R"xxx(fill\s*=\s*"([^"]*)")xxx");
    static const std::regex rPathD(R"xxx(\bd\s*=\s*"([^"]*)")xxx");
    static const std::regex rTagId(R"xxx(\bid\s*=\s*"([^"]+)")xxx");
    static const std::regex rRectX(R"xxx(\bx\s*=\s*"([^"]+)")xxx");
    static const std::regex rRectY(R"xxx(\by\s*=\s*"([^"]+)")xxx");
    static const std::regex rRectW(R"xxx(\bwidth\s*=\s*"([^"]+)")xxx");
    static const std::regex rRectH(R"xxx(\bheight\s*=\s*"([^"]+)")xxx");

    objects.clear();
    selectedIndex = -1;

    auto it  = std::sregex_iterator(content.begin(), content.end(), rTag);
    auto end = std::sregex_iterator{};

    for (; it != end; ++it)
    {
        const std::string tag     = (*it)[0].str();
        const std::string tagName = (*it)[1].str(); // "path" or "rect"

        std::smatch m;
        std::string fill;
        // Try CSS style first ("fill:#rrggbb;" inside style="..."), then XML attribute
        if (std::regex_search(tag, m, rFillCSS))
            fill = m[1].str();
        else if (std::regex_search(tag, m, rFillAttr))
            fill = m[1].str();

        if (tagName == "path" || tagName == "PATH")
        {
            std::string d;
            if (std::regex_search(tag, m, rPathD))
                d = m[1].str();

            if (d.empty())
                continue;

            auto verts = ParsePathD(d);
            if (verts.size() < 3)
                continue;

            std::string objId;
            if (std::regex_search(tag, m, rTagId))
                objId = m[1].str();

            EditorObject obj;
            obj.isRect   = false;
            obj.type     = FillToType(fill);
            obj.vertices = std::move(verts);
            obj.id       = objId;
            objects.push_back(std::move(obj));
        }
        else if (tagName == "rect" || tagName == "RECT")
        {
            if (fill != "#ffffff" && fill != "#FFFFFF")
                continue;

            double rx = 0, ry = 0, rw = 0, rh = 0;
            try
            {
                if (std::regex_search(tag, m, rRectX))
                    rx = std::stod(m[1].str());
                if (std::regex_search(tag, m, rRectY))
                    ry = std::stod(m[1].str());
                if (std::regex_search(tag, m, rRectW))
                    rw = std::stod(m[1].str());
                if (std::regex_search(tag, m, rRectH))
                    rh = std::stod(m[1].str());
            }
            catch (...)
            {
                continue;
            }

            // SVG rect: x=left, y=top_svg=-gameMaxY  → gameMaxY = -ry
            const double gameMinX = rx, gameMaxX = rx + rw;
            const double gameMaxY = -ry, gameMinY = -(ry + rh);

            EditorObject obj;
            obj.isRect   = true;
            obj.type     = ObjectType::RoomBounds;
            obj.vertices = {
                { gameMinX, gameMinY },
                { gameMaxX, gameMinY },
                { gameMaxX, gameMaxY },
                { gameMinX, gameMaxY }
            };
            objects.push_back(std::move(obj));
        }
    }

    // Load spawn position from orange circle element
    {
        static const std::regex rSpawnCircle(R"xxx(<circle\s[^/]*/?>)xxx", std::regex::icase);
        static const std::regex rSpawnFill(R"xxx(fill\s*=\s*"#ff8800")xxx", std::regex::icase);
        static const std::regex rSpawnId(R"xxx(\bid\s*=\s*"PlayerSpawn")xxx", std::regex::icase);
        static const std::regex rCX(R"xxx(\bcx\s*=\s*"([^"]+)")xxx");
        static const std::regex rCY(R"xxx(\bcy\s*=\s*"([^"]+)")xxx");

        auto cit = std::sregex_iterator(content.begin(), content.end(), rSpawnCircle);
        for (; cit != std::sregex_iterator{}; ++cit)
        {
            const std::string ctag = (*cit)[0].str();
            if (!std::regex_search(ctag, rSpawnFill) && !std::regex_search(ctag, rSpawnId))
                continue;
            std::smatch cm;
            if (std::regex_search(ctag, cm, rCX))
                try
                {
                    s_spawnPos.x = std::stod(cm[1].str());
                }
                catch (...)
                {
                }
            if (std::regex_search(ctag, cm, rCY))
                try
                {
                    s_spawnPos.y = -std::stod(cm[1].str());
                }
                catch (...)
                {
                }
            break;
        }
    }

    statusMessage = "Loaded " + std::to_string(objects.size()) + " objects";
    statusTimer   = 2.0;
}

// ---------------------------------------------------------------------------
// Ctrl+S / Ctrl+O detection
// ---------------------------------------------------------------------------

void LevelEditor::UpdateSaveLoad()
{
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    const bool ctrl  = (SDL_GetModState() & KMOD_CTRL) != 0;
    auto&      input = Engine::GetInput();

    if (ctrl && input.KeyJustPressed(CS230::Input::Keys::S))
        SaveSVG();

    if (ctrl && input.KeyJustPressed(CS230::Input::Keys::O))
        LoadSVG();

    // Ctrl+Z: undo
    if (ctrl && input.KeyJustPressed(CS230::Input::Keys::Z))
    {
        if (!undoStack.empty())
        {
            objects = undoStack.back();
            undoStack.pop_back();
            selectedIndex      = -1;
            draggedVertexIndex = -1;
        }
        return;
    }

    // Ctrl+D: duplicate selected
    if (ctrl && input.KeyJustPressed(CS230::Input::Keys::D))
    {
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(objects.size()))
        {
            PushUndo();
            EditorObject copy = objects[selectedIndex];
            for (auto& v : copy.vertices)
            {
                v.x += gridSize;
                v.y -= gridSize;
            }
            objects.push_back(std::move(copy));
            selectedIndex = static_cast<int>(objects.size()) - 1;
        }
        return;
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void LevelEditor::Update(double dt)
{
    HandleCamera();
    UpdateModeSwitch();
    if (!scriptEditorMode)
    {
        const bool spawnInputConsumed = UpdateSpawnEditing();
        if (!spawnInputConsumed)
        {
            UpdateObjectCreation();
            UpdateSelectionAndEdit();
        }
    }
    UpdateTriggerEditing();
    UpdateSaveLoad();

    // Tab: auto-save and switch to game
    {
        const Uint8* sdlKeys = SDL_GetKeyboardState(nullptr);
        const bool   tabNow  = sdlKeys[SDL_SCANCODE_TAB] != 0;
        if (tabNow && !prevTabDown && !ImGui::GetIO().WantCaptureKeyboard)
        {
            SaveSVG();
            Mode3::SetReturnPosition(s_spawnPos);
            Engine::GetGameStateManager().ChangeStateWithFade<Mode3>();
        }
        prevTabDown = tabNow;
    }

    if (statusTimer > 0.0)
        statusTimer -= dt;
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void LevelEditor::Draw()
{
    const Math::ivec2 win = Engine::GetWindow().GetSize();
    auto&             r   = Engine::GetRenderer2D();

    Engine::GetWindow().Clear(0x111118FF);

    const Math::TransformationMatrix vp = CS200::build_ndc_matrix(win) * GetViewMatrix();

    r.BeginScene(vp);

    if (s_gom != nullptr)
        s_gom->DrawAll(vp);

    DrawGrid(r, win);
    DrawObjects(r);
    if (scriptEditorMode)
        DrawTriggers(r);

    r.EndScene();
}

// ---------------------------------------------------------------------------
// DrawImGui
// ---------------------------------------------------------------------------

void LevelEditor::DrawImGui()
{
    pendingWheel += ImGui::GetIO().MouseWheel;

    ImGui::SetNextWindowPos({ 10.0f, 10.0f }, ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints({ 285.0f, 100.0f }, { 285.0f, 700.0f });
    ImGui::Begin("##editor_hud", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    // ---- Header with Script Editor toggle ----
    ImGui::TextColored({ 0.4f, 0.8f, 1.0f, 1.0f }, "[ LEVEL EDITOR ]");
    ImGui::SameLine(170.0f);
    ImGui::TextDisabled("Zoom %.2fx", editorZoom);

    // Script Editor toggle button
    {
        const bool wasScript = scriptEditorMode;
        if (wasScript)
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.6f, 0.3f, 0.8f, 1.0f });
        if (ImGui::Button(scriptEditorMode ? "Script Ed [ON]" : "Script Ed", { 265.0f, 0.0f }))
        {
            scriptEditorMode = !scriptEditorMode;
            if (!scriptEditorMode)
                placingTrigger = false;
        }
        if (wasScript)
            ImGui::PopStyleColor();
    }

    ImGui::Separator();

    // ---- Mode buttons ----
    auto ModeBtn = [&](const char* label, EditorMode mode, bool enabled = true)
    {
        const bool active = (currentMode == mode);
        if (!enabled)
            ImGui::BeginDisabled();
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.6f, 0.2f, 1.0f });
        if (ImGui::Button(label, { 60.0f, 0.0f }) && enabled)
        {
            currentMode     = mode;
            isDrawingRect   = false;
            placingSpawn    = false;
            isDraggingSpawn = false;
            polyInProgress.clear();
            draggedVertexIndex = -1;
        }
        if (active)
            ImGui::PopStyleColor();
        if (!enabled)
            ImGui::EndDisabled();
    };

    ModeBtn("View[V]", EditorMode::View);
    ImGui::SameLine();
    ModeBtn("Rect[R]", EditorMode::Rect);
    ImGui::SameLine();
    ModeBtn("Poly[P]", EditorMode::Polygon);
    ImGui::SameLine();
    ModeBtn("Vtx[E]", EditorMode::VertexEdit, selectedIndex >= 0 && selectedIndex < static_cast<int>(objects.size()));

    ImGui::Separator();

    // ---- New type selector ----
    ImGui::Text("Type:");
    ImGui::SameLine();
    int typeIdx = static_cast<int>(currentType);
    ImGui::SetNextItemWidth(110.0f);
    if (ImGui::Combo("##newtype", &typeIdx, TypeNames, IM_ARRAYSIZE(TypeNames)))
        currentType = static_cast<ObjectType>(typeIdx);

    // ---- Grid size slider ----
    ImGui::Text("Grid:");
    ImGui::SameLine();
    float gs = static_cast<float>(gridSize);
    ImGui::SetNextItemWidth(110.0f);
    if (ImGui::InputFloat("##grid", &gs, 10.0f, 40.0f, "%.0f"))
        gridSize = static_cast<double>(std::clamp(gs, 10.0f, 160.0f));

    ImGui::Separator();

    // ---- Selected object inspector ----
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(objects.size()))
    {
        auto& selObj = objects[selectedIndex];
        ImGui::TextColored({ 1.0f, 1.0f, 0.4f, 1.0f }, "Sel #%d (%s)", selectedIndex, selObj.isRect ? "Rect" : "Poly");

        int selType = static_cast<int>(selObj.type);
        ImGui::SetNextItemWidth(130.0f);
        if (ImGui::Combo("Type##sel", &selType, TypeNames, IM_ARRAYSIZE(TypeNames)))
        {
            PushUndo();
            selObj.type = static_cast<ObjectType>(selType);
        }

        // ---- Star settings: constellation IDs ----
        if (selObj.type == ObjectType::BossStar || selObj.type == ObjectType::TargetStar || selObj.type == ObjectType::LaserStar)
        {
            ImGui::Separator();

            const bool isBossStar   = selObj.type == ObjectType::BossStar;
            const bool isTargetStar = selObj.type == ObjectType::TargetStar;
            const bool isLaserStar  = selObj.type == ObjectType::LaserStar;

            ImVec4      titleColor = { 0.22f, 1.0f, 0.08f, 1.0f };
            const char* title      = "LaserStar ID: %s";

            if (isBossStar)
            {
                titleColor = { 1.0f, 0.27f, 0.67f, 1.0f };
                title      = "BossStar ID: %s";
            }
            else if (isTargetStar)
            {
                titleColor = { 1.0f, 0.84f, 0.0f, 1.0f };
                title      = "TargetStar ID: %s";
            }

            ImGui::TextColored(titleColor, title, selObj.id.empty() ? "(none)" : selObj.id.c_str());

            static char       constellationGroupBuf[64] = {};
            static int        targetIndexBuf            = 0;
            static char       starIdBuf[128]            = {};
            static int        lastStarIdx               = -1;
            static ObjectType lastStarType              = ObjectType::Floor;

            if (lastStarIdx != selectedIndex || lastStarType != selObj.type)
            {
                if (isBossStar)
                {
                    const std::string group = ExtractBossGroupFromId(selObj.id);
                    strncpy_s(constellationGroupBuf, sizeof(constellationGroupBuf), group.c_str(), _TRUNCATE);
                }
                else if (isTargetStar)
                {
                    const std::string group = ExtractTargetGroupFromId(selObj.id);
                    strncpy_s(constellationGroupBuf, sizeof(constellationGroupBuf), group.c_str(), _TRUNCATE);
                    targetIndexBuf = ExtractTargetIndexFromId(selObj.id);
                }
                else
                {
                    strncpy_s(starIdBuf, sizeof(starIdBuf), selObj.id.c_str(), _TRUNCATE);
                }

                lastStarIdx  = selectedIndex;
                lastStarType = selObj.type;
            }

            if (isBossStar || isTargetStar)
            {
                ImGui::SetNextItemWidth(160.0f);
                bool changed = ImGui::InputText("Group##constellation_group", constellationGroupBuf, sizeof(constellationGroupBuf), ImGuiInputTextFlags_CharsNoBlank);

                if (isTargetStar)
                {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(70.0f);
                    changed |= ImGui::InputInt("Index##target_index", &targetIndexBuf, 1, 1);
                    targetIndexBuf = std::max(0, targetIndexBuf);
                }

                if (changed)
                {
                    PushUndo();

                    std::string group = constellationGroupBuf;
                    if (group.empty())
                        group = "BOSS";

                    if (isBossStar)
                    {
                        selObj.id = "BS_" + group;
                    }
                    else
                    {
                        selObj.id = "TS_" + group + "_" + std::to_string(targetIndexBuf);
                    }
                }

                if (isBossStar)
                {
                    ImGui::TextDisabled("BossStar: BS_<GROUP>");
                    ImGui::TextDisabled("ex) BS_ORION");
                }
                else
                {
                    ImGui::TextDisabled("TargetStar: TS_<GROUP>_<INDEX>");
                    ImGui::TextDisabled("ex) TS_ORION_0, TS_ORION_1");
                }

                ImGui::TextColored({ 0.7f, 0.9f, 1.0f, 1.0f }, "Set rule: BS_ORION matches TS_ORION_0, TS_ORION_1, ...");
            }
            else if (isLaserStar)
            {
                ImGui::SetNextItemWidth(240.0f);
                if (ImGui::InputText("ID##laserstarid", starIdBuf, sizeof(starIdBuf), ImGuiInputTextFlags_CharsNoBlank))
                {
                    selObj.id = starIdBuf;
                }

                if (ImGui::IsItemDeactivatedAfterEdit())
                    PushUndo();

                if (selObj.id.empty())
                {
                    if (ImGui::SmallButton("Init Laser ID##star"))
                    {
                        PushUndo();
                        selObj.id = MakeDefaultObjectId(selObj.type, selectedIndex);
                        strncpy_s(starIdBuf, sizeof(starIdBuf), selObj.id.c_str(), _TRUNCATE);
                    }
                }

                ImGui::TextDisabled("ex) LS_Y_SHOT_TRACK_E_BOSS");
                ImGui::TextDisabled("format: LS_(Y/R/W)_(CONT/SHOT)_(STATIC/ROTATE/BLINK/TRACK)_(N/S/E/W/NE/NW/SE/SW)_(Name)");
            }
        }
        
        // ---- Turret settings (direction + interval) ----
        if (selObj.type == ObjectType::Turret)
        {
            ImGui::Separator();
            ImGui::TextColored({ 1.0f, 0.4f, 0.0f, 1.0f }, "Turret ID: %s", selObj.id.empty() ? "(none)" : selObj.id.c_str());

            // Parse current id
            const char*        dirItems[]   = { "N", "S", "E", "W", "NE", "NW", "SE", "SW" };
            static const char* dirValues[]  = { "N", "S", "E", "W", "NE", "NW", "SE", "SW" };
            const char*        modeItems[]  = { "Player Line", "Timed" };
            int                modeIdx      = 0; // default: existing line-of-sight turret
            int                dirIdx       = 1; // default S
            float              interval     = 2.0f;
            float              initialDelay = 2.0f;
            {
                // simple split on '_'
                std::string sid = selObj.id;
                size_t      p1  = sid.find('_');
                size_t      p2  = (p1 != std::string::npos) ? sid.find('_', p1 + 1) : std::string::npos;
                size_t      p3  = (p2 != std::string::npos) ? sid.find('_', p2 + 1) : std::string::npos;
                if (p1 != std::string::npos && p2 != std::string::npos)
                {
                    const std::string prefix = sid.substr(0, p1);
                    if (prefix == "ATRT")
                        modeIdx = 1;

                    std::string d = sid.substr(p1 + 1, p2 - p1 - 1);
                    for (int k = 0; k < 8; ++k)
                        if (d == dirValues[k])
                        {
                            dirIdx = k;
                            break;
                        }
                    try
                    {
                        interval = std::stof(sid.substr(p2 + 1, p3 - p2 - 1));
                    }
                    catch (...)
                    {
                    }
                    initialDelay = interval;
                    if (p3 != std::string::npos)
                        try
                        {
                            initialDelay = std::stof(sid.substr(p3 + 1));
                        }
                        catch (...)
                        {
                        }
                }
            }
            ImGui::SetNextItemWidth(120.0f);
            bool changed = ImGui::Combo("Mode##t", &modeIdx, modeItems, 2);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            changed |= ImGui::Combo("Dir##t", &dirIdx, dirItems, 8);
            ImGui::SetNextItemWidth(105.0f);
            changed |= ImGui::InputFloat("Cooldown##t", &interval, 0.5f, 2.0f, "%.1f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(105.0f);
            changed |= ImGui::InputFloat("Delay##t", &initialDelay, 0.5f, 2.0f, "%.1f");
            interval     = std::clamp(interval, 0.5f, 10.0f);
            initialDelay = std::clamp(initialDelay, 0.0f, 30.0f);
            if (changed)
            {
                PushUndo();
                char        buf[64];
                const char* prefix = (modeIdx == 1) ? "ATRT" : "LTRT";
                snprintf(buf, sizeof(buf), "%s_%s_%.1f_%.1f", prefix, dirValues[dirIdx], interval, initialDelay);
                selObj.id = buf;
            }
        }

        // ---- BreakableWall: water flag + custom ID ----
        if (selObj.type == ObjectType::BreakableWall)
        {
            ImGui::Separator();
            bool isWater = (selObj.id.find("WATER") != std::string::npos);
            if (ImGui::Checkbox("Water Wall##bw", &isWater))
            {
                PushUndo();
                selObj.id = isWater ? "BWALL_WATER" : "";
            }
            if (isWater)
                ImGui::TextDisabled("깨지면 물이 터져나와 플레이어 사망");

            if (!isWater)
            {
                // Sync buffer when selection changes
                static char bwIdBuf[64] = {};
                static int  lastBwIdx   = -1;
                if (lastBwIdx != selectedIndex)
                {
                    strncpy_s(bwIdBuf, sizeof(bwIdBuf), selObj.id.c_str(), _TRUNCATE);
                    lastBwIdx = selectedIndex;
                }
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::InputText("ID##bwid", bwIdBuf, sizeof(bwIdBuf), ImGuiInputTextFlags_CharsNoBlank))
                    selObj.id = bwIdBuf;
                if (ImGui::IsItemDeactivatedAfterEdit())
                    PushUndo();
                ImGui::TextDisabled("ex) BOSS_ENTRANCE");
            }
        }

        // ---- Elevator settings (travel distance + period) ----
        if (selObj.type == ObjectType::Elevator)
        {
            ImGui::Separator();
            ImGui::TextColored({ 0.27f, 0.53f, 0.8f, 1.0f }, "Elevator ID: %s", selObj.id.empty() ? "(none)" : selObj.id.c_str());

            // Parse current id  ELEV_<dist>_<period>
            float elevDist   = 400.0f;
            float elevPeriod = 4.0f;
            {
                std::string sid = selObj.id;
                size_t      p1  = sid.find('_');
                size_t      p2  = (p1 != std::string::npos) ? sid.find('_', p1 + 1) : std::string::npos;
                if (p1 != std::string::npos)
                    try
                    {
                        elevDist = std::stof(sid.substr(p1 + 1, p2 - p1 - 1));
                    }
                    catch (...)
                    {
                    }
                if (p2 != std::string::npos)
                    try
                    {
                        elevPeriod = std::stof(sid.substr(p2 + 1));
                    }
                    catch (...)
                    {
                    }
            }

            ImGui::SetNextItemWidth(100.0f);
            bool changed = ImGui::InputFloat("Dist##elev", &elevDist, 40.0f, 200.0f, "%.0f");
            elevDist     = std::clamp(elevDist, -2000.0f, 2000.0f);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            changed |= ImGui::InputFloat("Period##elev", &elevPeriod, 0.5f, 2.0f, "%.1fs");
            elevPeriod = std::clamp(elevPeriod, 0.5f, 30.0f);
            if (changed)
            {
                PushUndo();
                char buf[64];
                snprintf(buf, sizeof(buf), "ELEV_%.0f_%.1f", elevDist, elevPeriod);
                selObj.id = buf;
            }
            if (selObj.id.empty())
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("Init"))
                {
                    PushUndo();
                    selObj.id = "ELEV_400_4.0";
                }
            }
        }

        // Vertex coordinate list
        if (ImGui::TreeNode("Vertices"))
        {
            for (int vi = 0; vi < static_cast<int>(selObj.vertices.size()); ++vi)
                ImGui::Text("[%d] (%.0f, %.0f)", vi, selObj.vertices[vi].x, selObj.vertices[vi].y);
            ImGui::TreePop();
        }

        if (ImGui::Button("Delete[Del]", { 130.0f, 0.0f }))
        {
            PushUndo();
            objects.erase(objects.begin() + selectedIndex);
            selectedIndex      = -1;
            draggedVertexIndex = -1;
            currentMode        = EditorMode::View;
        }
        ImGui::SameLine();
        if (ImGui::Button("Dup[Ctrl+D]", { 130.0f, 0.0f }))
        {
            PushUndo();
            EditorObject copy = selObj;
            for (auto& v : copy.vertices)
            {
                v.x += gridSize;
                v.y -= gridSize;
            }
            objects.push_back(std::move(copy));
            selectedIndex = static_cast<int>(objects.size()) - 1;
        }
    }
    else
    {
        ImGui::TextDisabled("(nothing selected)");
    }

    // ---- Object list ----
    ImGui::Separator();
    ImGui::Text("Objects: %d   Undo: %d", static_cast<int>(objects.size()), static_cast<int>(undoStack.size()));
    ImGui::BeginChild("obj_list", { 0.0f, 110.0f }, true);
    for (int i = 0; i < static_cast<int>(objects.size()); ++i)
    {
        const CS200::RGBA rgba  = TypeColor(objects[i].type);
        const ImVec4      imcol = { ((rgba >> 24) & 0xFF) / 255.f, ((rgba >> 16) & 0xFF) / 255.f, ((rgba >> 8) & 0xFF) / 255.f, 1.0f };
        char              label[48];
        snprintf(label, sizeof(label), "#%d %s (%s)", i, objects[i].isRect ? "Rect" : "Poly", TypeNames[static_cast<int>(objects[i].type)]);
        ImGui::PushStyleColor(ImGuiCol_Text, imcol);
        if (ImGui::Selectable(label, i == selectedIndex))
        {
            selectedIndex = i;
            currentMode   = EditorMode::View;
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::Separator();

    // ---- Save / Load / Play ----
    if (ImGui::Button("Save[Ctrl+S]", { 130.0f, 0.0f }))
        SaveSVG();
    ImGui::SameLine();
    if (ImGui::Button("Load[Ctrl+O]", { 130.0f, 0.0f }))
        LoadSVG();

    ImGui::Spacing();
    if (ImGui::Button("Play [Tab]", { 265.0f, 0.0f }))
    {
        SaveSVG();
        Mode3::SetReturnPosition(s_spawnPos);
        Engine::GetGameStateManager().ChangeStateWithFade<Mode3>();
    }

    ImGui::Separator();
    ImGui::TextColored({ 1.0f, 0.55f, 0.0f, 1.0f }, "Spawn:");
    ImGui::SameLine();
    ImGui::TextDisabled("(%.0f, %.0f)", s_spawnPos.x, s_spawnPos.y);
    const bool spawnMode = placingSpawn || isDraggingSpawn;
    if (spawnMode)
        ImGui::PushStyleColor(ImGuiCol_Button, { 0.85f, 0.42f, 0.08f, 1.0f });
    if (ImGui::Button(spawnMode ? "Placing Spawn..." : "Place Spawn [G]", { 265.0f, 0.0f }))
    {
        placingSpawn    = !placingSpawn;
        isDraggingSpawn = false;
        currentMode     = EditorMode::View;
        isDrawingRect   = false;
        polyInProgress.clear();
        draggedVertexIndex = -1;
    }
    if (spawnMode)
        ImGui::PopStyleColor();
    if (ImGui::Button("Center On Spawn", { 130.0f, 0.0f }))
        editorCamPos = s_spawnPos;
    ImGui::SameLine();
    if (ImGui::Button("Spawn At View", { 130.0f, 0.0f }))
        s_spawnPos = SnapToGrid(editorCamPos, gridSize);
    float spArr[2] = { static_cast<float>(s_spawnPos.x), static_cast<float>(s_spawnPos.y) };
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::InputFloat("X##spawnx", &spArr[0], 40.0f, 200.0f, "%.0f"))
        s_spawnPos.x = static_cast<double>(spArr[0]);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::InputFloat("Y##spawny", &spArr[1], 40.0f, 200.0f, "%.0f"))
        s_spawnPos.y = static_cast<double>(spArr[1]);

    // Status message (disappears after 2 s)
    if (statusTimer > 0.0)
    {
        const bool   isError = statusMessage.find("FAIL") != std::string::npos;
        const ImVec4 col     = isError ? ImVec4{ 1.f, 0.4f, 0.4f, 1.f } : ImVec4{ 0.4f, 1.f, 0.4f, 1.f };
        ImGui::TextColored(col, "%s", statusMessage.c_str());
    }

    // ---- Hotkey help toggle ----
    ImGui::Separator();
    ImGui::Checkbox("Hotkeys", &showHotkeyHelp);
    if (showHotkeyHelp)
    {
        ImGui::TextDisabled("V=View  R=Rect  P=Poly  E=Vtx");
        ImGui::TextDisabled("Del=Delete  Esc=Cancel  Enter=Close poly");
        ImGui::TextDisabled("Ctrl+Z=Undo  Ctrl+D=Dup");
        ImGui::TextDisabled("Ctrl+S=Save  Ctrl+O=Load");
        ImGui::TextDisabled("G=Place/drag spawn");
        ImGui::TextDisabled("Tab=Play   RClick=Context menu");
        ImGui::TextDisabled("Wheel=Zoom  MMB=Pan  Arrows=Page");
    }

    ImGui::End();

    // ---- Right-click context menu (outside ImGui windows) ----
    if (!ImGui::GetIO().WantCaptureMouse && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        contextMenuWorldPos = ScreenToWorld(Engine::GetInput().GetMousePosition());
        ImGui::OpenPopup("##edctx");
    }

    if (ImGui::BeginPopup("##edctx"))
    {
        DrawContextMenu();
        ImGui::EndPopup();
    }

    if (scriptEditorMode)
    {
        DrawScriptEditorPanel();
        DrawTimelinePanel();
    }
}

// ---------------------------------------------------------------------------
// Undo
// ---------------------------------------------------------------------------

void LevelEditor::PushUndo()
{
    undoStack.push_back(objects);
    if (undoStack.size() > 20)
        undoStack.erase(undoStack.begin());
}

// ---------------------------------------------------------------------------
// Context menu
// ---------------------------------------------------------------------------

void LevelEditor::DrawContextMenu()
{
    if (ImGui::MenuItem("Set Spawn Here"))
    {
        s_spawnPos      = SnapToGrid(contextMenuWorldPos, gridSize);
        placingSpawn    = false;
        isDraggingSpawn = false;
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Rect Mode [R]"))
    {
        currentMode   = EditorMode::Rect;
        isDrawingRect = false;
    }
    if (ImGui::MenuItem("Polygon Mode [P]"))
    {
        currentMode = EditorMode::Polygon;
        polyInProgress.clear();
    }

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(objects.size()))
    {
        ImGui::Separator();
        if (ImGui::MenuItem("Delete Selected"))
        {
            PushUndo();
            objects.erase(objects.begin() + selectedIndex);
            selectedIndex      = -1;
            draggedVertexIndex = -1;
            currentMode        = EditorMode::View;
        }

        ImGui::Separator();
        ImGui::TextDisabled("Change Type:");
        for (int i = 0; i < IM_ARRAYSIZE(TypeNames); ++i)
        {
            const bool isCurrent = (static_cast<int>(objects[selectedIndex].type) == i);
            if (ImGui::MenuItem(TypeNames[i], nullptr, isCurrent) && !isCurrent)
            {
                PushUndo();
                objects[selectedIndex].type = static_cast<ObjectType>(i);
            }
        }
    }
}

// ===========================================================================
// Script Editor
// ===========================================================================

namespace
{
    const char* EventTypeName(EventType t)
    {
        switch (t)
        {
            case EventType::Lock: return "Lock";
            case EventType::Unlock: return "Unlock";
            case EventType::CameraPan: return "CamPan";
            case EventType::CameraReturn: return "CamReturn";
            case EventType::Text: return "Text";
            case EventType::Wait: return "Wait";
            case EventType::MovePlayer: return "MovePlayer";
        }
        return "?";
    }

    ImVec4 EventTypeColor(EventType t)
    {
        switch (t)
        {
            case EventType::Lock: return { 1.0f, 0.3f, 0.3f, 1.0f };
            case EventType::Unlock: return { 0.3f, 1.0f, 0.3f, 1.0f };
            case EventType::CameraPan: return { 0.3f, 0.5f, 1.0f, 1.0f };
            case EventType::CameraReturn: return { 0.3f, 0.9f, 1.0f, 1.0f };
            case EventType::Text: return { 1.0f, 0.9f, 0.2f, 1.0f };
            case EventType::Wait: return { 0.6f, 0.6f, 0.6f, 1.0f };
            case EventType::MovePlayer: return { 1.0f, 0.6f, 0.1f, 1.0f };
        }
        return { 1, 1, 1, 1 };
    }
}

// ---------------------------------------------------------------------------
// Trigger I/O
// ---------------------------------------------------------------------------

void LevelEditor::SaveTriggers()
{
    const auto dir = assets::get_base_path() / "Assets/scripts";
    std::filesystem::create_directories(dir);
    const auto path = (dir / "triggers.txt").string();

    std::ofstream f(path);
    if (!f.is_open())
    {
        statusMessage = "Trigger save FAILED";
        statusTimer   = 2.0;
        return;
    }

    for (const auto& t : triggers)
    {
        f << "TRIGGER " << t.id << " " << t.pos.x << " " << t.pos.y << " " << t.radius << " " << (t.oneShot ? 1 : 0) << " " << t.scriptFile << "\n";
    }
    statusMessage = "Triggers saved";
    statusTimer   = 2.0;
}

void LevelEditor::LoadTriggers()
{
    std::string path;
    try
    {
        path = assets::locate_asset("Assets/scripts/triggers.txt").string();
    }
    catch (...)
    {
        return;
    }

    std::ifstream f(path);
    if (!f.is_open())
        return;

    triggers.clear();
    selectedTrigger = -1;
    editEvents.clear();
    selectedEvent = -1;

    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream ss(line);
        std::string        cmd;
        ss >> cmd;
        if (cmd != "TRIGGER")
            continue;

        ScriptTrigger t;
        int           os = 1;
        ss >> t.id >> t.pos.x >> t.pos.y >> t.radius >> os >> t.scriptFile;
        t.oneShot = (os != 0);
        triggers.push_back(t);
    }
}

void LevelEditor::LoadEditEvents()
{
    if (selectedTrigger < 0 || selectedTrigger >= static_cast<int>(triggers.size()))
    {
        editEvents.clear();
        return;
    }
    editEvents    = ScriptManager::LoadScriptFile(triggers[selectedTrigger].scriptFile);
    selectedEvent = -1;
}

// ---------------------------------------------------------------------------
// Draw triggers in world
// ---------------------------------------------------------------------------

void LevelEditor::DrawTriggers(CS200::IRenderer2D& r)
{
    constexpr CS200::RGBA COL_TRIGGER      = 0xFF00FFFF; // magenta outline
    constexpr CS200::RGBA COL_TRIGGER_FILL = 0xFF00FF20; // dim fill
    constexpr CS200::RGBA COL_SELECTED     = 0xFFFF00FF; // yellow when selected

    for (int i = 0; i < static_cast<int>(triggers.size()); ++i)
    {
        const auto& t     = triggers[i];
        const bool  isSel = (i == selectedTrigger);
        const auto  col   = isSel ? COL_SELECTED : COL_TRIGGER;

        const auto mat = Math::TranslationMatrix(t.pos) * Math::ScaleMatrix(t.radius);
        r.DrawCircle(mat, COL_TRIGGER_FILL, col, isSel ? 2.5 : 1.5);

        // Small crosshair at center
        constexpr double CH = 12.0;
        r.DrawLine({ t.pos.x - CH, t.pos.y }, { t.pos.x + CH, t.pos.y }, col, 1.0);
        r.DrawLine({ t.pos.x, t.pos.y - CH }, { t.pos.x, t.pos.y + CH }, col, 1.0);
    }

    // Trigger being placed: preview circle at mouse
    if (placingTrigger)
    {
        const Math::vec2 mp  = ScreenToWorld(Engine::GetInput().GetMousePosition());
        const double     r_  = (selectedTrigger >= 0 && selectedTrigger < static_cast<int>(triggers.size())) ? triggers[selectedTrigger].radius : 120.0;
        const auto       mat = Math::TranslationMatrix(mp) * Math::ScaleMatrix(r_);
        r.DrawCircle(mat, 0xFF00FF10u, COL_TRIGGER, 1.0);
    }

    // Draw CamPan view rectangles for current script's events
    const Math::ivec2 winSz = Engine::GetWindow().GetSize();
    for (int i = 0; i < static_cast<int>(editEvents.size()); ++i)
    {
        const auto& e = editEvents[i];
        if (e.type != EventType::CameraPan && e.type != EventType::MovePlayer)
            continue;

        const bool isSel = (i == selectedEvent);

        if (e.type == EventType::CameraPan)
        {
            // Draw camera view rectangle
            const double     vw = e.viewSize.x > 0.0 ? e.viewSize.x : winSz.x;
            const double     vh = e.viewSize.y > 0.0 ? e.viewSize.y : winSz.y;
            const double     hw = vw * 0.5, hh = vh * 0.5;
            const Math::vec2 c = e.position;

            const uint32_t col = isSel ? 0x88BBFFFF : 0x334488FF;
            const double   lw  = isSel ? 2.5 : 1.0;

            r.DrawLine({ c.x - hw, c.y - hh }, { c.x + hw, c.y - hh }, col, lw);
            r.DrawLine({ c.x + hw, c.y - hh }, { c.x + hw, c.y + hh }, col, lw);
            r.DrawLine({ c.x + hw, c.y + hh }, { c.x - hw, c.y + hh }, col, lw);
            r.DrawLine({ c.x - hw, c.y + hh }, { c.x - hw, c.y - hh }, col, lw);

            // Crosshair at center
            constexpr double CH = 15.0;
            r.DrawLine({ c.x - CH, c.y }, { c.x + CH, c.y }, col, 1.0);
            r.DrawLine({ c.x, c.y - CH }, { c.x, c.y + CH }, col, 1.0);
        }
        else // MovePlayer
        {
            // Draw player target marker (orange circle)
            const uint32_t col = isSel ? 0xFF9922FF : 0xFF660088;
            const auto     mat = Math::TranslationMatrix(e.position) * Math::ScaleMatrix(18.0);
            r.DrawCircle(mat, 0xFF660020u, col, isSel ? 2.5 : 1.5);
            constexpr double CH = 12.0;
            r.DrawLine({ e.position.x - CH, e.position.y }, { e.position.x + CH, e.position.y }, col, 1.5);
            r.DrawLine({ e.position.x, e.position.y - CH }, { e.position.x, e.position.y + CH }, col, 1.5);
        }
    }

    // Camera rect drag preview
    if (camRectInProgress)
    {
        const Math::vec2 cur = SnapToGrid(ScreenToWorld(Engine::GetInput().GetMousePosition()), gridSize);
        const double     x0  = std::min(camRectDragStart.x, cur.x);
        const double     x1  = std::max(camRectDragStart.x, cur.x);
        const double     y0  = std::min(camRectDragStart.y, cur.y);
        const double     y1  = std::max(camRectDragStart.y, cur.y);
        r.DrawLine({ x0, y0 }, { x1, y0 }, 0xFFFFFFFF, 1.5);
        r.DrawLine({ x1, y0 }, { x1, y1 }, 0xFFFFFFFF, 1.5);
        r.DrawLine({ x1, y1 }, { x0, y1 }, 0xFFFFFFFF, 1.5);
        r.DrawLine({ x0, y1 }, { x0, y0 }, 0xFFFFFFFF, 1.5);
    }
}

// ---------------------------------------------------------------------------
// Trigger editing input
// ---------------------------------------------------------------------------

void LevelEditor::UpdateTriggerEditing()
{
    if (!scriptEditorMode)
        return;

    auto&            input      = Engine::GetInput();
    const bool       imguiMouse = ImGui::GetIO().WantCaptureMouse;
    const Math::vec2 mouseWorld = ScreenToWorld(input.GetMousePosition());

    // Place trigger
    if (placingTrigger && !imguiMouse && input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        ScriptTrigger t;
        t.id         = "trigger_" + std::to_string(triggers.size());
        t.pos        = SnapToGrid(mouseWorld, gridSize);
        t.radius     = 120.0;
        t.scriptFile = "script_" + std::to_string(triggers.size());
        triggers.push_back(t);
        selectedTrigger = static_cast<int>(triggers.size()) - 1;
        editEvents.clear();
        selectedEvent  = -1;
        placingTrigger = false;

        // Sync ID buf
        std::copy(t.id.begin(), t.id.begin() + std::min(t.id.size(), sizeof(triggerIdBuf) - 1), triggerIdBuf);
        triggerIdBuf[t.id.size()] = '\0';
        std::copy(t.scriptFile.begin(), t.scriptFile.begin() + std::min(t.scriptFile.size(), sizeof(scriptNameBuf) - 1), scriptNameBuf);
        scriptNameBuf[t.scriptFile.size()] = '\0';
        return;
    }

    // Click to select existing trigger
    if (!placingTrigger && !imguiMouse && input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        int found = -1;
        for (int i = static_cast<int>(triggers.size()) - 1; i >= 0; --i)
        {
            const double dx = mouseWorld.x - triggers[i].pos.x;
            const double dy = mouseWorld.y - triggers[i].pos.y;
            if (std::sqrt(dx * dx + dy * dy) <= triggers[i].radius)
            {
                found = i;
                break;
            }
        }
        if (found != selectedTrigger)
        {
            selectedTrigger = found;
            selectedEvent   = -1;
            if (found >= 0)
            {
                const auto& t = triggers[found];
                std::copy(t.id.begin(), t.id.begin() + std::min(t.id.size(), sizeof(triggerIdBuf) - 1), triggerIdBuf);
                triggerIdBuf[t.id.size()] = '\0';
                std::copy(t.scriptFile.begin(), t.scriptFile.begin() + std::min(t.scriptFile.size(), sizeof(scriptNameBuf) - 1), scriptNameBuf);
                scriptNameBuf[t.scriptFile.size()] = '\0';
                LoadEditEvents();
            }
        }
    }

    // Drag selected trigger
    if (!imguiMouse && selectedTrigger >= 0 && selectedTrigger < static_cast<int>(triggers.size()) && input.MouseButtonDown(CS230::Input::MouseButton::Left))
    {
        const double dx = mouseWorld.x - triggers[selectedTrigger].pos.x;
        const double dy = mouseWorld.y - triggers[selectedTrigger].pos.y;
        if (std::sqrt(dx * dx + dy * dy) <= triggers[selectedTrigger].radius * 0.25)
            triggers[selectedTrigger].pos = SnapToGrid(mouseWorld, gridSize);
    }

    // Camera rect drawing (for CamPan event) and MovePlayer target
    if (!placingTrigger && selectedEvent >= 0 && selectedEvent < static_cast<int>(editEvents.size()) && !imguiMouse)
    {
        auto& selEv = editEvents[selectedEvent];

        if (selEv.type == EventType::CameraPan && drawingCamRect)
        {
            // In-progress: update drag
            if (input.MouseButtonDown(CS230::Input::MouseButton::Left))
                camRectInProgress = true;

            if (input.MouseButtonJustReleased(CS230::Input::MouseButton::Left))
            {
                const Math::vec2 cur = SnapToGrid(mouseWorld, gridSize);
                const double     vw  = std::abs(cur.x - camRectDragStart.x);
                const double     vh  = std::abs(cur.y - camRectDragStart.y);
                if (vw > 20.0 && vh > 20.0)
                {
                    selEv.position.x = (camRectDragStart.x + cur.x) * 0.5;
                    selEv.position.y = (camRectDragStart.y + cur.y) * 0.5;
                    selEv.viewSize   = { vw, vh };
                }
                camRectInProgress = false;
                drawingCamRect    = false;
            }
        }
        else if (selEv.type == EventType::CameraPan && drawingCamRect && input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
        {
            camRectDragStart  = SnapToGrid(mouseWorld, gridSize);
            camRectInProgress = true;
        }

        // MovePlayer: click to set target position
        if (selEv.type == EventType::MovePlayer && input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
        {
            selEv.position = SnapToGrid(mouseWorld, gridSize);
            return; // consume click
        }
    }

    // Start cam rect drag when in draw mode and left pressed
    if (!placingTrigger && drawingCamRect && selectedEvent >= 0 && selectedEvent < static_cast<int>(editEvents.size()) && editEvents[selectedEvent].type == EventType::CameraPan && !imguiMouse &&
        input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        camRectDragStart  = SnapToGrid(mouseWorld, gridSize);
        camRectInProgress = true;
    }

    // T key: toggle place mode
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        const Uint8* sdlKeys = SDL_GetKeyboardState(nullptr);
        static bool  prevT   = false;
        const bool   curT    = sdlKeys[SDL_SCANCODE_T] != 0;
        if (curT && !prevT)
            placingTrigger = !placingTrigger;
        prevT = curT;
    }
}

// ---------------------------------------------------------------------------
// Script Editor right panel
// ---------------------------------------------------------------------------

void LevelEditor::DrawScriptEditorPanel()
{
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({ io.DisplaySize.x - 300.0f, 10.0f }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ 290.0f, 500.0f }, ImGuiCond_Always);
    ImGui::Begin("##script_panel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::TextColored({ 0.8f, 0.4f, 1.0f, 1.0f }, "[ SCRIPT EDITOR ]");
    ImGui::TextDisabled("[T] Place Trigger  [LClick] Select");
    ImGui::Separator();

    // Place mode indicator
    if (placingTrigger)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, { 0.7f, 0.2f, 0.9f, 1.0f });
        if (ImGui::Button("Placing... (click world)", { 280.0f, 0.0f }))
            placingTrigger = false;
        ImGui::PopStyleColor();
    }
    else
    {
        if (ImGui::Button("+ Place Trigger [T]", { 280.0f, 0.0f }))
            placingTrigger = true;
    }

    // Trigger list
    ImGui::Separator();
    ImGui::Text("Triggers (%d):", static_cast<int>(triggers.size()));
    ImGui::BeginChild("##trig_list", { 0.0f, 120.0f }, true);
    for (int i = 0; i < static_cast<int>(triggers.size()); ++i)
    {
        ImGui::PushID(i);
        const bool sel = (i == selectedTrigger);
        if (ImGui::Selectable(triggers[i].id.c_str(), sel))
        {
            selectedTrigger = i;
            selectedEvent   = -1;
            const auto& t   = triggers[i];
            std::copy(t.id.begin(), t.id.begin() + std::min(t.id.size(), sizeof(triggerIdBuf) - 1), triggerIdBuf);
            triggerIdBuf[t.id.size()] = '\0';
            std::copy(t.scriptFile.begin(), t.scriptFile.begin() + std::min(t.scriptFile.size(), sizeof(scriptNameBuf) - 1), scriptNameBuf);
            scriptNameBuf[t.scriptFile.size()] = '\0';
            LoadEditEvents();
        }
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 16.0f);
        if (ImGui::SmallButton("X"))
        {
            triggers.erase(triggers.begin() + i);
            if (selectedTrigger >= static_cast<int>(triggers.size()))
                selectedTrigger = static_cast<int>(triggers.size()) - 1;
            editEvents.clear();
            selectedEvent = -1;
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    // Selected trigger properties
    if (selectedTrigger >= 0 && selectedTrigger < static_cast<int>(triggers.size()))
    {
        ImGui::Separator();
        auto& t = triggers[selectedTrigger];

        // ID
        if (ImGui::InputText("ID##tid", triggerIdBuf, sizeof(triggerIdBuf)))
            t.id = triggerIdBuf;

        // Script file
        if (ImGui::InputText("Script##tsf", scriptNameBuf, sizeof(scriptNameBuf)))
            t.scriptFile = scriptNameBuf;
        ImGui::SameLine();
        if (ImGui::SmallButton("Load##tl"))
            LoadEditEvents();

        // Position
        float pos[2] = { static_cast<float>(t.pos.x), static_cast<float>(t.pos.y) };
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputFloat("X##tpx", &pos[0], 40.0f, 200.0f, "%.0f"))
            t.pos.x = static_cast<double>(pos[0]);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputFloat("Y##tpy", &pos[1], 40.0f, 200.0f, "%.0f"))
            t.pos.y = static_cast<double>(pos[1]);

        float rad = static_cast<float>(t.radius);
        if (ImGui::InputFloat("Radius##tr", &rad, 20.0f, 100.0f, "%.0f"))
            t.radius = std::clamp(static_cast<double>(rad), 20.0, 1000.0);

        ImGui::Checkbox("One Shot##tos", &t.oneShot);

        // Camera rect draw hint for selected CamPan event
        if (selectedEvent >= 0 && selectedEvent < static_cast<int>(editEvents.size()))
        {
            const auto& se = editEvents[selectedEvent];
            if (se.type == EventType::CameraPan)
            {
                ImGui::Separator();
                const bool isCamDraw = drawingCamRect;
                if (isCamDraw)
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.6f, 0.9f, 1.0f });
                if (ImGui::Button(isCamDraw ? "Drawing CamView..." : "Draw Cam View [drag]", { 280.0f, 0.0f }))
                    drawingCamRect = !drawingCamRect;
                if (isCamDraw)
                    ImGui::PopStyleColor();

                if (se.viewSize.x > 0.0)
                {
                    const double zoom = Engine::GetWindow().GetSize().x / se.viewSize.x;
                    ImGui::TextDisabled("View: %.0f x %.0f  Zoom: x%.2f", se.viewSize.x, se.viewSize.y, zoom);
                }
                else
                    ImGui::TextDisabled("View: screen size (zoom x1.0)");
            }
            else if (se.type == EventType::MovePlayer)
            {
                ImGui::Separator();
                ImGui::TextColored({ 1.0f, 0.6f, 0.1f, 1.0f }, "Click world to set player target");
                ImGui::TextDisabled("Target: (%.0f, %.0f)", se.position.x, se.position.y);
            }
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Save Triggers##ts", { 136.0f, 0.0f }))
        SaveTriggers();
    ImGui::SameLine();
    if (ImGui::Button("Load Triggers##tl", { 136.0f, 0.0f }))
        LoadTriggers();

    ImGui::End();
}

// ---------------------------------------------------------------------------
// Timeline bottom panel
// ---------------------------------------------------------------------------

void LevelEditor::DrawTimelinePanel()
{
    if (selectedTrigger < 0 || selectedTrigger >= static_cast<int>(triggers.size()))
        return;

    const ImGuiIO& io = ImGui::GetIO();
    const float    ph = 220.0f; // panel height

    ImGui::SetNextWindowPos({ 0.0f, io.DisplaySize.y - ph }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ io.DisplaySize.x - 300.0f, ph }, ImGuiCond_Always);
    ImGui::Begin("##timeline", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

    const auto& trig = triggers[selectedTrigger];
    ImGui::TextColored({ 0.8f, 0.4f, 1.0f, 1.0f }, "Timeline: %s -> %s.txt", trig.id.c_str(), trig.scriptFile.c_str());

    // View range + add event controls
    {
        float ve = static_cast<float>(timelineViewEnd);
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::InputFloat("View(s)##tve", &ve, 1.0f, 5.0f, "%.0fs"))
            timelineViewEnd = std::clamp(static_cast<double>(ve), 2.0, 60.0);
        ImGui::SameLine();

        static const char* addTypeNames[] = { "Lock", "Unlock", "CamPan", "CamReturn", "Text", "Wait", "MovePlayer" };
        static int         addType        = 5;
        ImGui::SetNextItemWidth(100.0f);
        ImGui::Combo("##addet", &addType, addTypeNames, 7);
        ImGui::SameLine();
        if (ImGui::Button("+ Event"))
        {
            ScriptEvent ev;
            ev.type     = static_cast<EventType>(addType);
            ev.time     = (selectedEvent >= 0 && selectedEvent < static_cast<int>(editEvents.size())) ? editEvents[selectedEvent].time + editEvents[selectedEvent].duration + 0.1 : 0.0;
            ev.duration = (ev.type == EventType::Lock || ev.type == EventType::Unlock || ev.type == EventType::MovePlayer) ? 0.0 : 1.0;
            editEvents.push_back(ev);
            std::sort(editEvents.begin(), editEvents.end(), [](const ScriptEvent& a, const ScriptEvent& b) { return a.time < b.time; });
            selectedEvent = static_cast<int>(editEvents.size()) - 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Script"))
            ScriptManager::SaveScriptFile(trig.scriptFile, editEvents);
        ImGui::SameLine();
        if (ImGui::Button("Load Script"))
            LoadEditEvents();
    }

    ImGui::Separator();

    // --- Timeline canvas ---
    const float LABEL_W   = 80.0f;
    const float canvasX   = ImGui::GetCursorScreenPos().x + LABEL_W;
    const float canvasY   = ImGui::GetCursorScreenPos().y;
    const float canvasW   = ImGui::GetContentRegionAvail().x - LABEL_W;
    const float trackH    = 20.0f;
    const float rulerH    = 20.0f;
    const int   numTracks = 6;
    const float canvasH   = rulerH + trackH * numTracks + 4.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled({ canvasX, canvasY }, { canvasX + canvasW, canvasY + canvasH }, IM_COL32(25, 25, 30, 255));

    // Ruler (seconds)
    const float pxPerSec = canvasW / static_cast<float>(timelineViewEnd);
    for (int i = 0; i <= static_cast<int>(timelineViewEnd); ++i)
    {
        const float x = canvasX + i * pxPerSec;
        dl->AddLine({ x, canvasY }, { x, canvasY + canvasH }, IM_COL32(70, 70, 80, 255));
        char buf[8];
        snprintf(buf, sizeof(buf), "%ds", i);
        dl->AddText({ x + 2.0f, canvasY + 2.0f }, IM_COL32(160, 160, 160, 255), buf);
    }

    // Track labels
    const char* trackLabels[] = { "Lock/Unlock", "CamPan", "CamReturn", "Text", "Wait", "MovePlayer" };
    for (int tr = 0; tr < numTracks; ++tr)
    {
        const float ty = canvasY + rulerH + tr * trackH;
        dl->AddRectFilled({ canvasX - LABEL_W, ty }, { canvasX, ty + trackH }, IM_COL32(40, 40, 50, 255));
        dl->AddText({ canvasX - LABEL_W + 4.0f, ty + 4.0f }, IM_COL32(180, 180, 180, 255), trackLabels[tr]);
        dl->AddLine({ canvasX - LABEL_W, ty + trackH }, { canvasX + canvasW, ty + trackH }, IM_COL32(50, 50, 60, 255));
    }

    // Event blocks
    auto EventTrack = [](EventType t) -> int
    {
        switch (t)
        {
            case EventType::Lock:
            case EventType::Unlock: return 0;
            case EventType::CameraPan: return 1;
            case EventType::CameraReturn: return 2;
            case EventType::Text: return 3;
            case EventType::Wait: return 4;
            case EventType::MovePlayer: return 5;
        }
        return 4;
    };

    for (int i = 0; i < static_cast<int>(editEvents.size()); ++i)
    {
        const auto& e = editEvents[i];
        if (e.time > timelineViewEnd)
            continue;

        const int    track  = EventTrack(e.type);
        const float  ex     = canvasX + static_cast<float>(e.time) * pxPerSec;
        const float  ew     = std::max(static_cast<float>(e.duration) * pxPerSec, 6.0f);
        const float  ey     = canvasY + rulerH + track * trackH + 2.0f;
        const ImVec4 cv4    = EventTypeColor(e.type);
        const ImU32  col    = IM_COL32(static_cast<int>(cv4.x * 255), static_cast<int>(cv4.y * 255), static_cast<int>(cv4.z * 255), 220);
        const ImU32  selCol = (i == selectedEvent) ? IM_COL32(255, 255, 255, 255) : col;

        dl->AddRectFilled({ ex, ey }, { ex + ew, ey + trackH - 4.0f }, col, 3.0f);
        dl->AddRect({ ex, ey }, { ex + ew, ey + trackH - 4.0f }, selCol, 3.0f, 0, (i == selectedEvent) ? 2.0f : 1.0f);

        // Label inside block
        const char* lbl = EventTypeName(e.type);
        if (ew > 30.0f)
            dl->AddText({ ex + 3.0f, ey + 3.0f }, IM_COL32(0, 0, 0, 220), lbl);
    }

    // Click detection on timeline (select event)
    const ImVec2 mp       = ImGui::GetMousePos();
    const bool   inCanvas = mp.x >= canvasX && mp.x <= canvasX + canvasW && mp.y >= canvasY + rulerH && mp.y <= canvasY + canvasH;
    if (inCanvas && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const double clickTime  = (mp.x - canvasX) / pxPerSec;
        const int    clickTrack = static_cast<int>((mp.y - canvasY - rulerH) / trackH);

        selectedEvent = -1;
        for (int i = 0; i < static_cast<int>(editEvents.size()); ++i)
        {
            const auto& e = editEvents[i];
            if (EventTrack(e.type) == clickTrack && clickTime >= e.time && clickTime <= e.time + std::max(e.duration, 0.1))
            {
                selectedEvent = i;
                break;
            }
        }
    }

    // Advance cursor past canvas
    ImGui::Dummy({ canvasW + LABEL_W, canvasH });

    // --- Selected event properties ---
    if (selectedEvent >= 0 && selectedEvent < static_cast<int>(editEvents.size()))
    {
        ImGui::Separator();
        auto& e = editEvents[selectedEvent];

        ImGui::TextColored(EventTypeColor(e.type), "[%s]", EventTypeName(e.type));
        ImGui::SameLine();

        static const char* typeCombo[] = { "Lock", "Unlock", "CamPan", "CamReturn", "Text", "Wait", "MovePlayer" };
        int                ti          = static_cast<int>(e.type);
        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::Combo("Type##ety", &ti, typeCombo, 7))
            e.type = static_cast<EventType>(ti);
        ImGui::SameLine();

        float t = static_cast<float>(e.time);
        ImGui::SetNextItemWidth(70.0f);
        if (ImGui::InputFloat("Time##et", &t, 0.1f, 1.0f, "%.2f"))
        {
            e.time = std::clamp(static_cast<double>(t), 0.0, 60.0);
            std::sort(editEvents.begin(), editEvents.end(), [](const ScriptEvent& a, const ScriptEvent& b) { return a.time < b.time; });
        }
        ImGui::SameLine();

        if (e.type != EventType::Lock && e.type != EventType::Unlock && e.type != EventType::MovePlayer)
        {
            float dur = static_cast<float>(e.duration);
            ImGui::SetNextItemWidth(70.0f);
            if (ImGui::InputFloat("Dur##ed", &dur, 0.1f, 1.0f, "%.2f"))
                e.duration = std::clamp(static_cast<double>(dur), 0.0, 30.0);
            ImGui::SameLine();
        }

        if (e.type == EventType::CameraPan)
        {
            float pp[2] = { static_cast<float>(e.position.x), static_cast<float>(e.position.y) };
            ImGui::SetNextItemWidth(90.0f);
            if (ImGui::InputFloat("CX##ecp", &pp[0], 40.0f, 200.0f, "%.0f"))
                e.position.x = static_cast<double>(pp[0]);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.0f);
            if (ImGui::InputFloat("CY##ecpy", &pp[1], 40.0f, 200.0f, "%.0f"))
                e.position.y = static_cast<double>(pp[1]);
            ImGui::SameLine();

            // viewSize display / edit
            float vs[2] = { static_cast<float>(e.viewSize.x), static_cast<float>(e.viewSize.y) };
            ImGui::SetNextItemWidth(80.0f);
            if (ImGui::InputFloat("VW##evsx", &vs[0], 40.0f, 200.0f, "%.0f"))
                e.viewSize.x = std::clamp(static_cast<double>(vs[0]), 0.0, 5000.0);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            if (ImGui::InputFloat("VH##evsy", &vs[1], 40.0f, 200.0f, "%.0f"))
                e.viewSize.y = std::clamp(static_cast<double>(vs[1]), 0.0, 5000.0);
            ImGui::SameLine();
        }

        if (e.type == EventType::MovePlayer)
        {
            float pp[2] = { static_cast<float>(e.position.x), static_cast<float>(e.position.y) };
            ImGui::SetNextItemWidth(90.0f);
            if (ImGui::InputFloat("PX##empx", &pp[0], 40.0f, 200.0f, "%.0f"))
                e.position.x = static_cast<double>(pp[0]);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.0f);
            if (ImGui::InputFloat("PY##empy", &pp[1], 40.0f, 200.0f, "%.0f"))
                e.position.y = static_cast<double>(pp[1]);
            ImGui::SameLine();
        }

        if (e.type == EventType::Text)
        {
            std::copy(e.text.begin(), e.text.begin() + std::min(e.text.size(), sizeof(eventTextBuf) - 1), eventTextBuf);
            eventTextBuf[std::min(e.text.size(), sizeof(eventTextBuf) - 1)] = '\0';
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::InputText("Text##etx", eventTextBuf, sizeof(eventTextBuf)))
                e.text = eventTextBuf;
            ImGui::SameLine();
        }

        if (ImGui::SmallButton("Del##ede"))
        {
            editEvents.erase(editEvents.begin() + selectedEvent);
            selectedEvent  = -1;
            drawingCamRect = false;
        }

        // Text position controls
        if (e.type == EventType::Text)
        {
            ImGui::Separator();
            if (e.position.x == 0.0 && e.position.y == 0.0)
                e.position = { 0.5, 0.16 };

            ImGui::TextColored({ 1.0f, 0.9f, 0.2f, 1.0f }, "Text Box Position (0~1)");
            float px = static_cast<float>(e.position.x);
            float py = static_cast<float>(e.position.y);
            ImGui::SetNextItemWidth(130.0f);
            if (ImGui::InputFloat("X##etpx", &px, 0.05f, 0.2f, "%.2f"))
                e.position.x = std::clamp(static_cast<double>(px), 0.0, 1.0);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(130.0f);
            if (ImGui::InputFloat("Y##etpy", &py, 0.05f, 0.2f, "%.2f"))
                e.position.y = std::clamp(static_cast<double>(py), 0.0, 1.0);

            ImGui::TextDisabled("Presets:");
            ImGui::SameLine();
            if (ImGui::SmallButton("Bot##ep"))
                e.position = { 0.5, 0.12 };
            ImGui::SameLine();
            if (ImGui::SmallButton("Top##ep"))
                e.position = { 0.5, 0.88 };
            ImGui::SameLine();
            if (ImGui::SmallButton("L##ep"))
                e.position = { 0.15, 0.5 };
            ImGui::SameLine();
            if (ImGui::SmallButton("R##ep"))
                e.position = { 0.85, 0.5 };
            ImGui::SameLine();
            if (ImGui::SmallButton("Ctr##ep"))
                e.position = { 0.5, 0.5 };

            // Preview box (ImGui foreground draw list)
            {
                ImDrawList*    tpdl = ImGui::GetForegroundDrawList();
                const ImGuiIO& tpio = ImGui::GetIO();
                const float    sw   = tpio.DisplaySize.x;
                const float    sh   = tpio.DisplaySize.y;
                const float    boxW = sw * 0.4f;
                const float    boxH = sh * 0.08f;
                const float    bx   = sw * px - boxW * 0.5f;
                const float    by   = sh * (1.0f - py) - boxH * 0.5f;
                tpdl->AddRectFilled({ bx, by }, { bx + boxW, by + boxH }, IM_COL32(5, 8, 14, 160), 6.0f);
                tpdl->AddRect({ bx, by }, { bx + boxW, by + boxH }, IM_COL32(68, 136, 204, 200), 6.0f, 0, 2.0f);
                if (!e.text.empty())
                    tpdl->AddText({ bx + 8.0f, by + boxH * 0.3f }, IM_COL32(255, 255, 255, 200), e.text.c_str());
            }
        }
    }

    ImGui::End();
}
