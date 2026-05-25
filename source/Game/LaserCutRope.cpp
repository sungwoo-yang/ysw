#include "LaserCutRope.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"

LaserCutRope::LaserCutRope(Math::vec2 in_position, Math::vec2 in_size)
    : CS230::GameObject(in_position), size(in_size)
{
    Math::irect collision_box{
        { static_cast<int>(-size.x / 2.0), static_cast<int>(-size.y / 2.0) },
        {  static_cast<int>( size.x / 2.0),  static_cast<int>( size.y / 2.0) }
    };

    AddGOComponent(new CS230::RectCollision(collision_box, this));
}

void LaserCutRope::Update(double dt)
{
    if (isCut)
    {
        return;
    }

    CS230::GameObject::Update(dt);
}

void LaserCutRope::Draw(const Math::TransformationMatrix& camera_matrix)
{
    if (isCut)
    {
        return;
    }

    auto& renderer = Engine::GetRenderer2D();

    Math::TransformationMatrix transform = GetMatrix() * Math::ScaleMatrix(size);

    renderer.DrawRectangle(transform, 0x00CC00FF, CS200::WHITE, 1.0);

    CS230::GameObject::Draw(camera_matrix);
}

void LaserCutRope::Cut()
{
    if (isCut)
    {
        return;
    }

    isCut = true;
    RemoveGOComponent<CS230::RectCollision>();

    Engine::GetLogger().LogEvent("LaserCutRope cut!");
}

bool LaserCutRope::IsCut() const
{
    return isCut;
}