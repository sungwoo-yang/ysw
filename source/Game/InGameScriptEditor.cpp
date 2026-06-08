#include "InGameScriptEditor.hpp"

#include "CutscenePlayer.hpp"
#include "ScriptManager.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/RGBA.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Input.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Path.hpp"
#include "Engine/Window.hpp"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <sstream>

// ---------------------------------------------------------------------------
// Refs + conversion
// ---------------------------------------------------------------------------

void InGameScriptEditor::SetRefs(CS230::Camera* cam, CutscenePlayer* cp,
                                  ScriptManager* sm, Math::ivec2 ws)
{
    camera    = cam;
    cutscene  = cp;
    scriptMgr = sm;
    winSize   = ws;
}

Math::vec2 InGameScriptEditor::ScreenToWorld(Math::vec2 sp) const
{
    if (!camera) return {};
    const double zoom = camera->GetScale();
    const Math::vec2 camBL = camera->GetPosition();
    return {
        camBL.x + sp.x / zoom,
        camBL.y + (winSize.y - sp.y) / zoom
    };
}

// ---------------------------------------------------------------------------
// Trigger file I/O
// ---------------------------------------------------------------------------

void InGameScriptEditor::LoadTriggers()
{
    std::string path;
    try { path = assets::locate_asset("Assets/scripts/triggers.txt").string(); }
    catch (...) { return; }

    std::ifstream f(path);
    if (!f.is_open()) return;

    triggers.clear();
    selectedTrigger = -1;
    editEvents.clear();
    selectedEvent   = -1;

    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;
        if (cmd != "TRIGGER") continue;

        ScriptTrigger t;
        int os = 1;
        ss >> t.id >> t.pos.x >> t.pos.y >> t.radius >> os >> t.scriptFile;
        t.oneShot = (os != 0);
        triggers.push_back(t);
    }
}

void InGameScriptEditor::SaveTriggers()
{
    const auto dir  = assets::get_base_path() / "Assets/scripts";
    std::filesystem::create_directories(dir);
    const auto path = (dir / "triggers.txt").string();

    std::ofstream f(path);
    if (!f.is_open()) return;

    for (const auto& t : triggers)
    {
        f << "TRIGGER " << t.id
          << " " << t.pos.x << " " << t.pos.y
          << " " << t.radius
          << " " << (t.oneShot ? 1 : 0)
          << " " << t.scriptFile << "\n";
    }

    // Immediately sync to runtime ScriptManager so triggers fire without restart
    if (scriptMgr) scriptMgr->SetTriggers(triggers);
}

void InGameScriptEditor::SaveCurrentScript()
{
    if (selectedTrigger < 0 || selectedTrigger >= static_cast<int>(triggers.size())) return;
    ScriptManager::SaveScriptFile(triggers[selectedTrigger].scriptFile, editEvents);
}

void InGameScriptEditor::LoadEditEvents()
{
    if (selectedTrigger < 0 || selectedTrigger >= static_cast<int>(triggers.size()))
    { editEvents.clear(); return; }

    editEvents    = ScriptManager::LoadScriptFile(triggers[selectedTrigger].scriptFile);
    selectedEvent = -1;

}

// ---------------------------------------------------------------------------
// Update — handles mouse input for trigger placement / selection / cam rect
// ---------------------------------------------------------------------------

