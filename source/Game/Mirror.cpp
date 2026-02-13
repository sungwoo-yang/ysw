#include "Mirror.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"

Mirror::Mirror(Math::vec2 in_position, Math::vec2 in_size, float in_rotation) : CS230::GameObject(in_position, static_cast<double>(in_rotation), { 1.0, 1.0 }), size(in_size)
{
    // Set collision box
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>(size.x / 2.0),  static_cast<int>(size.y / 2.0) }
    };
    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void Mirror::Draw(const Math::TransformationMatrix& camera_matrix)
{
    auto&                      renderer  = Engine::GetRenderer2D();
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(size);

    // Draw mirror base
    renderer.DrawRectangle(transform, 0x00FFFF80, CS200::WHITE, 2.0);

    // Draw reflective surface line
    Math::vec2 half = size * 0.5;
    Math::vec2 p1   = { -half.x, 0 };
    Math::vec2 p2   = { half.x, 0 };

    renderer.DrawLine(GetMatrix(), p1, p2, 0x00FFFFFF, 3.0);

    CS230::GameObject::Draw(camera_matrix);
}

Physics::LineSegment Mirror::GetReflectiveSegment()
{
    Math::TransformationMatrix mat = GetMatrix();

    Math::vec2 half = size * 0.5;
    Math::vec2 p1   = { -half.x, 0.0 };
    Math::vec2 p2   = { half.x, 0.0 };

    // Convert local to world coordinates
    Math::vec2 worldP1 = mat * p1;
    Math::vec2 worldP2 = mat * p2;

    return { worldP1, worldP2, true };
}