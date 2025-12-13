#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Physics/Reflection.hpp"

class Mirror : public CS230::GameObject
{
public:
    Mirror(Math::vec2 position, Math::vec2 size, float rotation = 0.0f);
    
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    Physics::LineSegment GetReflectiveSegment();

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Mirror;
    }

    std::string TypeName() override
    {
        return "Mirror";
    }

private:
    Math::vec2 size;
};