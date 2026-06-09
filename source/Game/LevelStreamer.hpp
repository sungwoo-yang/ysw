#pragma once
#include "Engine/Rect.hpp"
#include "Engine/Vec2.hpp"

#include <list>
#include <vector>

namespace CS230 { class GameObject; }

// Activates / deactivates game objects based on which rooms are near the player.
// Active set = current room + all NSEW-adjacent rooms.
// Deactivation is delayed by DEACTIVATE_DELAY seconds to avoid pop-in at boundaries.
// Floor-type objects are kept in a 2-ring buffer (safer for collision).
class LevelStreamer
{
public:
    static constexpr double ADJACENCY_GAP    = 400.0;  // px — max gap between "touching" rooms
    static constexpr double DEACTIVATE_DELAY = 1.5;    // s  — lag before actually deactivating

    // Call once after the map finishes loading.
    void Init(const std::vector<Math::rect>&       rooms,
              const std::list<CS230::GameObject*>& objects);

    // Call every frame.
    void Update(Math::vec2 playerPos, double dt);

    // How many rooms are currently active (for debug display).
    int ActiveRoomCount() const;

private:
    // ---- per-room state ----
    struct RoomState
    {
        Math::rect rect;
        bool   active        = true;
        double deactivateTimer = 0.0;
        std::vector<int> neighbors; // pre-computed NSEW neighbor indices
    };

    // ---- per-object record ----
    struct ObjRecord
    {
        CS230::GameObject* obj      = nullptr;
        int                roomIdx  = -1;   // -1 → always active
        bool               isFloor  = false;
    };

    std::vector<RoomState> rooms_;
    std::vector<ObjRecord> objs_;

    // Pre-allocated working buffers — reused each Update() to avoid heap allocation
    std::vector<bool> shouldActive_;
    std::vector<bool> shouldFloor_;

    bool AreNSEWAdjacent(const Math::rect& a, const Math::rect& b) const;
    int  FindRoom(Math::vec2 pos) const;
    void SetRoomActive(int idx, bool active);
};
