#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Physics/Reflection.hpp"
#include "Engine/Polygon.h"
#include "Engine/Vec2.hpp"
#include <vector>

namespace CS230
{
    // Represents static environmental geometry (floors, walls, ceilings) parsed from map data
    class MapElement : public CS230::GameObject
    {
    public:
        // Initializes the map element with a world position and a local polygonal shape
        MapElement(Math::vec2 pos, Polygon polygon, GameObjectTypes in_type = GameObjectTypes::Floor);
        // MapElement(Math::vec2 pos, Polygon polygon);
        void Draw(const Math::TransformationMatrix& camera_matrix) override;

        // Retrieves the world-space boundaries of the polygon for laser/physics intersections
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
        // The base shape of the terrain element in local coordinates
        Polygon         local_polygon;
        GameObjectTypes in_type;
    };
}