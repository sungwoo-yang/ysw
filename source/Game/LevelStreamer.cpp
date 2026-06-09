#include "LevelStreamer.hpp"

#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool LevelStreamer::AreNSEWAdjacent(const Math::rect& a, const Math::rect& b) const
{
    const double xGap = std::max(0.0, std::max(a.Left()   - b.Right(),
                                                b.Left()   - a.Right()));
    const double yGap = std::max(0.0, std::max(a.Bottom() - b.Top(),
                                                b.Bottom() - a.Top()));

    const double xOvlp = std::min(a.Right(), b.Right()) - std::max(a.Left(),   b.Left());
    const double yOvlp = std::min(a.Top(),   b.Top())   - std::max(a.Bottom(), b.Bottom());

    // East / West: rooms side by side — must share vertical extent (no diagonals)
    if (xGap <= ADJACENCY_GAP && yOvlp > 0.0) return true;
    // North / South: rooms stacked — must share horizontal extent (no diagonals)
    if (yGap <= ADJACENCY_GAP && xOvlp > 0.0) return true;

    return false;
}

int LevelStreamer::FindRoom(Math::vec2 pos) const
{
    // Exact containment first
    for (int i = 0; i < static_cast<int>(rooms_.size()); ++i)
    {
        const auto& r = rooms_[i].rect;
        if (pos.x >= r.Left() && pos.x <= r.Right() &&
            pos.y >= r.Bottom() && pos.y <= r.Top())
            return i;
    }
    // Nearest room within ADJACENCY_GAP
    int    best     = -1;
    double bestDist = ADJACENCY_GAP;
    for (int i = 0; i < static_cast<int>(rooms_.size()); ++i)
    {
        const auto& r = rooms_[i].rect;
        const double cx = std::clamp(pos.x, r.Left(), r.Right());
        const double cy = std::clamp(pos.y, r.Bottom(), r.Top());
        const double dx = pos.x - cx, dy = pos.y - cy;
        const double dist = std::sqrt(dx*dx + dy*dy);
        if (dist < bestDist) { bestDist = dist; best = i; }
    }
    return best;
}

void LevelStreamer::SetRoomActive(int idx, bool active)
{
    if (idx < 0 || idx >= static_cast<int>(rooms_.size())) return;
    rooms_[idx].active = active;
}

int LevelStreamer::ActiveRoomCount() const
{
    int n = 0;
    for (const auto& r : rooms_) if (r.active) ++n;
    return n;
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void LevelStreamer::Init(const std::vector<Math::rect>&       rooms,
                         const std::list<CS230::GameObject*>& objects)
{
    rooms_.clear();
    objs_.clear();

    if (rooms.empty()) return;

    // Build room states + adjacency lists
    rooms_.resize(rooms.size());
    for (int i = 0; i < static_cast<int>(rooms.size()); ++i)
    {
        rooms_[i].rect   = rooms[i];
        rooms_[i].active = true;
    }

    for (int i = 0; i < static_cast<int>(rooms_.size()); ++i)
        for (int j = 0; j < static_cast<int>(rooms_.size()); ++j)
            if (i != j && AreNSEWAdjacent(rooms_[i].rect, rooms_[j].rect))
                rooms_[i].neighbors.push_back(j);

    shouldActive_.assign(rooms_.size(), false);
    shouldFloor_ .assign(rooms_.size(), false);

    // Assign each object to its room
    for (CS230::GameObject* obj : objects)
    {
        if (!obj) continue;

        const GameObjectTypes t = obj->Type();

        ObjRecord rec;
        rec.obj     = obj;
        rec.isFloor = (t == GameObjectTypes::Floor);

        // Player is managed externally, always active
        if (t == GameObjectTypes::Player)
        {
            rec.roomIdx = -1;
            objs_.push_back(rec);
            continue;
        }

        rec.roomIdx = FindRoom(obj->GetPosition());
        objs_.push_back(rec);
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void LevelStreamer::Update(Math::vec2 playerPos, double dt)
{
    if (rooms_.empty()) return;

    // 1. Find current room
    const int curIdx = FindRoom(playerPos);

    // 2. Mark which rooms "should" be active this frame
    //    Current + 1-ring neighbors → active
    //    Floor objects also get 2-ring (neighbors' neighbors)
    std::fill(shouldActive_.begin(), shouldActive_.end(), false);
    std::fill(shouldFloor_ .begin(), shouldFloor_ .end(), false);

    if (curIdx >= 0)
    {
        shouldActive_[curIdx] = true;
        shouldFloor_ [curIdx] = true;

        for (int n1 : rooms_[curIdx].neighbors)
        {
            shouldActive_[n1] = true;
            shouldFloor_ [n1] = true;

            // 2nd ring — only for floors
            for (int n2 : rooms_[n1].neighbors)
                shouldFloor_[n2] = true;
        }
    }
    else
    {
        // Player not in any room → keep everything active
        std::fill(shouldActive_.begin(), shouldActive_.end(), true);
        std::fill(shouldFloor_ .begin(), shouldFloor_ .end(), true);
    }

    // 3. Update per-room deactivation timers
    for (int i = 0; i < static_cast<int>(rooms_.size()); ++i)
    {
        auto& rs = rooms_[i];
        if (shouldActive_[i])
        {
            // Activate immediately, reset timer
            rs.active        = true;
            rs.deactivateTimer = 0.0;
        }
        else
        {
            // Count down before deactivating
            if (rs.active)
            {
                rs.deactivateTimer += dt;
                if (rs.deactivateTimer >= DEACTIVATE_DELAY)
                {
                    rs.active        = false;
                    rs.deactivateTimer = 0.0;
                }
            }
        }
    }

    // 4. Apply active state to every object
    for (auto& rec : objs_)
    {
        if (!rec.obj) continue;

        // Always-active objects (player, or unassigned)
        if (rec.roomIdx < 0)
        {
            rec.obj->SetIsActive(true);
            rec.obj->SetVisible(true);
            continue;
        }

        // Floors get the wider 2-ring buffer (immediate activation, delayed deactivation).
        // Others use the 1-ring buffer with the room's delayed-deactivation state.
        const bool roomOn = rec.isFloor
            ? (shouldFloor_[rec.roomIdx] || rooms_[rec.roomIdx].active)
            : rooms_[rec.roomIdx].active;

        rec.obj->SetIsActive(roomOn);
        rec.obj->SetVisible(roomOn);
    }
}
