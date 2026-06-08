#pragma once
#include "Engine/Vec2.hpp"
#include <string>
#include <vector>

enum class WaypointType { Walk, Jump };

struct WaypointStep {
    WaypointType type = WaypointType::Walk;
    Math::vec2   pos  = {};
};

// Time-based script event (fires at absolute time from cutscene start)
enum class EventType { Lock, Unlock, CameraPan, CameraReturn, Text, Wait, MovePlayer, FreezeTime, UnfreezeTime };

struct ScriptEvent
{
    double      time     = 0.0;
    EventType   type     = EventType::Wait;
    Math::vec2  position = {};    // CameraPan: world center; MovePlayer: legacy target
    Math::vec2  viewSize = {};    // CameraPan: world viewport size (0,0 = screen = zoom 1.0)
    double      duration = 1.0;  // CameraPan / CameraReturn / Text / Wait
    std::string text;             // Text only
    std::vector<WaypointStep> waypoints;  // MovePlayer: ordered waypoint chain
};

// A trigger zone in the world that fires a named script file
struct ScriptTrigger
{
    std::string  id;
    Math::vec2   pos;
    double       radius     = 120.0;
    std::string  scriptFile;  // "name" -> Assets/scripts/name.txt
    bool         oneShot    = true;
    bool         triggered  = false;
};
