#include "Engine/Collision.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/RGBA.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/Logger.hpp"
#include "Engine/Polygon.h"

void ProjectPolygon(const Polygon& polygon, const Math::vec2& axis, double& min, double& max)
{
    if (polygon.vertices.empty())
    {
        min = 0;
        max = 0;
        return;
    }

    double project_result = dot(polygon.vertices[0], axis);
    min                   = project_result;
    max                   = project_result;

    for (size_t i = 1; i < polygon.vertices.size(); i++)
    {
        project_result = dot(polygon.vertices[i], axis);
        if (project_result < min)
            min = project_result;
        if (project_result > max)
            max = project_result;
    }
}

namespace CS230
{
    RectCollision::RectCollision(Math::irect bound, GameObject* obj) : boundary(bound), object(obj)
    {
    }

    Math::rect RectCollision::WorldBoundary()
    {
        return { object->GetMatrix() * static_cast<Math::vec2>(boundary.point_1), object->GetMatrix() * static_cast<Math::vec2>(boundary.point_2) };
    }

    void RectCollision::Draw(const Math::TransformationMatrix& display_matrix)
    {
        auto&      renderer       = Engine::GetRenderer2D();
        Math::rect world_boundary = WorldBoundary();

        Math::vec2 bottom_left  = display_matrix * Math::vec2{ world_boundary.Left(), world_boundary.Bottom() };
        Math::vec2 bottom_right = display_matrix * Math::vec2{ world_boundary.Right(), world_boundary.Bottom() };
        Math::vec2 top_left     = display_matrix * Math::vec2{ world_boundary.Left(), world_boundary.Top() };
        Math::vec2 top_right    = display_matrix * Math::vec2{ world_boundary.Right(), world_boundary.Top() };

        renderer.DrawLine(top_left, top_right, CS200::WHITE, 1.0);
        renderer.DrawLine(top_right, bottom_right, CS200::WHITE, 1.0);
        renderer.DrawLine(bottom_right, bottom_left, CS200::WHITE, 1.0);
        renderer.DrawLine(bottom_left, top_left, CS200::WHITE, 1.0);
    }

    bool RectCollision::IsCollidingWith(GameObject* other_object)
    {
        Collision* other_collider = other_object->GetGOComponent<Collision>();

        if (other_collider == nullptr)
        {
            Engine::GetLogger().LogDebug("No collision component found in other object");
            return false;
        }

        Math::rect rectangle_1 = WorldBoundary();

        if (other_collider->Shape() == CollisionShape::Rect)
        {
            Math::rect rectangle_2 = static_cast<RectCollision*>(other_collider)->WorldBoundary();
            return (rectangle_1.Right() > rectangle_2.Left() && rectangle_1.Left() < rectangle_2.Right() && rectangle_1.Top() > rectangle_2.Bottom() && rectangle_1.Bottom() < rectangle_2.Top());
        }
        else if (other_collider->Shape() == CollisionShape::Poly)
        {
            return other_collider->IsCollidingWith(this->object);
        }
        else if (other_collider->Shape() == CollisionShape::Circle)
        {
            Engine::GetLogger().LogDebug("Rect vs Circle collision not yet implemented.");
            return false;
        }

        Engine::GetLogger().LogError("Rect vs unsupported type");
        return false;
    }

    bool RectCollision::IsCollidingWith(Math::vec2 point)
    {
        Math::rect rect = WorldBoundary();
        return (point.x >= rect.Left() && point.x < rect.Right() && point.y >= rect.Bottom() && point.y <= rect.Top());
    }

    CircleCollision::CircleCollision(double rad, GameObject* obj) : radius(rad), object(obj)
    {
    }

    void CircleCollision::Draw(const Math::TransformationMatrix& display_matrix)
    {
        auto& renderer = Engine::GetRenderer2D();

        Math::TransformationMatrix transform = display_matrix * Math::TranslationMatrix(object->GetPosition()) * Math::ScaleMatrix(Math::vec2{ GetRadius(), GetRadius() });

        renderer.DrawCircle(transform, CS200::CLEAR, CS200::WHITE, 1.0);
    }

    double CircleCollision::GetRadius()
    {
        Math::vec2 scale = object->GetScale();
        return scale.x > scale.y ? radius * scale.x : radius * scale.y;
    }

    bool CircleCollision::IsCollidingWith(GameObject* other_object)
    {
        Collision* other_collider = other_object->GetGOComponent<Collision>();
        if (other_collider == nullptr)
        {
            Engine::GetLogger().LogDebug("No collision component found in other object");
            return false;
        }

        if (other_collider->Shape() == CollisionShape::Circle)
        {
            CircleCollision* other       = dynamic_cast<CircleCollision*>(other_collider);
            Math::vec2       A           = object->GetPosition();
            Math::vec2       B           = other->object->GetPosition();
            double           dx          = A.x - B.x;
            double           dy          = A.y - B.y;
            double           distance_sq = dx * dx + dy * dy;
            double           sum_radius  = GetRadius() + other->GetRadius();
            return distance_sq <= sum_radius * sum_radius;
        }
        else
        {
            Engine::GetLogger().LogDebug("Circle vs (Rect/Poly) collision not yet implemented.");
            return false;
        }
    }