void InGameScriptEditor::Update()
{
    if (!active) return;

    auto&            input      = Engine::GetInput();
    const bool       imguiMouse = ImGui::GetIO().WantCaptureMouse;
    const Math::vec2 mouseWorld = ScreenToWorld(input.GetMousePosition());

    bool clickConsumed = false;

    // ── CamPan: drag to draw rect, drag inside to move ───────────────────────
    if (!imguiMouse && selectedEvent >= 0 &&
        selectedEvent < static_cast<int>(editEvents.size()) &&
        editEvents[selectedEvent].type == EventType::CameraPan)
    {
        auto& e = editEvents[selectedEvent];
        const double vw = e.viewSize.x > 0.0 ? e.viewSize.x : winSize.x;
        const double vh = e.viewSize.y > 0.0 ? e.viewSize.y : winSize.y;
        const bool inside = (mouseWorld.x >= e.position.x - vw * 0.5 &&
                             mouseWorld.x <= e.position.x + vw * 0.5 &&
                             mouseWorld.y >= e.position.y - vh * 0.5 &&
                             mouseWorld.y <= e.position.y + vh * 0.5);

        if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
        {
            if (inside)
            {
                camRectMoving  = true;
                camRectMoveOff = mouseWorld - e.position;
            }
            else
            {
                camRectStart  = mouseWorld;
                camRectInProg = true;
            }
            clickConsumed = true;
        }

        if (camRectMoving && input.MouseButtonDown(CS230::Input::MouseButton::Left))
            e.position = mouseWorld - camRectMoveOff;

        if (camRectInProg && input.MouseButtonJustReleased(CS230::Input::MouseButton::Left))
        {
            const double dw = std::abs(mouseWorld.x - camRectStart.x);
            const double dh = std::abs(mouseWorld.y - camRectStart.y);
            if (dw > 20.0 && dh > 20.0)
            {
                e.position = { (camRectStart.x + mouseWorld.x) * 0.5,
                               (camRectStart.y + mouseWorld.y) * 0.5 };
                e.viewSize  = { dw, dh };
            }
            camRectInProg = false;
        }
        if (input.MouseButtonJustReleased(CS230::Input::MouseButton::Left))
            camRectMoving = false;
    }

    // ── MovePlayer: waypoint drag / add ──────────────────────────────────────
    if (!imguiMouse && selectedEvent >= 0 &&
        selectedEvent < static_cast<int>(editEvents.size()) &&
        editEvents[selectedEvent].type == EventType::MovePlayer)
    {
        auto& e   = editEvents[selectedEvent];
        auto& wps = e.waypoints;

        auto nearestWP = [&](Math::vec2 p) -> int
        {
            int    best  = -1;
            double bestD = 30.0 * 30.0;
            for (int i = 0; i < static_cast<int>(wps.size()); ++i)
            {
                const double ddx = p.x - wps[i].pos.x;
                const double ddy = p.y - wps[i].pos.y;
                const double d2  = ddx*ddx + ddy*ddy;
                if (d2 < bestD) { bestD = d2; best = i; }
            }
            return best;
        };

        if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
        {
            const int near = nearestWP(mouseWorld);
            if (near >= 0)
            {
                selectedWaypoint = near;
                waypointDragging = true;
            }
            else
            {
                wps.push_back({ WaypointType::Walk, mouseWorld });
                selectedWaypoint = static_cast<int>(wps.size()) - 1;
                waypointDragging = true;
            }
            clickConsumed = true;
        }

        if (waypointDragging && input.MouseButtonDown(CS230::Input::MouseButton::Left) &&
            selectedWaypoint >= 0 && selectedWaypoint < static_cast<int>(wps.size()))
        {
            wps[selectedWaypoint].pos = mouseWorld;
        }

        if (input.MouseButtonJustReleased(CS230::Input::MouseButton::Left))
            waypointDragging = false;
    }

    // ── Text: click to set screen position ───────────────────────────────────
    if (!imguiMouse && !clickConsumed && selectedEvent >= 0 &&
        selectedEvent < static_cast<int>(editEvents.size()) &&
        editEvents[selectedEvent].type == EventType::Text)
    {
        if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
        {
            const Math::vec2 raw = input.GetMousePosition();
            editEvents[selectedEvent].position = {
                raw.x / winSize.x,
                1.0 - raw.y / winSize.y
            };
            clickConsumed = true;
        }
    }

    // ── Place trigger ─────────────────────────────────────────────────────────
    if (!clickConsumed && placingTrigger && !imguiMouse &&
        input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        ScriptTrigger t;
        t.id         = "trigger_" + std::to_string(triggers.size());
        t.pos        = mouseWorld;
        t.radius     = 120.0;
        t.scriptFile = "script_" + std::to_string(triggers.size());
        triggers.push_back(t);
        selectedTrigger = static_cast<int>(triggers.size()) - 1;
        editEvents.clear();
        selectedEvent   = -1;
        placingTrigger  = false;
        clickConsumed   = true;

        const auto& ti = triggers[selectedTrigger];
        std::copy(ti.id.begin(), ti.id.begin() + std::min(ti.id.size(), sizeof(trigIdBuf)-1), trigIdBuf);
        trigIdBuf[ti.id.size()] = '\0';
        std::copy(ti.scriptFile.begin(), ti.scriptFile.begin() + std::min(ti.scriptFile.size(), sizeof(scriptBuf)-1), scriptBuf);
        scriptBuf[ti.scriptFile.size()] = '\0';
    }

    // ── Select trigger ────────────────────────────────────────────────────────
    if (!clickConsumed && !placingTrigger && !imguiMouse &&
        input.MouseButtonJustPressed(CS230::Input::MouseButton::Left))
    {
        int found = -1;
        for (int i = static_cast<int>(triggers.size()) - 1; i >= 0; --i)
        {
            const double dx = mouseWorld.x - triggers[i].pos.x;
            const double dy = mouseWorld.y - triggers[i].pos.y;
            if (std::sqrt(dx*dx + dy*dy) <= triggers[i].radius)
            { found = i; break; }
        }
        if (found != selectedTrigger)
        {
            selectedTrigger = found;
            selectedEvent   = -1;
            if (found >= 0)
            {
                const auto& ti = triggers[found];
                std::copy(ti.id.begin(), ti.id.begin() + std::min(ti.id.size(), sizeof(trigIdBuf)-1), trigIdBuf);
                trigIdBuf[ti.id.size()] = '\0';
                std::copy(ti.scriptFile.begin(), ti.scriptFile.begin() + std::min(ti.scriptFile.size(), sizeof(scriptBuf)-1), scriptBuf);
                scriptBuf[ti.scriptFile.size()] = '\0';
                LoadEditEvents();
            }
        }
    }

    // ── Drag selected trigger (right mouse) ───────────────────────────────────
    if (!imguiMouse && selectedTrigger >= 0 &&
        selectedTrigger < static_cast<int>(triggers.size()) &&
        input.MouseButtonDown(CS230::Input::MouseButton::Right))
    {
        triggers[selectedTrigger].pos = mouseWorld;
    }
}

// ---------------------------------------------------------------------------
// DrawWorld — renders overlays into the game scene (world-space renderer)
// ---------------------------------------------------------------------------

namespace
{
    const char* IGEventTypeName(EventType t)
    {
        switch (t)
        {
        case EventType::Lock:         return "Lock";
        case EventType::Unlock:       return "Unlock";
        case EventType::CameraPan:    return "CamPan";
        case EventType::CameraReturn: return "CamReturn";
        case EventType::Text:         return "Text";
        case EventType::Wait:         return "Wait";
        case EventType::MovePlayer:   return "MovePly";
        case EventType::FreezeTime:   return "Freeze";
        case EventType::UnfreezeTime: return "Unfreeze";
        }
        return "?";
    }

