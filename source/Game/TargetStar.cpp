#include "TargetStar.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"

TargetStar::TargetStar(Math::vec2 in_position) : CS230::GameObject(in_position), color(CS200::WHITE), isHit(false), hitTimer(0.0), isBeingHit(false)
{
}

void TargetStar::Update(double dt)
{
    // If already fully activated, lock the state and skip timer logic
    if (isHit)
    {
        CS230::GameObject::Update(dt);
        return;
    }

    // Accumulate charge time if currently being hit by a laser this frame
    if (isBeingHit)
    {
        hitTimer += dt;

        // Fully activate target when threshold is reached
        if (hitTimer >= activationTime)
        {
            isHit = true;
            color = 0xFFFF00FF; // Change visual color to Yellow to indicate activation
        }
    }
    else
    {
        // Reset progress entirely if the laser beam is broken or moves off target
        hitTimer = 0.0;
        color    = CS200::WHITE;
    }

    // Reset flag for the next frame (must be continuously set by the laser's Update)
    isBeingHit = false;

    CS230::GameObject::Update(dt);
}

void TargetStar::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
{
    auto&                      renderer  = Engine::GetRenderer2D();
    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix({ radius, radius });

    // Draw the target visually as a circle (Color changes based on hit progress/activation)
    renderer.DrawCircle(transform, color);
}

void TargetStar::OnHit()
{
    // Flag to register a laser impact for the current frame
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
