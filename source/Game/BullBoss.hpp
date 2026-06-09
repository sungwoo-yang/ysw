#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"

class BreakableWall;
class Player;

class BullBoss : public CS230::GameObject
{
public:
    // Adjust these to resize the boss placeholder rectangle
    static constexpr double WIDTH  = 240.0;
    static constexpr double HEIGHT = 240.0;

    enum class State { Idle, Charging, Breaking, Intro, Chasing, Done };

    BullBoss(Math::vec2 pos, BreakableWall* entranceWall, Player* player);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    bool CanCollideWith(GameObjectTypes) override { return false; }
    void ResolveCollision(CS230::GameObject*) override {}

    GameObjectTypes Type()     override { return GameObjectTypes::BullBoss; }
    std::string     TypeName() override { return "BullBoss"; }

    State GetState() const { return state; }
    void  TriggerEntrance();   // called by Mode3 to start the sequence

private:
    State          state        = State::Idle;
    double         stateTimer   = 0.0;
    BreakableWall* entranceWall = nullptr;
    Player*        player       = nullptr;

    static constexpr double CHARGE_SPEED   = 600.0;  // px/s leftward
    static constexpr double WALL_STOP_DIST = 20.0;   // stop this far right of wall edge
    static constexpr double BREAK_WAIT     = 0.65;   // wait for shard animation
    static constexpr double INTRO_DURATION = 1.2;    // pause before chase
};
