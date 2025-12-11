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

class Mode2 : public CS230::GameState
{
public:
    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    gsl::czstring GetName() const override
    {
        return "Mode2";
    }

private:
    void InitGame();

    enum class State
    {
        Loading,
        Playing
    };

    State currentState = State::Loading;

    CS230::Camera*     camera           = nullptr;
    Player*            player           = nullptr;
    CS230::MapManager* mapManager       = nullptr;
    WorldTextManager*  worldTextManager = nullptr;

    Math::rect level_boundary = {
        {    0.0,    0.0 },
        { 2560.0, 1440.0 }
    };
};