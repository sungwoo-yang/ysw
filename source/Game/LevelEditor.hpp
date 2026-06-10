#pragma once

#include "Engine/GameObjectManager.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Vec2.hpp"
#include "Game/TutorialScript.hpp"

#include <cstdint>
#include <gsl/gsl>
#include <string>
#include <vector>

namespace CS200
{
    class IRenderer2D;
}

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

enum class ObjectType
{
    Floor,
    Platform,
    Special,
    RoomBounds,
    TargetStar,
    LaserStar,
    BossStar,
    Turret,
    Block,
    Rope,
    Water,
    BreakableWall,
    Staircase,
    Spike,
    Elevator
};

struct EditorObject
{
    std::vector<Math::vec2> vertices;
    ObjectType              type   = ObjectType::Floor;
    bool                    isRect = false;
    std::string             id; // used by Turret: "LTRT_(dir)_(interval)_(delay)" or "ATRT_(dir)_(interval)_(delay)"
};

// ---------------------------------------------------------------------------
// LevelEditor GameState
// ---------------------------------------------------------------------------

class LevelEditor : public CS230::GameState
{
public:
    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    gsl::czstring GetName() const override
    {
        return "LevelEditor";
    }

    static void SetGameObjects(CS230::GameObjectManager* gom)
    {
        s_gom = gom;
    }

    static void SetSpawnPos(Math::vec2 pos)
    {
        s_spawnPos = pos;
    }

    // Call before PushState: centers editor camera on worldCenter at given zoom.
    // Also records the game-return position (used when Tab-exiting back to Mode3).
    static void SetInitialCamera(Math::vec2 worldCenter, double zoom = 1.0)
    {
        s_savedCamPos   = worldCenter;
        s_savedZoom     = zoom;
        s_gameReturnPos = worldCenter; // always return to here on Tab-exit
    }

private:
    enum class EditorMode
    {
        View,
        Rect,
        Polygon,
        VertexEdit
    };

    // ---- Camera ----
    Math::TransformationMatrix GetViewMatrix() const;
    Math::vec2                 ScreenToWorld(Math::vec2 sp) const;
    Math::vec2                 WorldToScreen(Math::vec2 wp) const;
    void                       HandleCamera();
    void                       DrawGrid(CS200::IRenderer2D& r, Math::ivec2 winSize);

    // ---- Input ----
    void UpdateModeSwitch();
    void UpdateObjectCreation();
    void UpdateSelectionAndEdit();
    bool UpdateSpawnEditing();
    void UpdateSaveLoad();

    // ---- SVG I/O ----
    void SaveSVG();
    void LoadSVG();

    // ---- Script editor ----
    void DrawScriptEditorPanel();
    void DrawTimelinePanel();
    void UpdateTriggerEditing();
    void DrawTriggers(CS200::IRenderer2D& r);
    void SaveTriggers();
    void LoadTriggers();
    void LoadEditEvents();

    // ---- Undo / context ----
    void PushUndo();
    void DrawContextMenu();

    // ---- Rendering ----
    void DrawObjects(CS200::IRenderer2D& r);
    void DrawDashedLine(CS200::IRenderer2D& r, Math::vec2 from, Math::vec2 to, uint32_t color, double width);

    // ---- Camera state ----
    Math::vec2 editorCamPos{ 0.0, 0.0 };
    double     editorZoom{ 1.0 };
    Math::vec2 prevMousePos{ 0.0, 0.0 };
    float      pendingWheel{ 0.0f };

    // ---- Editor mode ----
    EditorMode currentMode{ EditorMode::View };
    ObjectType currentType{ ObjectType::Floor };

    // ---- Grid ----
    double gridSize{ 40.0 };

    // Rect creation
    bool       isDrawingRect{ false };
    Math::vec2 rectStart{ 0.0, 0.0 };

    // Polygon creation
    std::vector<Math::vec2> polyInProgress;

    // Selection & movement (View mode)
    int        selectedIndex{ -1 };
    bool       isDraggingObject{ false };
    Math::vec2 dragLastWorld{ 0.0, 0.0 };

    // Spawn marker editing
    bool placingSpawn{ false };
    bool isDraggingSpawn{ false };

    // Vertex edit
    int draggedVertexIndex{ -1 };

    // SDL key state (no engine key for these)
    bool prevDeleteDown{ false };
    bool prevTabDown{ false };

    // World position captured at right-click (for context menu actions)
    Math::vec2 contextMenuWorldPos{ 0.0, 0.0 };

    // Undo stack (max 20 snapshots)
    std::vector<std::vector<EditorObject>> undoStack;

    // Finished objects
    std::vector<EditorObject> objects;

    // Status message (shown in ImGui for 2 seconds after save/load)
    std::string statusMessage;
    double      statusTimer{ 0.0 };

    // ImGui state
    bool showHotkeyHelp{ false };

    // ---- Script editor state ----
    bool scriptEditorMode{ false };

    // Triggers
    std::vector<ScriptTrigger> triggers;
    int                        selectedTrigger{ -1 };
    bool                       placingTrigger{ false };

    // Script file editing (for selected trigger)
    std::vector<ScriptEvent> editEvents;
    int                      selectedEvent{ -1 };
    double                   timelineViewEnd{ 10.0 };
    bool                     timelineDragging{ false };
    double                   timelineDragOriginX{ 0.0 };
    double                   timelineDragOriginTime{ 0.0 };

    char triggerIdBuf[64]  = {};
    char scriptNameBuf[64] = {};
    char eventTextBuf[256] = {};

    // Camera rect drawing (for CamPan events)
    bool       drawingCamRect{ false };
    Math::vec2 camRectDragStart{ 0.0, 0.0 };
    bool       camRectInProgress{ false };

    static CS230::GameObjectManager* s_gom;
    static Math::vec2                s_spawnPos;
    static Math::vec2                s_savedCamPos;
    static double                    s_savedZoom;
    static Math::vec2                s_gameReturnPos; // player pos when editor was opened
};
