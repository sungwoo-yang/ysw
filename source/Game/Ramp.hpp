#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"

class Ramp : public CS230::GameObject
{
public:
    enum class Dir { UpRight, UpLeft };

    Ramp(Math::vec2 pos, Math::vec2 size, Dir direction);

    void Draw(const Math::TransformationMatrix&) override;

    GameObjectTypes Type()     override { return GameObjectTypes::Floor; }
    std::string     TypeName() override { return "Ramp"; }

private:
    Math::vec2 size;
    Dir        direction;
};
