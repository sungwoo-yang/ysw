#pragma once

#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"

class FallingBlock : public CS230::GameObject
{
public:
    FallingBlock(Math::vec2 in_position, Math::vec2 in_size);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    bool CanCollideWith(GameObjectTypes other_object_type) override;
    void ResolveCollision(CS230::GameObject* other_object) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::FallingBlock;
    }

    std::string TypeName() override
    {
        return "FallingBlock";
    }

    void Release();
    bool IsReleased() const;

private:
    Math::vec2 size;

    bool   released  = false;
    double velocityY = 0.0;

    const double gravity = 1500.0;
};