#include "Engine/MapElement.h"
#include "CS200/IRenderer2D.hpp" 
#include "CS200/RGBA.hpp"      
#include "Engine/Engine.hpp"     
#include "Engine/Collision.hpp"

namespace CS230
{
    MapElement::MapElement(Math::vec2 pos, Polygon polygon)
        : CS230::GameObject(pos),
          local_polygon(std::move(polygon))
    {
        local_polygon.vertexCount = static_cast<int>(local_polygon.vertices.size());

        AddGOComponent(new SATCollision(local_polygon, this));
    }

    void MapElement::Draw(const Math::TransformationMatrix& camera_matrix)
    {
        CS200::IRenderer2D& renderer = Engine::GetRenderer2D();

        const Math::TransformationMatrix& model_matrix = GetMatrix();

        if (local_polygon.vertexCount < 2)
            return;

        for (int i = 0; i < local_polygon.vertexCount; ++i)
        {
            Math::vec2 p1 = local_polygon.vertices[i];
            Math::vec2 p2 = local_polygon.vertices[(i + 1) % local_polygon.vertexCount];

            renderer.DrawLine(model_matrix, p1, p2, CS200::WHITE, 1.0);
        }

        CS230::GameObject::Draw(camera_matrix);
    }
}