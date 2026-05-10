#pragma once
#include "Engine/GameState.hpp"
#include "Engine/Rect.hpp"

#include <string>
#include <vector>

namespace CS230
{
    class Camera;
    class MapManager;
}

namespace Boss
{
    class ShieldEnergy;
    class LightOrbManager;
    class BossLaserManager;
    class BossPatternController;
}

class Player;
class WorldTextManager;
class Gate;
class TargetStar;
class LaserStar;
class Constellation;
class BossController;

class Boss1 : public CS230::GameState
{
public:
    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    gsl::czstring GetName() const override
    {
        return "Boss1";
    }

private:
    void InitGame();
    void BuildConstellation();
    void DisableLegacyLaserStars();
    void EnterGameOver(const std::string& reason);

    enum class State
    {
        Loading,
        Intro,
        Playing,
        GameOver
    };

    State currentState = State::Loading;

    // Pointers for stage-specific logic and entities
    Player*                  player           = nullptr;
    CS230::Camera*           camera           = nullptr;
    CS230::MapManager*       mapManager       = nullptr;
    TargetStar*              puzzleTarget     = nullptr;
    Gate*                    puzzleGate       = nullptr;
    WorldTextManager*        worldTextManager = nullptr;
    std::vector<TargetStar*> targetStars;
    Constellation*           constellation  = nullptr;
    BossController*          bossController = nullptr;

    Boss::ShieldEnergy*          shieldEnergy          = nullptr;
    Boss::LightOrbManager*       lightOrbManager       = nullptr;
    Boss::BossLaserManager*      bossLaserManager      = nullptr;
    Boss::BossPatternController* bossPatternController = nullptr;

    double       playingTimer      = 0.0;
    bool         isCameraScaling   = false;
    const double targetCameraScale = 0.5;
    const double cameraScaleSpeed  = 0.5;

    double       introTimer    = 0.0;
    const double introDuration = 3.0;

    std::string  gameOverReason;
    const double fallDeathY = -2500.0;

    Math::rect level_boundary = {
        {    0.0,    0.0 },
        { 3000.0, 1440.0 }
    };
};