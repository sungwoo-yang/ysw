#include "Engine/MapElement.h"
#include "CS200/IRenderer2D.hpp"
#include "CS200/RGBA.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"

namespace CS230
{
    MapElement::MapElement(Math::vec2 pos, Polygon polygon, GameObjectTypes type) 
    : CS230::GameObject(pos), local_polygon(std::move(polygon)), in_type(type)
    {
        local_polygon.vertexCount = static_cast<int>(local_polygon.vertices.size());

        if (type == GameObjectTypes::Floor)
        {
            AddGOComponent(new SATCollision(local_polygon, this));
        }
    }
    // MapElement::MapElement(Math::vec2 pos, Polygon polygon) : CS230::GameObject(pos), local_polygon(std::move(polygon))
    // {
    //     local_polygon.vertexCount = static_cast<int>(local_polygon.vertices.size());

    //     AddGOComponent(new SATCollision(local_polygon, this));
    // }

    void MapElement::Draw(const Math::TransformationMatrix& camera_matrix)
    {
        if (in_type == GameObjectTypes::Background)
        {
            return;
        }

        CS200::IRenderer2D& renderer = Engine::GetRenderer2D();

        const Math::TransformationMatrix& model_matrix = GetMatrix();

        // Polygons require at least a valid line segment (2 points) to be drawn
        if (local_polygon.vertexCount < 2)
            return;

        // Render the polygon wireframe by connecting adjacent vertices
        for (size_t i = 0; i < static_cast<size_t>(local_polygon.vertexCount); ++i)
        {
            Math::vec2 p1 = local_polygon.vertices[i];
            Math::vec2 p2 = local_polygon.vertices[(i + 1) % static_cast<size_t>(local_polygon.vertexCount)];

            renderer.DrawLine(model_matrix, p1, p2, CS200::WHITE, 1.0);
        }

        CS230::GameObject::Draw(camera_matrix);
    }

    std::vector<Physics::LineSegment> CS230::MapElement::GetWallSegments()
    {
        std::vector<Physics::LineSegment> segments;
        Math::TransformationMatrix        mat = GetMatrix();

        const auto& verts = local_polygon.vertices;

        // Transform all local polygon edges into absolute world-space line segments
        for (size_t i = 0; i < verts.size(); ++i)
        {
            Math::vec2 p1 = mat * verts[i];
            Math::vec2 p2 = mat * verts[(i + 1) % verts.size()];

            // The 'false' flag indicates that standard map geometry absorbs lasers instead of reflecting them
            segments.push_back({ p1, p2, false });
        }
        return segments;
    }
}