    ImVec4 IGEventColor(EventType t)
    {
        switch (t)
        {
        case EventType::Lock:         return { 1.0f, 0.3f, 0.3f, 1.0f };
        case EventType::Unlock:       return { 0.3f, 1.0f, 0.3f, 1.0f };
        case EventType::CameraPan:    return { 0.3f, 0.5f, 1.0f, 1.0f };
        case EventType::CameraReturn: return { 0.3f, 0.9f, 1.0f, 1.0f };
        case EventType::Text:         return { 1.0f, 0.9f, 0.2f, 1.0f };
        case EventType::Wait:         return { 0.6f, 0.6f, 0.6f, 1.0f };
        case EventType::MovePlayer:   return { 1.0f, 0.6f, 0.1f, 1.0f };
        case EventType::FreezeTime:   return { 0.4f, 0.9f, 1.0f, 1.0f };
        case EventType::UnfreezeTime: return { 0.7f, 0.95f, 1.0f, 1.0f };
        }
        return { 1,1,1,1 };
    }

    int IGEventTrack(EventType t)
    {
        switch (t)
        {
        case EventType::Lock:
        case EventType::Unlock:       return 0;
        case EventType::CameraPan:    return 1;
        case EventType::CameraReturn: return 2;
        case EventType::Text:         return 3;
        case EventType::Wait:         return 4;
        case EventType::MovePlayer:   return 5;
        case EventType::FreezeTime:
        case EventType::UnfreezeTime: return 6;
        }
        return 4;
    }
}

