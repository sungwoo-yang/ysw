#include "TargetStar.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"

TargetStar::TargetStar(Math::vec2 position) : CS230::GameObject(position), color(CS200::WHITE), isHit(false), hitTimer(0.0), isBeingHit(false)
{
}

void TargetStar::Update(double dt)
{
    if (isHit)
    {
        CS230::GameObject::Update(dt);
        return;
    }

    if (isBeingHit)
    {
        hitTimer += dt;

        if (hitTimer >= activationTime)
        {
            isHit = true;
            color = 0xFFFF00FF;
        }
    }
    else
    {
        hitTimer = 0.0;
        color    = CS200::WHITE;
    }

    isBeingHit = false;

    CS230::GameObject::Update(dt);
}

void TargetStar::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
{
    auto&                      renderer  = Engine::GetRenderer2D();
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix({ radius, radius });

    renderer.DrawCircle(transform, color);
}

void TargetStar::OnHit()
{
    isBeingHit = true;
}

bool TargetStar::IsHit() const
{
    return isHit;
}

double TargetStar::GetRadius() const
{
    return radius;
}
