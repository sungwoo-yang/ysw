#pragma once
#include <string>
#include <vector>

// All persistent game state that survives between sessions.
struct SaveData
{
    double spawnX     = -10.0;
    double spawnY     = 0.0;
    double hp         = 5.0;
    bool   dash       = false;
    bool   wallClimb  = false;
    bool   bash       = false;

    // Names (or "x,y" fallback) of Gate objects that are currently open.
    std::vector<std::string> openGates;
};
