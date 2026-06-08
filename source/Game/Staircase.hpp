#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"
#include <algorithm>

class Staircase : public CS230::GameObject
{
public:
    enum class Dir { UpRight, UpLeft };

    Staircase(Math::vec2 pos, Math::vec2 size, int steps, Dir direction);

    void Draw(const Math::TransformationMatrix&) override;

    GameObjectTypes Type()     override { return GameObjectTypes::Staircase; }
    std::string     TypeName() override { return "Staircase"; }

private:
    Math::vec2 size;
    int        stepCount;
    Dir        direction;
};
