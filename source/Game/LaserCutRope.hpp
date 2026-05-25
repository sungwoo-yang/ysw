#pragma once

#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"

class LaserCutRope : public CS230::GameObject
{
public:
    LaserCutRope(Math::vec2 in_position, Math::vec2 in_size);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::LaserCutRope;
    }

    std::string TypeName() override
    {
        return "LaserCutRope";
    }

    void Cut();
    bool IsCut() const;

    Math::vec2 GetSize() const
    {
        return size;
    }

private:
    Math::vec2 size;
    bool       isCut = false;
};