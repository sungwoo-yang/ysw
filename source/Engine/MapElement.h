#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Physics/Reflection.hpp"
#include "Engine/Polygon.h"
#include "Engine/Vec2.hpp"
#include <vector>

namespace CS230
{
    class MapElement : public CS230::GameObject
    {
    public:
        MapElement(Math::vec2 pos, Polygon polygon);
        void Draw(const Math::TransformationMatrix& camera_matrix) override;

        std::vector<Physics::LineSegment> GetWallSegments();

        GameObjectTypes Type() override
        {
            return GameObjectTypes::Floor;
        }

        std::string TypeName() override
        {
            return "MapElement";
        }

    private:
        Polygon local_polygon;
    };
}