void InGameScriptEditor::DrawWorld(CS200::IRenderer2D& r)
{
    if (!active) return;

    constexpr CS200::RGBA COL_TRIG     = 0xFF00FFFF;
    constexpr CS200::RGBA COL_TRIG_SEL = 0xFFFF00FF;
    constexpr CS200::RGBA COL_TRIG_F   = 0xFF00FF18u;

    // Draw trigger circles
    for (int i = 0; i < static_cast<int>(triggers.size()); ++i)
    {
        const auto& t   = triggers[i];
        const bool  sel = (i == selectedTrigger);
        const auto  col = sel ? COL_TRIG_SEL : COL_TRIG;
        const auto  mat = Math::TranslationMatrix(t.pos) * Math::ScaleMatrix(t.radius);
        r.DrawCircle(mat, COL_TRIG_F, col, sel ? 2.5 : 1.5);
        constexpr double CH = 12.0;
        r.DrawLine({ t.pos.x - CH, t.pos.y }, { t.pos.x + CH, t.pos.y }, col, 1.0);
        r.DrawLine({ t.pos.x, t.pos.y - CH }, { t.pos.x, t.pos.y + CH }, col, 1.0);
    }

    // Placing preview
    if (placingTrigger)
    {
        const Math::vec2 mp = ScreenToWorld(Engine::GetInput().GetMousePosition());
        const auto mat = Math::TranslationMatrix(mp) * Math::ScaleMatrix(120.0);
        r.DrawCircle(mat, 0xFF00FF10u, COL_TRIG, 1.0);
    }

    // Draw CamPan view rects and MovePlayer markers for current script
    for (int i = 0; i < static_cast<int>(editEvents.size()); ++i)
    {
        const auto& e   = editEvents[i];
        const bool  sel = (i == selectedEvent);

        if (e.type == EventType::CameraPan)
        {
            const double vw = e.viewSize.x > 0.0 ? e.viewSize.x : winSize.x;
            const double vh = e.viewSize.y > 0.0 ? e.viewSize.y : winSize.y;
            const double hw = vw * 0.5, hh = vh * 0.5;
            const Math::vec2 c = e.position;

            // Highlight when mouse is inside (can move) vs outside (can draw)
            const Math::vec2 mw = ScreenToWorld(Engine::GetInput().GetMousePosition());
            const bool mouseInside = sel &&
                (mw.x >= c.x - hw && mw.x <= c.x + hw &&
                 mw.y >= c.y - hh && mw.y <= c.y + hh);

            const uint32_t col = sel
                ? (mouseInside ? 0xFFFF44FF : 0x88BBFFFF)   // yellow=move, blue=draw
                : 0x334488FF;
            const double lw = sel ? 2.5 : 1.0;

            r.DrawLine({ c.x-hw, c.y-hh }, { c.x+hw, c.y-hh }, col, lw);
            r.DrawLine({ c.x+hw, c.y-hh }, { c.x+hw, c.y+hh }, col, lw);
            r.DrawLine({ c.x+hw, c.y+hh }, { c.x-hw, c.y+hh }, col, lw);
            r.DrawLine({ c.x-hw, c.y+hh }, { c.x-hw, c.y-hh }, col, lw);
            constexpr double CH = 14.0;
            r.DrawLine({ c.x-CH, c.y }, { c.x+CH, c.y }, col, 1.0);
            r.DrawLine({ c.x, c.y-CH }, { c.x, c.y+CH }, col, 1.0);

            // Corner handles (resize hint)
            if (sel)
            {
                constexpr double HR = 8.0;
                for (auto corner : { Math::vec2{c.x-hw, c.y-hh}, {c.x+hw, c.y-hh},
                                     Math::vec2{c.x+hw, c.y+hh}, {c.x-hw, c.y+hh} })
                {
                    const auto mat = Math::TranslationMatrix(corner) * Math::ScaleMatrix(HR);
                    r.DrawCircle(mat, 0x88BBFF80u, col, 1.5);
                }
            }
        }
        else if (e.type == EventType::MovePlayer)
        {
            constexpr double PLAYER_VY = 1100.0;
            constexpr double PLAYER_G  = 2200.0;
            const auto& wps = e.waypoints;

            for (int wi = 0; wi < static_cast<int>(wps.size()); ++wi)
            {
                const Math::vec2 cur = wps[wi].pos;

                // Segment from previous waypoint
                if (wi > 0)
                {
                    const Math::vec2 prev = wps[wi-1].pos;
                    if (wps[wi].type == WaypointType::Jump)
                    {
                        const double dxw  = cur.x - prev.x;
                        const double dyw  = cur.y - prev.y;
                        const double disc = PLAYER_VY*PLAYER_VY - 2.0*PLAYER_G*dyw;
                        const double T    = (disc >= 0.0)
                            ? (PLAYER_VY + std::sqrt(disc)) / PLAYER_G
                            : 2.0*PLAYER_VY / PLAYER_G;
                        const double vxw  = (T > 0.001) ? dxw / T : 0.0;
                        for (int k = 0; k < 20; ++k)
                        {
                            const double t0 = T * k / 20.0;
                            const double t1 = T * (k+1) / 20.0;
                            const Math::vec2 p0 = { prev.x + vxw*t0,
                                                    prev.y + PLAYER_VY*t0 - 0.5*PLAYER_G*t0*t0 };
                            const Math::vec2 p1 = { prev.x + vxw*t1,
                                                    prev.y + PLAYER_VY*t1 - 0.5*PLAYER_G*t1*t1 };
                            r.DrawLine(p0, p1, sel ? 0xFF6633FFu : 0x993311FFu, sel ? 2.0 : 1.0);
                        }
                    }
                    else
                    {
                        r.DrawLine(prev, cur, sel ? 0xFFAA22FFu : 0x775500FFu, sel ? 2.0 : 1.0);
                    }
                }

                // Waypoint marker
                const bool     wpSel     = sel && (wi == selectedWaypoint);
                const bool     isJump    = (wps[wi].type == WaypointType::Jump);
                const uint32_t markerCol = wpSel  ? 0xFFFFFFFFu
                    : (isJump ? (sel ? 0xFF7744FFu : 0xAA4422FFu)
                               : (sel ? 0xFFAA22FFu : 0x886600FFu));
                const double   r_        = wpSel ? 22.0 : (sel ? 17.0 : 14.0);
                const auto     mat       = Math::TranslationMatrix(cur) * Math::ScaleMatrix(r_);
                r.DrawCircle(mat,
                    isJump ? 0xFF33001Au : 0xFF660030u,
                    markerCol, wpSel ? 3.0 : (sel ? 2.0 : 1.5));
                constexpr double CH = 9.0;
                r.DrawLine({ cur.x-CH, cur.y }, { cur.x+CH, cur.y }, markerCol, 1.0);
                r.DrawLine({ cur.x, cur.y-CH }, { cur.x, cur.y+CH }, markerCol, 1.0);
            }
        }
    }

    // Camera rect drag preview
    if (camRectInProg)
    {
        const Math::vec2 cur = ScreenToWorld(Engine::GetInput().GetMousePosition());
        const double x0 = std::min(camRectStart.x, cur.x);
        const double x1 = std::max(camRectStart.x, cur.x);
        const double y0 = std::min(camRectStart.y, cur.y);
        const double y1 = std::max(camRectStart.y, cur.y);
        r.DrawLine({x0,y0},{x1,y0}, 0xFFFFFFFF, 1.5);
        r.DrawLine({x1,y0},{x1,y1}, 0xFFFFFFFF, 1.5);
        r.DrawLine({x1,y1},{x0,y1}, 0xFFFFFFFF, 1.5);
        r.DrawLine({x0,y1},{x0,y0}, 0xFFFFFFFF, 1.5);
    }
}

// ---------------------------------------------------------------------------
// DrawImGui
// ---------------------------------------------------------------------------

void InGameScriptEditor::DrawImGui()
{
    if (!active) return;
    DrawTriggerPanel();
    DrawTimelinePanel();
}

