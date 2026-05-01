#pragma once
#include "Engine/GameState.hpp"
#include "Engine/Rect.hpp"

namespace CS230
{
    class Camera;
    class MapManager;
}

class Player;
class WorldTextManager;
class Gate;
class TargetStar;

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

    enum class State
    {
        Loading,
        Playing
    };

    State currentState = State::Loading;

    // Pointers for stage-specific logic and entities
    Player*            player           = nullptr;
    CS230::Camera*     camera           = nullptr;
    CS230::MapManager* mapManager       = nullptr;
    TargetStar*        puzzleTarget     = nullptr;
    Gate*              puzzleGate       = nullptr;
    WorldTextManager*  worldTextManager = nullptr;
    std::vector<TargetStar*> targetStars;
    double playingTimer = 0.0;
    bool isCameraScaling = false;
    const double targetCameraScale = 0.5;
    const double cameraScaleSpeed = 0.5;

    Math::rect level_boundary = {
        {    0.0,    0.0 },
        { 2560.0, 1440.0 }
    };
};