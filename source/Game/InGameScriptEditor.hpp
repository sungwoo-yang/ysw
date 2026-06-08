#pragma once
#include "Engine/Vec2.hpp"
#include "Game/TutorialScript.hpp"
#include <string>
#include <vector>

namespace CS200 { class IRenderer2D; }
namespace CS230 { class Camera; }
class CutscenePlayer;
class ScriptManager;

// In-game script editor overlay.
// Floats on top of the running game — the player/world stays live.
// Mode3 creates this and calls Update / DrawWorld / DrawImGui each frame.
class InGameScriptEditor
{
public:
    bool active = false;

    void SetRefs(CS230::Camera* cam, CutscenePlayer* cp,
                 ScriptManager* sm, Math::ivec2 ws);

    void Update();
    void DrawWorld(CS200::IRenderer2D& renderer);
    void DrawImGui();

    void LoadTriggers();
    void SaveTriggers();       // saves file + syncs ScriptManager
    void SaveCurrentScript();  // saves current trigger's script file

private:
    // World ↔ screen conversion using the game camera
    Math::vec2 ScreenToWorld(Math::vec2 sp) const;

    // ImGui sub-panels
    void DrawTriggerPanel();
    void DrawTimelinePanel();
    void LoadEditEvents();

    CS230::Camera*   camera   = nullptr;
    CutscenePlayer*  cutscene = nullptr;
    ScriptManager*   scriptMgr = nullptr;
    Math::ivec2      winSize  = {};

    std::vector<ScriptTrigger> triggers;
    int    selectedTrigger = -1;
    bool   placingTrigger  = false;

    std::vector<ScriptEvent> editEvents;
    int    selectedEvent   = -1;
    double timelineEnd     = 10.0;

    // Camera rect interaction
    bool       camRectInProg    = false;  // drag-to-draw in progress
    Math::vec2 camRectStart     { 0.0, 0.0 };
    bool       camRectMoving    = false;  // drag-to-move in progress
    Math::vec2 camRectMoveOff   { 0.0, 0.0 };

    // MovePlayer waypoint interaction
    int        selectedWaypoint = -1;
    bool       waypointDragging = false;

    char trigIdBuf[64]   = {};
    char scriptBuf[64]   = {};
    char textBuf[256]    = {};
};
