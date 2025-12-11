#include "TargetStar.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"

TargetStar::TargetStar(Math::vec2 position) : CS230::GameObject(position), color(CS200::WHITE), isHit(false)
{
}

void TargetStar::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
{
    auto&                      renderer  = Engine::GetRenderer2D();
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix({ radius, radius });

    renderer.DrawCircle(transform, color);
}

void TargetStar::OnHit()
{
    color = 0xFFFF00FF;
    isHit = true;
}

bool TargetStar::IsHit() const
{
    return isHit;
}

double TargetStar::GetRadius() const
{
    return radius;
}
