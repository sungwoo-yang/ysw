#pragma once
#include "Engine/Camera.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Rect.hpp"
#include "Game/PostProcessor.hpp"
#include <gsl/gsl>

class Player;
namespace CS230 { class MapManager; }

class OriMode : public CS230::GameState {
public:
    void          Load()            override;
    void          Update(double dt) override;
    void          Unload()          override;
    void          Draw()            override;
    void          DrawImGui()       override;
    gsl::czstring GetName() const   override { return "OriMode"; }
    bool          CanPause() const  override { return state == State::Playing; }

private:
    void InitGame();

    enum class State { Loading, Playing };
    State state = State::Loading;

    Player*            player     = nullptr;
    CS230::MapManager* mapManager = nullptr;
    CS230::Camera*     camera     = nullptr;

    Math::rect    boundary      = { { -500, 2000 }, { 11000, -4000 } };
    PostProcessor postProcessor;
};
