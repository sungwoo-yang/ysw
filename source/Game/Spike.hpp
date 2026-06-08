#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"

class Spike : public CS230::GameObject
{
public:
    enum class Dir { Up, Down, Left, Right };

    Spike(Math::vec2 pos, Math::vec2 size, Dir direction = Dir::Up);

    void Draw(const Math::TransformationMatrix&) override;
    bool CanCollideWith(GameObjectTypes other) override;
    void ResolveCollision(CS230::GameObject* other) override;

    GameObjectTypes Type()     override { return GameObjectTypes::Spike; }
    std::string     TypeName() override { return "Spike"; }

    static constexpr double DamageAmount = 4.0;

private:
    Math::vec2 size;
    Dir        direction;
};
