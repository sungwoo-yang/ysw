#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Physics/Reflection.hpp"

class Mirror : public CS230::GameObject
{
public:
    // Initialize mirror
    Mirror(Math::vec2 in_position, Math::vec2 in_size, float in_rotation = 0.0f);

    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    // Get reflective physics segment
    [[nodiscard]] Physics::LineSegment GetReflectiveSegment();

    [[nodiscard]] GameObjectTypes Type() override
    {
        return GameObjectTypes::Mirror;
    }

    [[nodiscard]] std::string TypeName() override
    {
        return "Mirror";
    }

private:
    Math::vec2 size;
};