void InGameScriptEditor::DrawTriggerPanel()
{
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({ io.DisplaySize.x - 305.0f, 10.0f }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ 295.0f, 520.0f }, ImGuiCond_Always);
    ImGui::Begin("##igs_panel", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::TextColored({ 0.4f, 1.0f, 0.5f, 1.0f }, "[ IN-GAME SCRIPT EDITOR ]");
    ImGui::TextDisabled("LClick=Select  RClick+drag=Move trig");
    ImGui::Separator();

    // Runtime status
    if (scriptMgr)
    {
        const int rtCount = static_cast<int>(scriptMgr->GetTriggers().size());
        if (rtCount == static_cast<int>(triggers.size()))
            ImGui::TextColored({ 0.3f, 1.0f, 0.4f, 1.0f }, "Runtime: %d triggers synced", rtCount);
        else
            ImGui::TextColored({ 1.0f, 0.4f, 0.2f, 1.0f },
                               "Runtime: %d triggers (editor: %d) -- Save to sync!",
                               rtCount, static_cast<int>(triggers.size()));
    }
    ImGui::Separator();

    // Simulate / Stop
    if (cutscene)
    {
        if (cutscene->IsPlaying())
        {
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.2f, 0.2f, 1.0f });
            if (ImGui::Button("STOP Simulation", { 285.0f, 0.0f })) cutscene->Stop();
            ImGui::PopStyleColor();
        }
        else if (!editEvents.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.7f, 0.3f, 1.0f });
            if (ImGui::Button("SIMULATE (play script)", { 285.0f, 0.0f }))
                cutscene->Play(editEvents);
            ImGui::PopStyleColor();
        }
    }

    ImGui::Separator();

    // Place trigger button
    {
        const bool placing = placingTrigger;
        if (placing) ImGui::PushStyleColor(ImGuiCol_Button, { 0.7f, 0.2f, 0.9f, 1.0f });
        if (ImGui::Button(placing ? "Placing... (LClick world)" : "+ Place Trigger", { 285.0f, 0.0f }))
            placingTrigger = !placingTrigger;
        if (placing) ImGui::PopStyleColor();
    }

    // Trigger list
    ImGui::Separator();
    ImGui::Text("Triggers (%d):", static_cast<int>(triggers.size()));
    ImGui::BeginChild("##igs_tlist", { 0.0f, 100.0f }, true);
    for (int i = 0; i < static_cast<int>(triggers.size()); ++i)
    {
        ImGui::PushID(i);
        const bool sel = (i == selectedTrigger);
        if (ImGui::Selectable(triggers[i].id.c_str(), sel))
        {
            selectedTrigger  = i;
            selectedEvent    = -1;
            selectedWaypoint = -1;
            camRectInProg    = false;
            camRectMoving    = false;
            const auto& ti  = triggers[i];
            std::copy(ti.id.begin(), ti.id.begin() + std::min(ti.id.size(), sizeof(trigIdBuf)-1), trigIdBuf);
            trigIdBuf[ti.id.size()] = '\0';
            std::copy(ti.scriptFile.begin(), ti.scriptFile.begin() + std::min(ti.scriptFile.size(), sizeof(scriptBuf)-1), scriptBuf);
            scriptBuf[ti.scriptFile.size()] = '\0';
            LoadEditEvents();
        }
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 16.0f);
        if (ImGui::SmallButton("X"))
        {
            triggers.erase(triggers.begin() + i);
            if (selectedTrigger >= static_cast<int>(triggers.size()))
                selectedTrigger = static_cast<int>(triggers.size()) - 1;
            editEvents.clear(); selectedEvent = -1;
            ImGui::PopID(); break;
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    // Selected trigger properties
    if (selectedTrigger >= 0 && selectedTrigger < static_cast<int>(triggers.size()))
    {
        ImGui::Separator();
        auto& t = triggers[selectedTrigger];

        if (ImGui::InputText("ID##igsid", trigIdBuf, sizeof(trigIdBuf))) t.id = trigIdBuf;

        if (ImGui::InputText("Script##igssf", scriptBuf, sizeof(scriptBuf))) t.scriptFile = scriptBuf;
        ImGui::SameLine();
        if (ImGui::SmallButton("Load##igsl")) LoadEditEvents();

        float pos[2] = { static_cast<float>(t.pos.x), static_cast<float>(t.pos.y) };
        ImGui::SetNextItemWidth(110.0f);
        if (ImGui::InputFloat("X##igspx", &pos[0], 40.0f, 200.0f, "%.0f")) t.pos.x = static_cast<double>(pos[0]);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.0f);
        if (ImGui::InputFloat("Y##igspy", &pos[1], 40.0f, 200.0f, "%.0f")) t.pos.y = static_cast<double>(pos[1]);

        float rad = static_cast<float>(t.radius);
        if (ImGui::InputFloat("Radius##igsr", &rad, 20.0f, 100.0f, "%.0f")) t.radius = std::clamp(static_cast<double>(rad), 20.0, 1000.0);

        ImGui::Checkbox("One Shot##igsos", &t.oneShot);

        // Per-event world interaction hints
        if (selectedEvent >= 0 && selectedEvent < static_cast<int>(editEvents.size()))
        {
            const auto& se = editEvents[selectedEvent];
            if (se.type == EventType::CameraPan)
            {
                ImGui::Separator();
                if (camRectInProg)
                    ImGui::TextColored({ 0.3f, 0.8f, 1.0f, 1.0f }, "Drawing view rect...");
                else if (camRectMoving)
                    ImGui::TextColored({ 1.0f, 1.0f, 0.3f, 1.0f }, "Moving view rect...");
                else
                {
                    ImGui::TextColored({ 0.7f, 0.85f, 1.0f, 1.0f }, "CamPan - Direct Edit:");
                    ImGui::TextDisabled("  Drag empty space -> draw new view");
                    ImGui::TextDisabled("  Drag inside rect  -> move view");
                }
                if (se.viewSize.x > 0.0)
                {
                    const double zoom = winSize.x / se.viewSize.x;
                    ImGui::TextColored({ 0.5f, 0.8f, 1.0f, 1.0f },
                                       "View %.0fx%.0f  Zoom x%.2f",
                                       se.viewSize.x, se.viewSize.y, zoom);
                }
                else ImGui::TextDisabled("View: screen size (zoom x1.0)");
            }
            else if (se.type == EventType::MovePlayer)
            {
                ImGui::Separator();
                ImGui::TextColored({ 1.0f, 0.7f, 0.2f, 1.0f }, "MovePlayer - Direct Edit:");
                ImGui::TextDisabled("  LClick near waypoint -> select/drag");
                ImGui::TextDisabled("  LClick empty world  -> add Walk waypoint");
                ImGui::TextDisabled("Waypoints: %d", static_cast<int>(se.waypoints.size()));
            }
        }
    }

    ImGui::Separator();
    // Save All: triggers + current script in one click
    ImGui::PushStyleColor(ImGuiCol_Button, { 0.6f, 0.4f, 0.1f, 1.0f });
    if (ImGui::Button("SAVE ALL (Triggers + Script)", { 285.0f, 0.0f }))
    {
        SaveTriggers();
        SaveCurrentScript();
    }
    ImGui::PopStyleColor();
    if (ImGui::Button("Save Triggers##igsts", { 140.0f, 0.0f })) SaveTriggers();
    ImGui::SameLine();
    if (ImGui::Button("Load Triggers##igstl", { 140.0f, 0.0f })) LoadTriggers();

    ImGui::End();
}

