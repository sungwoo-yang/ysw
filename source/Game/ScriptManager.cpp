#include "ScriptManager.hpp"

#include "CutscenePlayer.hpp"
#include "Engine/Path.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

// ── Trigger check ─────────────────────────────────────────────────────────────

void ScriptManager::CheckTriggers(Math::vec2 playerPos)
{
    if (!cutscenePlayer || cutscenePlayer->IsPlaying()) return;

    for (auto& t : triggers)
    {
        if (t.triggered && t.oneShot) continue;
        const double dx = playerPos.x - t.pos.x;
        const double dy = playerPos.y - t.pos.y;
        if (std::sqrt(dx * dx + dy * dy) <= t.radius)
        {
            if (t.oneShot) t.triggered = true;
            auto events = LoadScriptFile(t.scriptFile);
            if (!events.empty()) cutscenePlayer->Play(events);
            break;
        }
    }
}

// ── Script file I/O ───────────────────────────────────────────────────────────
// Format: <time> <CMD> [args...]
// 0.0 LOCK
// 0.5 TEXT 3.0 "Hello!"
// 1.5 CAM_PAN 500 -800 2.0
// 5.0 CAM_RETURN 1.0
// 6.0 UNLOCK

std::vector<ScriptEvent> ScriptManager::LoadScriptFile(const std::string& name)
{
    std::string path;
    try { path = assets::locate_asset("Assets/scripts/" + name + ".txt").string(); }
    catch (...) { return {}; }

    std::ifstream f(path);
    if (!f.is_open()) return {};

    std::vector<ScriptEvent> events;
    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        double      time = 0.0;
        std::string cmd;
        if (!(ss >> time >> cmd)) continue;

        ScriptEvent ev;
        ev.time = time;

        if      (cmd == "LOCK")          { ev.type = EventType::Lock;         ev.duration = 0.0; }
        else if (cmd == "UNLOCK")        { ev.type = EventType::Unlock;       ev.duration = 0.0; }
        else if (cmd == "FREEZE_TIME")   { ev.type = EventType::FreezeTime;   ev.duration = 0.0; }
        else if (cmd == "UNFREEZE_TIME") { ev.type = EventType::UnfreezeTime; ev.duration = 0.0; }
        else if (cmd == "WAIT")          { ev.type = EventType::Wait;         ss >> ev.duration; }
        else if (cmd == "CAM_RETURN")    { ev.type = EventType::CameraReturn; ss >> ev.duration; }
        else if (cmd == "CAM_PAN")
        {
            ev.type = EventType::CameraPan;
            // Collect remaining numbers on the line
            std::vector<double> nums;
            double v;
            while (ss >> v) nums.push_back(v);
            if (nums.size() >= 5)
            {
                // cx cy vw vh dur
                ev.position = { nums[0], nums[1] };
                ev.viewSize  = { nums[2], nums[3] };
                ev.duration  = nums[4];
            }
            else if (nums.size() >= 3)
            {
                // legacy: cx cy dur
                ev.position = { nums[0], nums[1] };
                ev.duration  = nums[2];
            }
        }
        else if (cmd == "MOVE_PLAYER")
        {
            ev.type     = EventType::MovePlayer;
            ev.duration = 0.0;
            std::string token;
            if (ss >> token)
            {
                if (token == "WALK" || token == "JUMP")
                {
                    // New format: WALK x y JUMP x y ...
                    do {
                        WaypointStep wp;
                        wp.type = (token == "JUMP") ? WaypointType::Jump : WaypointType::Walk;
                        if (ss >> wp.pos.x >> wp.pos.y)
                            ev.waypoints.push_back(wp);
                    } while (ss >> token);
                }
                else
                {
                    // Legacy format: x y
                    double x = 0.0, y = 0.0;
                    try { x = std::stod(token); } catch (...) {}
                    ss >> y;
                    ev.position = { x, y };
                    ev.waypoints.push_back({ WaypointType::Walk, { x, y } });
                }
            }
        }
        else if (cmd == "TEXT")
        {
            ev.type = EventType::Text;
            // Format: dur [px py] "text"
            ss >> ev.duration;
            std::string rest;
            std::getline(ss, rest);
            // Check if there are two numbers before the quote (normalized pos)
            const auto q1 = rest.find('"');
            if (q1 != std::string::npos)
            {
                const std::string before = rest.substr(0, q1);
                std::istringstream bs(before);
                double px, py;
                if (bs >> px >> py)
                    ev.position = { px, py };
                const auto q2 = rest.rfind('"');
                ev.text = (q2 != q1)
                    ? rest.substr(q1 + 1, q2 - q1 - 1)
                    : rest.substr(q1 + 1);
            }
            else ev.text = rest;
        }
        else continue;

        events.push_back(ev);
    }
    return events;
}

void ScriptManager::SaveScriptFile(const std::string& name,
                                    const std::vector<ScriptEvent>& events)
{
    const auto dir  = assets::get_base_path() / "Assets/scripts";
    std::filesystem::create_directories(dir);
    const auto path = (dir / (name + ".txt")).string();

    std::ofstream f(path);
    if (!f.is_open()) return;

    for (const auto& e : events)
    {
        f << e.time << " ";
        switch (e.type)
        {
        case EventType::Lock:         f << "LOCK\n";                                             break;
        case EventType::Unlock:       f << "UNLOCK\n";                                           break;
        case EventType::FreezeTime:   f << "FREEZE_TIME\n";                                      break;
        case EventType::UnfreezeTime: f << "UNFREEZE_TIME\n";                                    break;
        case EventType::Wait:         f << "WAIT "       << e.duration               << "\n";   break;
        case EventType::CameraReturn: f << "CAM_RETURN " << e.duration               << "\n";   break;
        case EventType::CameraPan:    f << "CAM_PAN "    << e.position.x
                                                          << " " << e.position.y
                                                          << " " << e.viewSize.x
                                                          << " " << e.viewSize.y
                                                          << " " << e.duration        << "\n";   break;
        case EventType::Text:         f << "TEXT "       << e.duration
                                                          << " " << e.position.x
                                                          << " " << e.position.y
                                                          << " \"" << e.text << "\"\n";          break;
        case EventType::MovePlayer:
            f << "MOVE_PLAYER";
            if (!e.waypoints.empty()) {
                for (const auto& wp : e.waypoints)
                    f << " " << (wp.type == WaypointType::Jump ? "JUMP" : "WALK")
                      << " " << wp.pos.x << " " << wp.pos.y;
            } else {
                f << " WALK " << e.position.x << " " << e.position.y;
            }
            f << "\n";
            break;
        }
    }
}

// ── Trigger file I/O ──────────────────────────────────────────────────────────
// Format: TRIGGER <id> <x> <y> <radius> <oneShot> <scriptFile>

void ScriptManager::LoadTriggers()
{
    std::string path;
    try { path = assets::locate_asset("Assets/scripts/triggers.txt").string(); }
    catch (...) { return; }

    std::ifstream f(path);
    if (!f.is_open()) return;

    triggers.clear();
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

void ScriptManager::SaveTriggers()
{
    const auto dir  = assets::get_base_path() / "Assets/scripts";
    std::filesystem::create_directories(dir);
    const auto path = (dir / "triggers.txt").string();

    std::ofstream f(path);
    if (!f.is_open()) return;

    for (const auto& t : triggers)
    {
        f << "TRIGGER " << t.id
          << " " << t.pos.x
          << " " << t.pos.y
          << " " << t.radius
          << " " << (t.oneShot ? 1 : 0)
          << " " << t.scriptFile
          << "\n";
    }
}
