#include "CutscenePlayer.hpp"

#include "Engine/Camera.hpp"
#include "Game/Player.hpp"
#include "Game/TutorialOverlay.hpp"

#include <algorithm>
#include <cmath>

void CutscenePlayer::SetRefs(Player* p, TutorialOverlay* ov,
                              CS230::Camera* cam, Math::ivec2 ws)
{
    player  = p;
    overlay = ov;
    camera  = cam;
    winSize = ws;
}

void CutscenePlayer::Play(const std::vector<ScriptEvent>& evs)
{
    events = evs;
    std::sort(events.begin(), events.end(),
              [](const ScriptEvent& a, const ScriptEvent& b){ return a.time < b.time; });

    totalTime = 0.0;
    for (const auto& e : events)
    {
        // CamPan/CamReturn: event time = move(0.8s) + hold(duration)
        const double extra = (e.type == EventType::CameraPan ||
                              e.type == EventType::CameraReturn) ? CAM_MOVE_SEC : 0.0;
        totalTime = std::max(totalTime, e.time + extra + e.duration);
    }

    playing          = true;
    currentTime      = 0.0;
    nextEventIdx     = 0;
    activeCam        = std::nullopt;
    overridingCamera = false;
    overridingZoom   = false;
    zoomTarget       = 1.0;
}

void CutscenePlayer::Stop()
{
    playing          = false;
    timeFrozen       = false;
    overridingCamera = false;
    overridingZoom   = false;
    zoomTarget       = 1.0;
    if (player) player->inputLocked = false;
    activeCam = std::nullopt;
}

void CutscenePlayer::FireEvent(const ScriptEvent& e)
{
    switch (e.type)
    {
    case EventType::Lock:
        if (player) player->inputLocked = true;
        break;

    case EventType::Unlock:
        if (player) player->inputLocked = false;
        break;

    case EventType::MovePlayer:
        if (player) {
            if (!e.waypoints.empty())
                player->SetAutoMove(e.waypoints);
            else
                player->SetAutoMove(e.position);
        }
        break;

    case EventType::CameraPan:
    {
        overridingCamera = true;
        overridingZoom   = true;
        const Math::vec2 from     = camera ? camera->GetPosition() : Math::vec2{};
        const double     fromZoom = camera ? camera->GetScale()    : 1.0;

        double toZoom = 1.0;
        if (e.viewSize.x > 0.0)
            toZoom = static_cast<double>(winSize.x) / e.viewSize.x;

        const double visW = winSize.x / toZoom;
        const double visH = winSize.y / toZoom;
        const Math::vec2 to = e.position - Math::vec2{ visW * 0.5, visH * 0.5 };

        // duration = hold time; movement takes CAM_MOVE_SEC
        activeCam = CamEffect{ from, to, fromZoom, toZoom,
                               e.time,
                               e.time + CAM_MOVE_SEC,
                               e.time + CAM_MOVE_SEC + e.duration,
                               false };
        cameraTarget = from;
        zoomTarget   = fromZoom;
        break;
    }

    case EventType::CameraReturn:
    {
        overridingCamera = true;
        overridingZoom   = true;
        const Math::vec2 from     = camera ? camera->GetPosition() : Math::vec2{};
        const double     fromZoom = camera ? camera->GetScale()    : 1.0;
        const Math::vec2 to = player
            ? player->GetPosition() - Math::vec2{
                  static_cast<double>(winSize.x) * 0.5,
                  static_cast<double>(winSize.y) * 0.5 }
            : Math::vec2{};

        activeCam = CamEffect{ from, to, fromZoom, 1.0,
                               e.time,
                               e.time + CAM_MOVE_SEC,
                               e.time + CAM_MOVE_SEC + e.duration,
                               true };
        cameraTarget = from;
        zoomTarget   = fromZoom;
        break;
    }

    case EventType::Text:
    {
        // position field holds normalized screen pos for Text events.
        // {0,0} means "use default bottom-center".
        const Math::vec2 pos = (e.position.x == 0.0 && e.position.y == 0.0)
                               ? Math::vec2{ 0.5, 0.16 }
                               : e.position;
        if (overlay) overlay->Show(e.text, e.duration, pos);
        break;
    }

    case EventType::FreezeTime:
        timeFrozen = true;
        break;

    case EventType::UnfreezeTime:
        timeFrozen = false;
        break;

    case EventType::Wait:
        break;
    }
}

void CutscenePlayer::Update(double dt)
{
    if (!playing) return;
    currentTime += dt;

    // Fire events whose time has arrived
    while (nextEventIdx < static_cast<int>(events.size()) &&
           currentTime >= events[static_cast<size_t>(nextEventIdx)].time)
    {
        FireEvent(events[static_cast<size_t>(nextEventIdx)]);
        ++nextEventIdx;
    }

    // Interpolate active camera pan + zoom (move phase only)
    if (activeCam)
    {
        const double moveDur = activeCam->moveEndTime - activeCam->startTime;
        double prog = (moveDur > 0.0)
            ? (currentTime - activeCam->startTime) / moveDur
            : 1.0;
        prog = std::clamp(prog, 0.0, 1.0);
        const double st = prog * prog * (3.0 - 2.0 * prog); // smoothstep

        cameraTarget = {
            activeCam->from.x + (activeCam->to.x - activeCam->from.x) * st,
            activeCam->from.y + (activeCam->to.y - activeCam->from.y) * st
        };
        zoomTarget = activeCam->fromZoom + (activeCam->toZoom - activeCam->fromZoom) * st;

        // After movement: hold at target
        if (currentTime >= activeCam->moveEndTime)
        {
            cameraTarget = activeCam->to;
            zoomTarget   = activeCam->toZoom;
        }

        // After hold: release camera
        if (currentTime >= activeCam->totalEndTime)
        {
            if (activeCam->returning)
            {
                overridingCamera = false;
                overridingZoom   = false;
            }
            activeCam = std::nullopt;
        }
    }

    // Done when all events fired and total time elapsed
    if (currentTime >= totalTime && nextEventIdx >= static_cast<int>(events.size()))
    {
        playing          = false;
        timeFrozen       = false;
        overridingCamera = false;
        overridingZoom   = false;
        zoomTarget       = 1.0;
        if (player) player->inputLocked = false;
    }
}