void InGameScriptEditor::DrawTimelinePanel()
{
    if (selectedTrigger < 0 || selectedTrigger >= static_cast<int>(triggers.size()))
        return;

    const ImGuiIO& io = ImGui::GetIO();
    const float    ph = 245.0f;

    ImGui::SetNextWindowPos({ 0.0f, io.DisplaySize.y - ph }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ io.DisplaySize.x - 305.0f, ph }, ImGuiCond_Always);
    ImGui::Begin("##igs_timeline", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    const auto& trig = triggers[selectedTrigger];
    ImGui::TextColored({ 0.4f, 1.0f, 0.5f, 1.0f }, "Timeline: %s -> %s.txt",
                       trig.id.c_str(), trig.scriptFile.c_str());

    // Controls row
    {
        float ve = static_cast<float>(timelineEnd);
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::InputFloat("View(s)##itve", &ve, 1.0f, 5.0f, "%.0fs")) timelineEnd = std::clamp(static_cast<double>(ve), 2.0, 60.0);
        ImGui::SameLine();

        static const char* typeNames[] = { "Lock","Unlock","CamPan","CamReturn","Text","Wait","MovePlayer","FreezeTime","UnfreezeTime" };
        static int addType = 5;
        ImGui::SetNextItemWidth(110.0f);
        ImGui::Combo("##itaddet", &addType, typeNames, 9);
        ImGui::SameLine();

        if (ImGui::Button("+ Event##it"))
        {
            ScriptEvent ev;
            ev.type     = static_cast<EventType>(addType);
            ev.time     = (selectedEvent >= 0 && selectedEvent < static_cast<int>(editEvents.size()))
                          ? editEvents[selectedEvent].time + editEvents[selectedEvent].duration + 0.1
                          : 0.0;
            ev.duration = (ev.type == EventType::Lock         ||
                           ev.type == EventType::Unlock       ||
                           ev.type == EventType::MovePlayer   ||
                           ev.type == EventType::FreezeTime   ||
                           ev.type == EventType::UnfreezeTime) ? 0.0 : 1.0;
            editEvents.push_back(ev);
            std::sort(editEvents.begin(), editEvents.end(),
                      [](const ScriptEvent& a, const ScriptEvent& b){ return a.time < b.time; });
            selectedEvent = static_cast<int>(editEvents.size()) - 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save##its"))
        {
            ScriptManager::SaveScriptFile(trig.scriptFile, editEvents);
            // Re-sync triggers so CheckTriggers picks up new scriptFile name if changed
            if (scriptMgr) scriptMgr->SetTriggers(triggers);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##itl")) LoadEditEvents();
    }

    ImGui::Separator();

    // Timeline canvas
    const float LABEL_W = 80.0f;
    const float canvasX = ImGui::GetCursorScreenPos().x + LABEL_W;
    const float canvasY = ImGui::GetCursorScreenPos().y;
    const float canvasW = ImGui::GetContentRegionAvail().x - LABEL_W;
    const float trackH  = 20.0f;
    const float rulerH  = 20.0f;
    const int   nTracks = 7;
    const float canvasH = rulerH + trackH * nTracks + 4.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled({ canvasX, canvasY }, { canvasX + canvasW, canvasY + canvasH },
                      IM_COL32(25, 25, 30, 255));

    const float pxPerSec = canvasW / static_cast<float>(timelineEnd);

    // Ruler
    for (int i = 0; i <= static_cast<int>(timelineEnd); ++i)
    {
        const float x = canvasX + i * pxPerSec;
        dl->AddLine({ x, canvasY }, { x, canvasY + canvasH }, IM_COL32(70, 70, 80, 255));
        char buf[8]; snprintf(buf, sizeof(buf), "%ds", i);
        dl->AddText({ x + 2.0f, canvasY + 2.0f }, IM_COL32(160, 160, 160, 255), buf);
    }

    // Track labels
    const char* labels[] = { "Lock/Unlock", "CamPan", "CamReturn", "Text", "Wait", "MovePlayer", "Freeze" };
    for (int tr = 0; tr < nTracks; ++tr)
    {
        const float ty = canvasY + rulerH + tr * trackH;
        dl->AddRectFilled({ canvasX - LABEL_W, ty }, { canvasX, ty + trackH },
                          IM_COL32(40, 40, 50, 255));
        dl->AddText({ canvasX - LABEL_W + 4.0f, ty + 4.0f },
                    IM_COL32(180, 180, 180, 255), labels[tr]);
        dl->AddLine({ canvasX - LABEL_W, ty + trackH },
                    { canvasX + canvasW, ty + trackH }, IM_COL32(50, 50, 60, 255));
    }

    // Playhead (when simulating)
    if (cutscene && cutscene->IsPlaying())
    {
        // We don't have direct access to currentTime, but we can show a fixed indicator
        dl->AddLine({ canvasX, canvasY }, { canvasX, canvasY + canvasH },
                    IM_COL32(255, 80, 80, 200), 2.0f);
    }

    // Event blocks
    for (int i = 0; i < static_cast<int>(editEvents.size()); ++i)
    {
        const auto& e     = editEvents[i];
        if (e.time > timelineEnd) continue;
        const int   track = IGEventTrack(e.type);
        const float ex    = canvasX + static_cast<float>(e.time) * pxPerSec;
        const float ew    = std::max(static_cast<float>(e.duration) * pxPerSec, 6.0f);
        const float ey    = canvasY + rulerH + track * trackH + 2.0f;
        const ImVec4 cv4  = IGEventColor(e.type);
        const ImU32  col  = IM_COL32(static_cast<int>(cv4.x*255), static_cast<int>(cv4.y*255),
                                      static_cast<int>(cv4.z*255), 220);
        const bool   isSel = (i == selectedEvent);
        dl->AddRectFilled({ ex, ey }, { ex + ew, ey + trackH - 4.0f }, col, 3.0f);
        dl->AddRect({ ex, ey }, { ex + ew, ey + trackH - 4.0f },
                    isSel ? IM_COL32(255,255,255,255) : col, 3.0f, 0, isSel ? 2.0f : 1.0f);
        if (ew > 30.0f)
            dl->AddText({ ex + 3.0f, ey + 3.0f }, IM_COL32(0,0,0,220), IGEventTypeName(e.type));
    }

    // Click to select event
    const ImVec2 mp = ImGui::GetMousePos();
    const bool inCanvas = mp.x >= canvasX && mp.x <= canvasX + canvasW &&
                          mp.y >= canvasY + rulerH && mp.y <= canvasY + canvasH;
    if (inCanvas && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const double ct     = (mp.x - canvasX) / pxPerSec;
        const int    ctrack = static_cast<int>((mp.y - canvasY - rulerH) / trackH);
        selectedEvent    = -1;
        selectedWaypoint = -1;
        for (int i = 0; i < static_cast<int>(editEvents.size()); ++i)
        {
            const auto& e = editEvents[i];
            if (IGEventTrack(e.type) == ctrack &&
                ct >= e.time && ct <= e.time + std::max(e.duration, 0.1))
            { selectedEvent = i; break; }
        }
    }

    ImGui::Dummy({ canvasW + LABEL_W, canvasH });

    // Selected event properties
    if (selectedEvent >= 0 && selectedEvent < static_cast<int>(editEvents.size()))
    {
        ImGui::Separator();
        auto& e = editEvents[selectedEvent];

        ImGui::TextColored(IGEventColor(e.type), "[%s]", IGEventTypeName(e.type));
        ImGui::SameLine();

        static const char* tnames[] = { "Lock","Unlock","CamPan","CamReturn","Text","Wait","MovePlayer","FreezeTime","UnfreezeTime" };
        int ti = static_cast<int>(e.type);
        ImGui::SetNextItemWidth(110.0f);
        if (ImGui::Combo("Type##iget", &ti, tnames, 9)) e.type = static_cast<EventType>(ti);
        ImGui::SameLine();

        float t = static_cast<float>(e.time);
        ImGui::SetNextItemWidth(70.0f);
        if (ImGui::InputFloat("Time##iget2", &t, 0.1f, 1.0f, "%.2f"))
        {
            e.time = std::clamp(static_cast<double>(t), 0.0, 60.0);
            std::sort(editEvents.begin(), editEvents.end(),
                      [](const ScriptEvent& a, const ScriptEvent& b){ return a.time < b.time; });
        }
        ImGui::SameLine();

        if (e.type != EventType::Lock && e.type != EventType::Unlock &&
            e.type != EventType::MovePlayer &&
            e.type != EventType::FreezeTime && e.type != EventType::UnfreezeTime)
        {
            float dur = static_cast<float>(e.duration);
            ImGui::SetNextItemWidth(70.0f);
            if (ImGui::InputFloat("Dur##iged", &dur, 0.1f, 1.0f, "%.2f")) e.duration = std::clamp(static_cast<double>(dur), 0.0, 30.0);
            ImGui::SameLine();
        }

        if (e.type == EventType::CameraPan)
        {
            if (e.viewSize.x > 0.0)
            {
                const double zoom = winSize.x / e.viewSize.x;
                ImGui::TextColored({ 0.5f, 0.8f, 1.0f, 1.0f },
                                   "(%.0f, %.0f) view %.0fx%.0f zoom x%.2f",
                                   e.position.x, e.position.y,
                                   e.viewSize.x, e.viewSize.y, zoom);
            }
            else
                ImGui::TextColored({ 0.5f, 0.8f, 1.0f, 1.0f },
                                   "(%.0f, %.0f) default zoom",
                                   e.position.x, e.position.y);
            ImGui::SameLine();
        }
        if (e.type == EventType::MovePlayer)
        {
            ImGui::TextColored({ 1.0f, 0.7f, 0.2f, 1.0f },
                               "%d waypoints", static_cast<int>(e.waypoints.size()));
            ImGui::SameLine();
        }
        if (e.type == EventType::Text)
        {
            std::copy(e.text.begin(), e.text.begin() + std::min(e.text.size(), sizeof(textBuf)-1), textBuf);
            textBuf[std::min(e.text.size(), sizeof(textBuf)-1)] = '\0';
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::InputText("Text##igtx", textBuf, sizeof(textBuf))) e.text = textBuf;
            ImGui::SameLine();
        }

        if (ImGui::SmallButton("Del##igdel"))
        {
            editEvents.erase(editEvents.begin() + selectedEvent);
            selectedEvent    = -1;
            selectedWaypoint = -1;
        }

        // Waypoint editor for MovePlayer
        if (selectedEvent >= 0 && e.type == EventType::MovePlayer)
        {
            ImGui::Separator();
            ImGui::TextColored({ 1.0f, 0.7f, 0.2f, 1.0f }, "Waypoint Path");
            ImGui::TextDisabled("LClick world: drag/add  |  panel: type/del");
            ImGui::SameLine();
            if (ImGui::SmallButton("+Walk##aw"))
            {
                e.waypoints.push_back({ WaypointType::Walk, {} });
                selectedWaypoint = static_cast<int>(e.waypoints.size()) - 1;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("+Jump##aj"))
            {
                e.waypoints.push_back({ WaypointType::Jump, {} });
                selectedWaypoint = static_cast<int>(e.waypoints.size()) - 1;
            }

            for (int wi = 0; wi < static_cast<int>(e.waypoints.size()); ++wi)
            {
                auto& wp = e.waypoints[wi];
                ImGui::PushID(wi);
                const bool wpSel = (wi == selectedWaypoint);
                if (wpSel) ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 0.2f, 1.0f });

                char rowLabel[64];
                std::snprintf(rowLabel, sizeof(rowLabel), "%d: %s  (%.0f, %.0f)",
                              wi, (wp.type == WaypointType::Jump ? "Jump" : "Walk"),
                              wp.pos.x, wp.pos.y);
                if (ImGui::Selectable(rowLabel, wpSel, 0, { 180.0f, 0.0f }))
                    selectedWaypoint = wi;

                if (wpSel) ImGui::PopStyleColor();

                ImGui::SameLine();
                if (ImGui::SmallButton(wp.type == WaypointType::Walk ? "->J" : "->W"))
                    wp.type = (wp.type == WaypointType::Walk) ? WaypointType::Jump : WaypointType::Walk;
                ImGui::SameLine();
                if (ImGui::SmallButton("X##wdel"))
                {
                    e.waypoints.erase(e.waypoints.begin() + wi);
                    if (selectedWaypoint >= static_cast<int>(e.waypoints.size()))
                        selectedWaypoint = static_cast<int>(e.waypoints.size()) - 1;
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
        }

        // Text position controls + visual preview
        if (e.type == EventType::Text)
        {
            ImGui::Separator();

            // Ensure default when zero
            if (e.position.x == 0.0 && e.position.y == 0.0)
                e.position = { 0.5, 0.16 };

            ImGui::TextColored({ 1.0f, 0.9f, 0.2f, 1.0f }, "Text Box Position (screen 0~1)");
            ImGui::TextDisabled("LClick screen = set position");

            float px = static_cast<float>(e.position.x);
            float py = static_cast<float>(e.position.y);
            bool changed = false;
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputFloat("X (L-R)##itpx", &px, 0.05f, 0.2f, "%.2f")) { e.position.x = std::clamp(static_cast<double>(px), 0.0, 1.0); changed = true; }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputFloat("Y (B-T)##itpy", &py, 0.05f, 0.2f, "%.2f")) { e.position.y = std::clamp(static_cast<double>(py), 0.0, 1.0); changed = true; }
            (void)changed;

            // Preset buttons
            ImGui::TextDisabled("Presets:");
            ImGui::SameLine();
            if (ImGui::SmallButton("Bot")) e.position = { 0.5, 0.12 };
            ImGui::SameLine();
            if (ImGui::SmallButton("Top")) e.position = { 0.5, 0.88 };
            ImGui::SameLine();
            if (ImGui::SmallButton("L"))   e.position = { 0.15, 0.5 };
            ImGui::SameLine();
            if (ImGui::SmallButton("R"))   e.position = { 0.85, 0.5 };
            ImGui::SameLine();
            if (ImGui::SmallButton("Ctr")) e.position = { 0.5, 0.5 };

            // Screen-space preview box using ImDrawList
            {
                const ImGuiIO& tpio  = ImGui::GetIO();
                ImDrawList*    tpdl  = ImGui::GetForegroundDrawList();
                const float    sw    = tpio.DisplaySize.x;
                const float    sh    = tpio.DisplaySize.y;

                // Approximate box size
                const float boxW = sw * 0.4f;
                const float boxH = sh * 0.08f;
                const float bx   = sw * px - boxW * 0.5f;
                // ImGui Y: 0=top, screen Y: 0=bottom → flip
                const float by   = sh * (1.0f - py) - boxH * 0.5f;

                tpdl->AddRectFilled({ bx, by }, { bx + boxW, by + boxH },
                                    IM_COL32(5, 8, 14, 160), 6.0f);
                tpdl->AddRect({ bx, by }, { bx + boxW, by + boxH },
                               IM_COL32(68, 136, 204, 200), 6.0f, 0, 2.0f);
                if (!e.text.empty())
                    tpdl->AddText({ bx + 8.0f, by + boxH * 0.3f },
                                  IM_COL32(255, 255, 255, 200), e.text.c_str());
            }
        }
    }

    ImGui::End();
}