    bool CircleCollision::IsCollidingWith(Math::vec2 point)
    {
        Math::vec2 center       = object->GetPosition();
        double     r            = GetRadius();
        Math::vec2 diff         = point - center;
        double     dist_squared = diff.x * diff.x + diff.y * diff.y;
        return dist_squared <= r * r;
    }

    SATCollision::SATCollision(Polygon bound, GameObject* obj) : boundary(bound), object(obj)
    {
        boundary.vertexCount = static_cast<int>(boundary.vertices.size());
    }

    void SATCollision::Draw(const Math::TransformationMatrix& display_matrix)
    {
        auto&   renderer       = Engine::GetRenderer2D();
        Polygon world_boundary = WorldBoundary();

        if (world_boundary.vertexCount < 2)
            return;

        for (int i = 0; i < world_boundary.vertexCount; ++i)
        {
            Math::vec2 p1 = world_boundary.vertices[i];
            Math::vec2 p2 = world_boundary.vertices[(i + 1) % world_boundary.vertexCount];

            renderer.DrawLine(display_matrix * p1, display_matrix * p2, CS200::WHITE, 1.0);
        }
    }

    Polygon SATCollision::WorldBoundary()
    {
        Polygon world_poly;
        world_poly.vertexCount = boundary.vertexCount;
        world_poly.vertices.reserve(boundary.vertexCount);

        const Math::TransformationMatrix& matrix = object->GetMatrix();

        for (const auto& v : boundary.vertices)
        {
            world_poly.vertices.push_back(matrix * v);
        }
        return world_poly;
    }

    bool SATCollision::IsCollidingWith(Math::vec2 point)
    {
        Polygon poly_1 = WorldBoundary();
        if (poly_1.vertexCount == 0)
            return false;

        for (int i = 0; i < poly_1.vertexCount; i++)
        {
            Math::vec2 edge = poly_1.vertices[(i + 1) % poly_1.vertexCount] - poly_1.vertices[i];
            Math::vec2 axis = GetPerpendicular(edge).Normalize();

            double minA, maxA;
            ProjectPolygon(poly_1, axis, minA, maxA);
            double projection = dot(point, axis);

            if (projection < minA || projection > maxA)
            {
                return false;
            }
        }
        return true;
    }

    bool SATCollision::IsCollidingWith(GameObject* other_object)
    {
        Collision* other_collider = other_object->GetGOComponent<Collision>();
        if (other_collider == nullptr)
            return false;

        Polygon poly_1 = WorldBoundary();
        Polygon poly_2;

        if (other_collider->Shape() == CollisionShape::Poly)
        {
            poly_2 = static_cast<SATCollision*>(other_collider)->WorldBoundary();
        }
        else if (other_collider->Shape() == CollisionShape::Rect)
        {
            Math::rect rect = static_cast<RectCollision*>(other_collider)->WorldBoundary();
            poly_2.vertices = {
                {  rect.Left(), rect.Bottom() },
                { rect.Right(), rect.Bottom() },
                { rect.Right(),    rect.Top() },
                {  rect.Left(),    rect.Top() }
            };
            poly_2.vertexCount = 4;
        }
        else if (other_collider->Shape() == CollisionShape::Circle)
        {
            Engine::GetLogger().LogDebug("SATCollision vs Circle-Collision.");
            return false;
        }

        if (poly_1.vertexCount == 0 || poly_2.vertexCount == 0)
            return false;

        for (int i = 0; i < poly_1.vertexCount; i++)
        {
            Math::vec2 edge = poly_1.vertices[(i + 1) % poly_1.vertexCount] - poly_1.vertices[i];
            Math::vec2 axis = GetPerpendicular(edge).Normalize();

            double minA, maxA, minB, maxB;
            ProjectPolygon(poly_1, axis, minA, maxA);
            ProjectPolygon(poly_2, axis, minB, maxB);

            if (maxA < minB || maxB < minA)
                return false;
        }

        for (int i = 0; i < poly_2.vertexCount; i++)
        {
            Math::vec2 edge = poly_2.vertices[(i + 1) % poly_2.vertexCount] - poly_2.vertices[i];
            Math::vec2 axis = GetPerpendicular(edge).Normalize();

            double minA, maxA, minB, maxB;
            ProjectPolygon(poly_1, axis, minA, maxA);
            ProjectPolygon(poly_2, axis, minB, maxB);

            if (maxA < minB || maxB < minA)
                return false;
        }

        return true;
    }

}