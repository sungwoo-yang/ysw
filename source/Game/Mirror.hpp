#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Physics/Reflection.hpp"

class Mirror : public CS230::GameObject
{
public:
    Mirror(Math::vec2 in_position, Math::vec2 in_size, float in_rotation = 0.0f);
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Mirror;
    }

    std::string TypeName() override
    {
        return "Mirror";
    }

    // Returns the world-space segment used for reflecting laser beams
    Physics::LineSegment GetReflectiveSegment();

private:
    Math::vec2 size;
};