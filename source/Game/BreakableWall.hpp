#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Rect.hpp"
#include "Engine/Vec2.hpp"
#include <string>

class BreakableWall : public CS230::GameObject
{
public:
    BreakableWall(Math::vec2 pos, Math::vec2 size, const std::string& id = "");

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    bool CanCollideWith(GameObjectTypes other) override;
    void ResolveCollision(CS230::GameObject* other) override;

    GameObjectTypes Type() override   { return GameObjectTypes::BreakableWall; }
    std::string     TypeName() override { return "BreakableWall"; }

    void Break();
    bool               IsBroken()    const { return state != State::Intact; }
    bool               IsWaterWall() const { return waterWall; }
    void               ResetIfWater();     // called by Mode3 on respawn
    Math::rect  GetWorldBounds() const
    {
        const Math::vec2 p = GetPosition();
        return Math::rect{{ p.x - size.x * 0.5, p.y - size.y * 0.5 },
                          { p.x + size.x * 0.5, p.y + size.y * 0.5 }};
    }

private:
    Math::vec2  size;
    bool        waterWall = false;

    enum class State { Intact, Breaking, Broken };
    State  state      = State::Intact;
    double breakTimer = 0.0;

    static constexpr double BreakDuration = 0.55;
    static constexpr double FlashDuration = 0.06;

};
