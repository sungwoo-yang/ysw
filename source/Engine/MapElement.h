#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/Polygon.h"
#include "Engine/Vec2.hpp"
#include <vector>
#include "Engine/GameObjectTypes.hpp"

namespace CS230
{
    class MapElement : public CS230::GameObject
    {
    public:
        MapElement(Math::vec2 pos, Polygon polygon);

        void Draw(const Math::TransformationMatrix& camera_matrix) override;

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