#pragma once
#include "Engine/Component.hpp"
#include "Engine/Vec2.hpp"
#include "Game/TutorialScript.hpp"
#include <optional>
#include <vector>

class Player;
class TutorialOverlay;
namespace CS230 { class Camera; }

class CutscenePlayer : public CS230::Component
{
public:
    void SetRefs(Player* p, TutorialOverlay* ov, CS230::Camera* cam, Math::ivec2 winSize);

    void Play(const std::vector<ScriptEvent>& events);
    void Stop();
    void Update(double dt) override;

    bool IsPlaying()       const { return playing; }
    bool IsTimeFrozen()    const { return timeFrozen; }
    bool overridingCamera  = false;
    bool overridingZoom    = false;
    Math::vec2 cameraTarget;
    double     zoomTarget  = 1.0;

private:
    static constexpr double CAM_MOVE_SEC = 0.8; // fixed pan transition time

    struct CamEffect
    {
        Math::vec2 from, to;
        double     fromZoom = 1.0, toZoom = 1.0;
        double     startTime;
        double     moveEndTime;   // startTime + CAM_MOVE_SEC
        double     totalEndTime;  // moveEndTime + hold duration
        bool       returning = false;
    };

    void FireEvent(const ScriptEvent& e);

    bool   playing      = false;
    bool   timeFrozen   = false;
    double currentTime  = 0.0;
    double totalTime    = 0.0;
    int    nextEventIdx = 0;

    std::vector<ScriptEvent>  events;
    std::optional<CamEffect>  activeCam;

    Player*          player  = nullptr;
    TutorialOverlay* overlay = nullptr;
    CS230::Camera*   camera  = nullptr;
    Math::ivec2      winSize = { 0, 0 };
};
